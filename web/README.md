# Ramen Web

Two applications over shared packages. See `../docs/architecture.md` for why
the split is shaped this way.

```
packages/
  device-model    domain types: telemetry, config, commands, capabilities
  device-client   DeviceClient interface + Cloud and Local implementations
  ui              shared presentation components
apps/
  cloud           hosted dashboard, Supabase-backed
  embedded        static SPA served from node flash
```

## Dependency rules

These exist to keep the embedded bundle small enough to live in ESP32 flash.
They are the constraint most likely to be violated by accident.

- `ui` may depend on `device-model`. Never on `device-client`, Supabase, or
  either app.
- `device-client` may depend on `device-model`. Never on `ui`.
- `apps/embedded` may depend on `ui`, `device-model`, and the local transport
  from `device-client`. **Never** on `apps/cloud`, authentication, analytics,
  or the Supabase client.
- `apps/cloud` may depend on anything in `packages/`.

## Status

Structure and contracts only. No build tooling, package manifests, or
application code yet; those arrive through spec-driven feature sets tracked in
`../changes/`.
