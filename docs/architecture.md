# Ramen Architecture

How a Ramen node is built, how it is configured, and how the web interfaces
relate to it. This document records decisions that are stable across feature
sets. Feature-level detail belongs in `changes/`.

## The governing rule

> Build configuration establishes the upper bound of possible behavior.
> Runtime configuration selects a valid subset of that behavior.
> Runtime state reports how much of that requested behavior is currently
> achievable.

Three layers, each constraining the next:

```
BUILD CAPABILITIES    what this binary could ever do        immutable, compile-time
        | constrains
DESIRED CONFIGURATION what the deployment wants it to do    mutable, NVS + optional cloud sync
        | reconciled into
EFFECTIVE STATE       what is actually running right now    transient, reported upward
```

## Deployment paths

A node supports two independent ways of reaching a browser. They are not
mutually exclusive.

```
                  CLOUD PATH
Node -- Wi-Fi --> Supabase --> cloud dashboard --> browser

                  LOCAL PATH
Node <-------- local Wi-Fi --------> browser
       node-served UI + HTTP/WS
```

### The local path is served by the node, not by the cloud app

The cloud dashboard is not the local interface. A page hosted at a public
origin still needs the client to have internet access in order to load, which
defeats the purpose of a local deployment, and browsers are tightening
Local Network Access rules such that a public HTTPS origin talking to
`http://192.168.x.x` may prompt for permission or be blocked outright. That
specification is still experimental and is not implemented consistently,
Safari/iOS in particular.

A node that serves its own UI makes everything same-origin and removes the
problem. The only requirement on the client is a browser and a Wi-Fi link.

Web Bluetooth was evaluated and rejected as the local transport: it is
effectively Chromium-only, and the project requires access from any device
without shipping a native app. BLE may still be useful later for provisioning
only, handing a node Wi-Fi credentials before it joins a network.

### AP mode is the baseline local deployment

```
LOCAL
|
+-- Direct    node creates its own Wi-Fi, client connects to it     (canonical)
|             predictable address, e.g. 192.168.4.1, captive portal optional
|
+-- LAN       node joins existing Wi-Fi, client reaches ramen-xxxx.local
              enhancement for multi-viewer or coexisting deployments
```

Direct AP mode makes the fewest assumptions, so it is the canonical local
deployment. LAN mode is an enhancement, not a prerequisite.

## Capabilities, not modes

"Mode" is the wrong abstraction once a node needs to serve a local dashboard
*and* upload to the cloud at the same time. Model independently enabled
capabilities instead:

```json
{
  "network":  { "wifi_sta": true, "wifi_ap": true, "mesh": false },
  "services": { "local_dashboard": true, "cloud_sync": true, "mesh_forwarding": false }
}
```

ESP-IDF supports simultaneous station and SoftAP operation via
`WIFI_MODE_APSTA`, so the networking layer permits this topology.

### Roles are presets, not firmware variants

`STANDALONE`, `GATEWAY`, and `LEAF` are named sets of capability flags. They
are a convenience for humans; the runtime only ever sees flags.

| Capability         | Standalone | Gateway  | Leaf |
| ------------------ | ---------- | -------- | ---- |
| Sensor acquisition | yes        | yes      | yes  |
| Wi-Fi STA          | optional   | yes      | no   |
| SoftAP             | optional   | optional | no   |
| Embedded website   | optional   | optional | no   |
| Cloud sync         | optional   | yes      | no   |
| Mesh TX/RX         | no         | yes      | yes  |

This is what answers "a mesh node has no reason to serve a website": a Leaf is
simply `local_ui: false, cloud: false, mesh: true`.

**One firmware image covers all three roles.** Not `gateway.bin` +
`leaf.bin` + `standalone.bin`; separate artifacts make version management
painful as soon as the variants share most of their implementation.

## Configuration authority

**The database is a configuration source, not the authority.** Making a
database key the fundamental mode switch is circular: the database would decide
whether a node should use a communications path that may itself have no
database connectivity.

NVS on the node holds the authoritative copy. The cloud may push a desired
configuration document, and the node persists and reconciles it. Lose the
internet and the node keeps running on stored configuration.

```
Cloud API ---- desired configuration ----> ConfigurationManager
Local UI   ------------------------------>         |
Serial     ------------------------------>         +--> NVS (authoritative)
Provisioning ---------------------------->         |
                                                   +--> Reconciler --> services
```

All configuration sources submit through one `ConfigurationManager`. No
subsystem mutates the running system directly. Setting a value does not stop a
service; it validates, persists, increments a revision, and lets the reconciler
converge desired against actual. This is the control-plane model, and it suits
devices that are intermittently connected.

### Identity

Keep three identifiers separate:

- `device_id` is durable and survives redeployment.
- `deployment_id` is a foreign key and is expected to change.
- `config_revision` tracks configuration versions.

Do not use a deployment-scoped hash as the intrinsic device identity. A node
moved from one deployment to another keeps its identity.

### Validation

Runtime configuration may request a capability the build does not contain. That
must be **rejected explicitly**, never silently ignored, and validated
**transactionally** so a node is never left half-transitioned.

```
new config -> schema validation -> capability validation
           -> cross-setting validation -> resource validation
           -> valid? commit and reconcile : reject whole document
```

| Class                      | Meaning                                                |
| -------------------------- | ------------------------------------------------------ |
| `CAPABILITY_NOT_AVAILABLE` | not compiled into this binary                          |
| `HARDWARE_NOT_AVAILABLE`   | firmware supports it, this board revision does not     |
| `CONFIGURATION_CONFLICT`   | each flag valid alone, the combination is not          |
| `RESOURCE_LIMIT_EXCEEDED`  | valid, but does not fit in available RAM/flash         |

A valid configuration that cannot currently activate is **not** an error.
Desired stays `true` while effective reports the reason:

```json
{ "cloud_sync": { "desired": true, "active": false, "reason": "wifi_unavailable" } }
```

The distinction between *invalid* and *currently unavailable* is load-bearing.
The cloud dashboard may read a node's advertised capabilities and disable
unavailable options, but client-side validation is convenience only. Firmware
is the authority.

## Telemetry does not know where it is going

Sensors publish to an internal event bus. Sinks consume independently, and
each sink is enabled by configuration.

```
                        +-- CloudSink
                        |
Sensors --> EventBus ---+-- LocalWebSocketSink
                        |
                        +-- MeshSink
                        |
                        +-- LoggerSink
```

A leaf enables only `MeshSink`. A gateway enables `MeshReceiver` plus
`CloudSink`. Critically, **cloud ingestion does not care whether a measurement
originated locally or arrived over the mesh**; everything normalizes to one
event representation before it reaches a sink.

## Services and tasks

Each *capability* gets a service that can start and stop independently, under a
central `ServiceManager` that reconciles against desired configuration.

```
ConfigurationManager --desired--> ServiceManager
                                       |
                        +--------------+--------------+
                     CloudService  LocalWebService  MeshService
```

Not `StandaloneTask` / `GatewayTask` / `MeshTask`. Those cannot express
"cloud and local UI at once".

Do **not** create a FreeRTOS task per endpoint or per subsystem. Task stacks
cost real RAM and add synchronization surface. Lightweight services can share
an event loop, and the HTTP server framework already handles its own
concurrency.

**Disabling services does not grant the remainder more throughput.** FreeRTOS
distributes CPU among runnable tasks, and in a typical configuration the
services are a small fraction of available CPU (sensor ~5%, cloud ~3%,
HTTP ~2%, mesh ~8%). **Memory, not CPU, is normally the binding constraint on
an ESP32.** Design the capability system so it *could* account for per-service
RAM and flash requirements later.

## The ESP-NOW channel constraint

This is the item to prototype early, because it constrains the topology rather
than the implementation.

ESP32 supports simultaneous STA and SoftAP, **but in AP+STA coexistence the
SoftAP follows the channel of the external AP the station is associated with**,
and ESP-NOW shares the same radio. A gateway doing Wi-Fi uplink plus ESP-NOW to
leaf nodes plus SoftAP for a local dashboard is feasible in principle, but
channel coordination becomes part of the network architecture.

## Build configuration

Target ESP-IDF `Kconfig.projbuild` with named `sdkconfig.defaults` profiles, so
build variants are selected externally rather than by editing source. ESP-IDF
supports Arduino as a component, so existing Arduino APIs can be retained while
gaining CMake, Kconfig, and reproducible profiles. Avoid multiple near-identical
`.ino` sketches; they become brittle once variants share most of their code.

Build time owns hardware revision, MCU target, flash size, partition layout,
compiled feature availability, and manufacturing data.

Runtime owns role, deployment ID, Wi-Fi credentials, which services are
enabled, mesh ID, gateway ID, sampling rate, telemetry interval, and database
destination. None of those should require recompilation.

## The device protocol is the central artifact

Firmware, embedded UI, cloud backend, and cloud UI are all consumers of one
versioned contract.

```
GET  /api/v1/device
GET  /api/v1/config
PUT  /api/v1/config
POST /api/v1/commands
WS   /api/v1/telemetry
```

The node publishes its own capabilities so a UI can adapt rather than assuming
every build has every feature:

```json
{
  "api_version": "1",
  "firmware_version": "1.8.0",
  "capabilities": { "mesh": true, "cloud": true, "local_ui": true }
}
```

The node does not need to know anything about the UI. The embedded dashboard is
the reference client that happens to ship with the firmware, which leaves room
for third-party clients against the same contract.

## Web layout

One repository, two applications, shared packages. A separate application is
not the same thing as a separate repository.

```
web/
  packages/
    device-model    domain types: telemetry, config, commands
    device-client   DeviceClient interface + Cloud and Local implementations
    ui              shared presentation components
  apps/
    cloud           hosted dashboard, Supabase-backed
    embedded        static SPA served from node flash
```

Both applications compose the same components against the same `DeviceClient`
interface, so components never know where a node is.

They are deliberately not one application. The cloud app accumulates accounts,
fleet overview, permissions, firmware management, and long history, none of
which belong on a device. Unifying them produces `if (isLocal)` throughout the
codebase.

**The bundle-size boundary is a hard constraint.** The embedded build must
never import the cloud application, authentication, analytics, or the database
client. The dependency graph should make that difficult rather than relying on
discipline. The embedded target is a static build (no SSR, no server
components, no API routes); the node serves bytes and the browser runs the app.

Split into separate repositories only when there is an organizational reason:
different teams, genuinely independent release cycles, or a security boundary.
Not preemptively.

## Provenance

The reasoning behind these decisions was worked out in a design conversation on
ESP32 web connectivity. The load-bearing conclusions are recorded above; the
alternatives that were considered and rejected (Web Bluetooth as primary
transport, database-as-mode-switch, per-mode firmware images, per-mode tasks,
two repositories) are noted so they are not relitigated without new
information.
