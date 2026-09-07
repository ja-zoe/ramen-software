# Embedded dashboard

The local interface, built as a static SPA and served from node flash. A client
needs only a browser and a Wi-Fi link to the node: no router, no internet, no
account, no app install.

Because the page is served by the node, it is same-origin with `/api/v1` and
`/ws`, which avoids the browser Local Network Access restrictions that a public
HTTPS origin would hit when contacting a private address.

Scope is deliberately narrower than the cloud application:

- current device state and live telemetry
- configuration and calibration
- controls and diagnostics

Composes shared `ui` components with `LocalDeviceClient`.

## Constraints

- Static build only. No SSR, no server components, no API routes. The node
  serves bytes; the browser executes the application.
- The output must fit in flash alongside firmware, so the bundle budget is
  real. Never import the cloud application, authentication, analytics, or the
  Supabase client.
- Treat this as the reference client for the node protocol. Anything it needs
  should be available to a third-party client through the same documented API.
