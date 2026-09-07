# @ramen/device-client

The transport abstraction. Shared UI components talk to this interface and
never learn where a node actually is.

```ts
interface DeviceClient {
  getInfo(): Promise<DeviceInfo>
  getConfig(): Promise<DeviceConfig>
  updateConfig(config: DeviceConfig): Promise<void>
  subscribeTelemetry(cb: (event: TelemetryEvent) => void): Unsubscribe
  sendCommand(command: DeviceCommand): Promise<void>
}
```

Two implementations:

```
CloudDeviceClient  --> Supabase / cloud API
LocalDeviceClient  --> node HTTP + WebSocket on /api/v1
```

`LocalDeviceClient` targets the versioned node protocol: `GET /api/v1/device`,
`GET|PUT /api/v1/config`, `POST /api/v1/commands`, and `WS /api/v1/telemetry`.

This interface is the compatibility boundary between the two deployment paths.
Keep it stable; when it must change, version the node protocol alongside it.

Depends on `device-model`. Must not depend on `ui`.
