# ESP32 BLE Interrupter

This project is a library and set of examples for building your own 
ESP32 wireless interrupter with (optional) BLE remote controllability.

The examples are selected with PlatformIO environments:

- `regular`      — Your classic configurable pulse train using MCPWM.
- `mains-ramped` — zero-cross-triggered pulses for mains ramped QCW coils.
- `musical`      — Musical playback from pre-computed stored tone sequences.

Build an example with `pio run -e regular` (replace `regular` with another
environment). To add a module, create a source file in `src/` and an
environment in `platformio.ini` whose `build_src_filter` selects it.

## BLE and capabilities

`BLEEngine` provides and handles all the underlying BLE connectivity and
comunications layer. A capability is a control variable That gets exposed
to the remote over BLE.
Four control and 3 read-only type and are currently available:

| Type | Class | Description |
| --- | --- | --- |
| Number | `NumberCapability` | A bounded numeric setting. |
| Toggle | `ToggleCapability` | An on/off setting. |
| Button | `ButtonCapability` | A momentary action. |
| Enum | `EnumCapability` | A choice from a set of options. |
| Numeric telemetry | `NumberTelemetry` | A numeric value reported by the ESP32. |
| Boolean telemetry | `BooleanTelemetry` | A true/false value reported by the ESP32. |
| String telemetry | `StringTelemetry` | A text value reported by the ESP32. |

Callbacks are available for individual changes, global changes, and
pre-change safety checks. 
//TODO detailed wiki 

# Generating pulses

`Interrupter` provides GPIO interrupt registration and a hardware timer. Use
these when pulses must be synchronized to an external event, as in
`mains-ramped`. The callbacks run on the CPU in interrupt context, so their
timing includes software and interrupt latency.

For a regular pulse train, prefer `PulseGenerator`. It uses the ESP32 MCPWM
peripheral to generate pulse edges in hardware, without CPU work for each
edge, resulting in near perfect timing and near 0 jitter. CPU callbacks can
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
