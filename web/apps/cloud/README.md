# Cloud dashboard

The hosted public dashboard for Rutgers air-quality data, backed by Supabase.

Composes shared `ui` components with `CloudDeviceClient`, plus the concerns
that only make sense off-device:

- fleet and deployment overview
- historical database charts and time-range selection
- accounts, permissions, and operator roles
- remote configuration and firmware management

Telemetry viewing is a low-friction public experience. Authentication is
reserved for authorized operators and privileged operations such as changing
node parameters, and authorization must be enforced in Supabase rather than
hidden in the UI.

This application is **not** the local interface. See `../embedded/`.
