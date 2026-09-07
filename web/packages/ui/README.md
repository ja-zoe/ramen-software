# @ramen/ui

Presentation components shared by both applications. Components receive data
and a `DeviceClient`; they do not know whether they are running against a cloud
backend or a node on the local network.

Expected components: device status, live readings, sensor charts, device
controls, configuration panel.

Charting note: readings are sampled every 5 seconds, so a node produces roughly
17,000 rows per day. Indoor pollutant events resolve in under a minute, and
those transients are the signal most worth seeing. Default views should avoid
aggressive time-bucket averaging that smooths them away.

Depends on `device-model` only. Must not depend on `device-client`, Supabase,
or either application.
