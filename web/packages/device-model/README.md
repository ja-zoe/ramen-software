# @ramen/device-model

Domain types shared by firmware-facing clients and both applications. This
package is the TypeScript expression of the device protocol described in
`../../../docs/architecture.md`.

Intended contents:

- `Telemetry` for a normalized measurement event. Cloud and mesh-relayed
  readings normalize to the same shape, so nothing downstream needs to know
  where a reading originated.
- `DeviceConfig` for the desired configuration document, including the
  `network` and `services` capability flags.
- `DeviceCapabilities` for what a given firmware build can do.
- `EffectiveState` for desired-versus-active reporting, including the reason a
  desired service is not currently active.
- `DeviceCommand` for command definitions.
- Validation schemas for the above.

Sensor outputs to model: PM1.0, PM2.5, PM4.0, PM10 in micrograms per cubic
meter; VOC index 0 to 500, baseline 100; NOx index 0 to 500, baseline 1;
temperature in Celsius; relative humidity as a percentage.

Note that VOC and NOx are **relative indices, not concentrations**. They should
not be given the same axis semantics as particulate matter.

Depends on nothing else in this workspace.
