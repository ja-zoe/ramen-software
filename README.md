# Ramen

**Rutgers Air Monitoring and Environmental Network.** Low-cost distributed air
quality sensing across campus, built by Students for Environmental and Energy
Development.

This repository holds the node firmware and both web interfaces.

## Why

Air quality measurably affects cognition, respiratory health, and academic
performance, but campuses have no continuous monitoring, so problem areas go
unidentified and interventions go unjustified. Spot checks miss what matters:
in the Spring 2026 deployment, one dorm room showed sustained elevated VOC
activity with occupancy-driven daily peaks while another showed a clean
baseline punctuated by ten-minute PM2.5 spikes reaching 92.8 ug/m3. Neither
pattern is visible to periodic sampling.

## Layout

```
firmware/         node firmware
  arduino/        currently deployed sketches
web/
  packages/       device-model, device-client, ui
  apps/
    cloud/        hosted Supabase-backed dashboard
    embedded/     static SPA served from node flash
docs/
  architecture.md architecture decisions and their rationale
changes/          spec-driven feature tracking
```

## Two deployment paths

```
CLOUD    node --wifi--> Supabase --> hosted dashboard --> browser
LOCAL    node <--local wifi--> browser, UI served by the node itself
```

The local path is served by the node rather than by the hosted dashboard. A
page on a public origin still needs internet access to load, which defeats a
local deployment, and browsers increasingly restrict public origins from
contacting private network addresses. A node serving its own UI is same-origin
and needs only a browser and a Wi-Fi link.

Both paths speak the same versioned device protocol, and both applications
compose the same components against the same `DeviceClient` interface.

## Node hardware

ESP32-C3 paired with a Sensirion SEN54 on a custom PCB in a 3D-printed
enclosure. Outputs PM1.0, PM2.5, PM4.0, PM10 in ug/m3, VOC index, NOx index,
temperature, and relative humidity, sampled every 5 seconds.

VOC and NOx are relative indices on a 0 to 500 scale, not concentrations. They
indicate departures from a local baseline rather than threshold exceedances.

## Working in this repository

Read `AGENTS.md` and `changes/CONTEXT.md` before starting feature work. Feature
work is tracked in `changes/` using the `spec-driven-dev` workflow; specs and
test schemes are written before source changes.

`main` is the single integration branch. The pre-restructure flat sketch layout
is preserved on the `v1-arduino` branch.

## Credentials

`config.h` is gitignored throughout. Copy the adjacent `config.h.example` and
fill it in locally. Never commit real credentials.
