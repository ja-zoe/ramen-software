# Ramen Firmware

## Current state

`arduino/` holds the sketches that produced the Spring 2026 dorm-room
deployment. They are the working, deployed firmware and remain the reference
for node behavior until the restructure below lands.

```
arduino/
  main/           deployed node firmware: SEN54 over I2C, Supabase upload
  battery_test/   battery characterization
  Wifi_sketch/    Wi-Fi connectivity experiment
```

`config.h` is gitignored in every sketch directory because it holds Wi-Fi and
Supabase credentials. Copy `config.h.example` and fill it in locally. Never
commit the real file.

## Target structure

Per `../docs/architecture.md`, firmware moves to ESP-IDF with Kconfig-selected
build profiles and Arduino available as a component, so existing Arduino APIs
survive the move:

```
firmware/
  main/
    app_main.cpp
    config_manager.cpp
    cloud/
    mesh/
    local_ui/
  Kconfig.projbuild
  configs/
    standalone.defaults
    gateway.defaults
    leaf.defaults
  CMakeLists.txt
```

One firmware image serves all three roles. The profiles select compiled
*capabilities*; they do not select behavior. Behavior is runtime configuration
persisted in NVS.

Do not add per-role `.ino` sketches. Variants that share most of their
implementation become brittle quickly, which is the reason for the Kconfig
approach.

## Build-time versus runtime

| Build time                                    | Runtime                                    |
| --------------------------------------------- | ------------------------------------------ |
| hardware revision, MCU target, flash size     | role and deployment ID                     |
| partition layout                              | Wi-Fi credentials                          |
| compiled feature availability                 | which services are enabled                 |
| manufacturing data                            | mesh ID, gateway ID                        |
| debug/release behavior                        | sampling rate, telemetry interval          |

Nothing in the right column should require recompilation.

## Prototype early

The ESP-NOW channel constraint. In AP+STA coexistence the SoftAP follows the
channel of the external AP the station is associated with, and ESP-NOW shares
the same radio. A gateway doing Wi-Fi uplink plus ESP-NOW plus SoftAP is
feasible in principle, but channel coordination is an architecture concern, not
an implementation detail. Validate it before committing to the gateway design.
