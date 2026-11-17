# ESP32 Interrupter Library

The main feature is BLE remote configurability: users can change module
settings and trigger actions from a remote. This project is a library and set
of examples for building your own ESP32 interrupter **modules**. A module is a
firmware entry point that defines its settings, signal-generation behavior,
and hardware connections.

The examples are selected with PlatformIO environments:

- `regular` — configurable pulse train using MCPWM.
- `mains-ramped` — zero-cross-triggered pulses using GPIO and a hardware timer.
- `musical` — playback of a stored tone sequence.

Build an example with `pio run -e regular` (replace `regular` with another
environment). To add a module, create a source file in `src/` and an
environment in `platformio.ini` whose `build_src_filter` selects it.

## BLE and capabilities

`BLEEngine` provides the BLE service and sends capability and telemetry
reports. Initialize it once with a device name. The remote connects, requests
capability definitions, then writes setting updates as a JSON object mapping
capability names to values, for example `{"BPS":20}`.

A **capability** is a remotely configurable setting. Use
`NumberCapability` for bounded numbers, `ToggleCapability` for on/off values,
`ButtonCapability` for momentary actions, and `EnumCapability` for a choice
from options. Register each capability with the static
`CapabilityRegistry::registerCapability()` API before starting BLE.
`CapabilityRegistry::setValue()` applies firmware-side changes through
validation and callbacks; read the current value directly from the capability
object’s `.value`.

Individual capability callbacks can apply accepted settings to hardware. The
registry also supports a change callback and a pre-change callback for
cross-setting validation or safety overrides. The pre-change callback runs for
incoming BLE updates; firmware-side `setValue()` calls skip it. `TelemetryValue`
provides read-only values that firmware can update and report through
`BLEEngine`.

# Generating pulses

`Interrupter` provides GPIO interrupt registration and a hardware timer. Use
these when pulses must be synchronized to an external event, as in
`mains-ramped`. The callbacks run on the CPU in interrupt context, so their
timing includes software and interrupt latency.

For a regular pulse train, prefer `PulseGenerator`. It uses the ESP32 MCPWM
peripheral to generate pulse edges in hardware, without CPU work for each
edge, resulting in more consistent timing and less jitter. CPU callbacks can
observe MCPWM pulse boundaries when needed, but do not generate the waveform.

## Music playback

`TrackStore` mounts LittleFS, lists `.tsq` files, and loads a sequence into
memory only when playback is requested. The `musical` example exposes a track
enum and Play, Pause / Resume, and Stop buttons over BLE. Changing the enum
selects a track without loading or starting it; Stop resets the player and
unloads the sequence. `On time` and `Dead time` tune pulse timing. The
`Track loaded` and `Playing` telemetry values are booleans; together they
distinguish idle, paused, and playing states. `TonePlayer` drives playback with
ESP32 timers and MCPWM. Upload a LittleFS filesystem image containing the
track files to use playback.
