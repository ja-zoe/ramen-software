# Project Context: Ramen

Shared bootstrap for every revision set. Read this once at the start of a
session. Keep only durable project-wide facts here; set- and feature-specific
details belong in their revision files.

## App

**Ramen** is the Rutgers Air Monitoring and Environmental Network: distributed
low-cost air quality sensor nodes and the interfaces that read them. This
repository holds the node firmware and both web applications.

Two deployment paths, which are independent and may run simultaneously:

- **Cloud.** Nodes upload to Supabase; a hosted dashboard provides a
  low-friction public viewing experience over production telemetry.
- **Local.** A node serves its own static dashboard over local Wi-Fi and
  operates with no router, internet, database, or account.

## Invariants

- **Revision workflow:** all feature work follows
  `.agents/skills/spec-driven-dev/SKILL.md` and is tracked under `changes/`.
- **Branch strategy:** `main` is the single integration branch. Revision sets
  branch from `main` as `feat/setN-<slug>`; feature branches fork from their set
  branch as `feat/setN/RN.M-<slug>`.
- **Merge gates:** a feature merges only after its documented test scheme
  passes. A revision set merges to `main` only after all features pass, the app
  boots cleanly, and the user explicitly approves the merge.
- **Architecture:** `docs/architecture.md` is authoritative for node
  architecture, configuration layering, and the web split. Do not contradict it
  without recording the change there.
- **Device protocol is the central artifact:** firmware, embedded UI, cloud
  backend, and cloud UI are all consumers of one versioned contract
  (`/api/v1/device`, `/api/v1/config`, `/api/v1/commands`,
  `WS /api/v1/telemetry`). Do not invent a dashboard-only protocol; reconcile
  changes across firmware and web together.
- **Deployed firmware:** `firmware/arduino/main` is the firmware that produced
  the Spring 2026 deployment and is the reference for current node behavior.
  Cloud mode must keep working with it and the existing database.
- **Configuration layering:** build configuration establishes the upper bound of
  possible behavior; runtime configuration selects a valid subset; runtime state
  reports how much is currently achievable. NVS on the node is the authoritative
  configuration store. The database is a configuration source, never the
  authority.
- **Application stack:** TypeScript, React, and Vite; Supabase JS for cloud
  data; TanStack Query for server state; Apache ECharts for charts; pnpm for
  package management; Vitest and Playwright for automated validation.
- **Web dependency boundaries:** `web/apps/embedded` must never import
  `web/apps/cloud`, authentication, analytics, or the Supabase client, because
  it ships inside ESP32 flash. `web/packages/ui` depends on `device-model` only,
  never on a transport.
- **Access model:** telemetry is primarily a low-friction public experience.
  Authentication is reserved for authorized operators and future privileged
  operations such as changing node parameters. Authorization must be enforced
  in Supabase, not only hidden in the UI.
- **Initial cloud scope:** node inventory and online/offline state, current
  readings, historical charts with time-range selection, sensor/event logs, and
  clear connection/stale-data states. Alerts, node configuration, exports, maps,
  and firmware management are out of scope for the initial iteration.
- **Telemetry presentation:** store and query timestamps in UTC and display
  them in the viewer's local timezone. Temperature supports Celsius and
  Fahrenheit; particulate matter is displayed in micrograms per cubic meter.
  VOC and NOx are relative 0-500 indices, not concentrations, and must not be
  given the same axis semantics as particulate matter.
- **Sampling reality:** nodes sample every 5 seconds, roughly 17,000 rows per
  node per day. Indoor pollutant events resolve in under a minute, and those
  transients are the signal most worth seeing. Default views must not smooth
  them away.
- **Responsive target:** both mobile field use and desktop operations are
  first-class. Do not establish a visual direction or brand treatment without
  user review.
- **Credentials:** `config.h` is gitignored throughout `firmware/`. Only
  `config.h.example` is tracked. Never commit real credentials.

## Production data model

- **Backend:** Supabase/Postgres.
- **Node identity:** `nodes.mac_address` is the firmware's node identifier.
- **Tables:**
  - `nodes(mac_address, sensor_model, firmware_version, created_at)`
  - `measurements(id, node_id, captured_at, created_at, pm1_0, pm2_5, pm4_0,
    pm10_0, humidity, temperature, voc_index, nox_index, error_code)`
  - `logs(id, node_id, event_type, severity, message, created_at)`
  - `locations(id, campus_name, building_name, room_number, description)`
  - `deployments(id, node_id, location_id, start_at, end_at)`
- **Location history:** resolve a measurement's location from the node's active
  deployment at the measurement timestamp; do not treat location as a permanent
  node property.
- **Identity separation:** device identity, deployment association, and
  configuration revision are three distinct identifiers. A node moved between
  deployments keeps its device identity.

## Decisions already made

Recorded here because they close questions that were previously open. Rationale
is in `docs/architecture.md`.

- Local access is **served by the node**, not by the hosted dashboard reaching
  into the LAN. This replaces the earlier idea of a cloud-versus-direct source
  selector inside one application.
- Web Bluetooth is rejected as the local transport (effectively Chromium-only).
  BLE may return later for provisioning only.
- Direct AP mode is the canonical local deployment; joining an existing LAN is
  an enhancement.
- Node behavior is modeled as independently enabled capabilities, not mutually
  exclusive modes. `STANDALONE`, `GATEWAY`, and `LEAF` are configuration
  presets.
- One firmware image covers all roles. No per-role binaries.
- Firmware and web live in one repository with separate applications over
  shared packages. Separate repositories only when there is an organizational
  reason.

## Decisions still to make

- Supported browser/platform matrix, and whether mDNS (`ramen-xxxx.local`) is
  reliable enough on iOS to be a documented entry point
- Public versus operator visibility for node identifiers and event logs
- Supabase public-read policies/views and operator roles
- Visual direction and Rutgers brand treatment
- Deployment target and environment constraints for the cloud application
- Exact production measurement interval and history-retention requirements
- Migration path and timing from the Arduino sketches to the ESP-IDF/Kconfig
  structure, including whether Arduino-as-component is used
- ESP-NOW channel coordination for a gateway running Wi-Fi uplink, mesh, and
  SoftAP simultaneously; needs a prototype before the gateway design is fixed

Add decisions here only when they become stable project-wide constraints.
