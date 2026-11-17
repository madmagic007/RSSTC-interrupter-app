# ESP32 BLE Interrupter library

This Arduino library provides building blocks for ESP32 interrupters that are
configured remotely over Bluetooth Low Energy (BLE):

- `Interrupter` manages registered capabilities, starts the BLE service, and
  provides GPIO-interrupt and hardware-timer helpers.
- `Capability` is the abstract base for named settings and change handling.
- `NumberCapability` adds a value range; it is in
  `lib/Interrupter/src/capabilities/NumberCapability.h`.
- `ToggleCapability`, `ButtonCapability`, and `EnumCapability` provide
  capability-specific interfaces in `lib/Interrupter/src/capabilities/`.
- `TelemetryValue`, a report-only value separate from capabilities, lives in
  `lib/Interrupter/src/telemetry/`.
- `PulseGenerator` uses the ESP32 MCPWM peripheral to generate a pulse train.

The library handles exposing settings and receiving BLE updates. Your firmware
defines what each setting means and how the ESP32 produces the interrupter
signal.

## Remote configuration with capabilities

Capabilities are named controls with a current value. `NumberCapability`
provides inclusive minimum and maximum bounds; toggle and button capabilities
do not have ranges, and enum capabilities define their selectable options.
Registering a capability with an `Interrupter` exposes it through that module's
BLE service. Another BLE device connects and sends new values; the library
validates and stores accepted updates, then reports changed values over BLE.

Create an `Interrupter`, declare and register the capabilities your firmware
needs, configure its signal-generation hardware, then call `begin()` with the
BLE device name. Keep the objects alive for the lifetime of the firmware:

```cpp
#include "Interrupter.h"

static Interrupter interrupter;
static NumberCapability pulseWidth("Pulse width", 0, 0, 200);

void setup() {
    if (!interrupter.registerCapability(pulseWidth)) {
        Serial.println("Could not register pulse width");
        return;
    }

    // Configure the signal-generation hardware here.
    interrupter.begin("My Interrupter");
}

void loop() {
    delay(1000);
    interrupter.sendHeartbeat();
}
```

The initial value is used at startup. Connecting to BLE does not automatically
change it or start output. The remote device is responsible for sending any
updates.

## BLE request and response protocol

The device does not send definitions or telemetry snapshots automatically on
connect. The host should subscribe to the relevant notification characteristics,
then write requests to the host-requests characteristic:

| UUID | Purpose |
|---|---|
| `00000000-0000-0000-0000-000000000000` | Interrupter service; included in advertising. |
| `11111111-1111-1111-1111-111111111111` | Capability definitions; one JSON notification per registered capability, in response to the `caps` request. |
| `22222222-2222-2222-2222-222222222222` | Device reports; capability value changes and the `hb` heartbeat. |
| `33333333-3333-3333-3333-333333333333` | Control writes; host sends capability updates as a JSON object, for example `{"BPS":20}`. |
| `44444444-4444-4444-4444-444444444444` | Telemetry value updates and snapshots. |
| `55555555-5555-5555-5555-555555555555` | Telemetry definitions; one JSON notification per registered telemetry value, in response to the `telemetry` request. |
| `66666666-6666-6666-6666-666666666666` | Host requests; write the exact text `caps` or `telemetry`. |

Requests and control updates use separate write characteristics, so ordinary
capability updates are sent directly to the JSON update handler without
checking for request command strings. A `caps` request returns capability
metadata, including each capability's current value. A `telemetry` request
returns the telemetry definitions and then a current-value snapshot on the
telemetry-updates characteristic. Later calls to `reportTelemetry()` send
updates on that same characteristic. A device-side capability change is
reported on the device-reports characteristic.

Definition responses and telemetry snapshots are notifications, not a single
aggregate JSON document. The host must enable notifications before writing a
request; each record is a separate JSON object. Definition records have
`"kind":"definition"` and are followed by an end record with `"kind":"end"`
and a `count`. A `telemetry` request then sends the current-value snapshot on
the telemetry-updates characteristic, ending with
`"kind":"snapshot_end"` and a `count`. These markers let the host know when
each response batch is complete, including when it is empty.

Capability definitions include `name`, `type`, and `value`, plus `min`/`max`
for ranged capabilities. Telemetry definitions include `name` and `type` (and
optional enum `options`). Capability reports and telemetry value records have
`"kind":"value"`, `name`, `type`, and `value`. Ongoing telemetry updates use
the same value-record shape as the requested snapshot.

## Producing signals

### MCPWM with `PulseGenerator`

Use `PulseGenerator` for a periodic pulse train when the rate and pulse width
are known between updates. The MCPWM peripheral generates the edges in
hardware, so the CPU does not need to service an interrupt for every edge.
This makes `PulseGenerator` the preferred option when repeatable pulse timing
is important: its edges are generated by hardware without per-pulse CPU or
interrupt-service latency, so it has substantially less jitter than toggling
GPIO from a timer callback. At the configured 1 MHz timer resolution, pulse
width and period changes have 1 microsecond steps. This is 1-microsecond
resolution, not 0.01-microsecond accuracy; actual clock accuracy depends on
the ESP32 clock source.

The GPIO-and-timer approach schedules an interrupt, then executes code to
change the pin. Interrupt and execution latency vary with system activity, so
edge timing can be off by more than 10 microseconds under load. Treat that as
possible software-timing jitter, not a fixed error bound; use MCPWM when such
jitter is unacceptable.

`src/regular.cpp` demonstrates binding BLE capabilities directly to generator
methods:

```cpp
#include "Interrupter.h"
#include "PulseGenerator.h"

static Interrupter interrupter;
static PulseGenerator pulseGen;
static NumberCapability pulseWidth(
    "Pulse width", 0, 0, 200, pulseGen, &PulseGenerator::setOnTimeUs);
static NumberCapability bps(
    "BPS", 16, PulseGenerator::MIN_BPS, 500,
    pulseGen, &PulseGenerator::setBps);

void setup() {
    interrupter.registerCapability(pulseWidth);
    interrupter.registerCapability(bps);

    pulseGen.begin(18, bps.value, pulseWidth.value, bps.max, pulseWidth.max);
    interrupter.begin("Regular Interrupter");
}
```

This example starts with zero pulse width; set a nonzero width over BLE to
start output. MCPWM's 16-bit period limits the rate to 16 BPS or higher at.
`PulseGenerator::begin()` and its update methods validate their arguments;
check their `bool` results in production firmware.

### GPIO interrupt and hardware timer

Use GPIO and timer callbacks when signal timing depends on an external event or
requires custom sequencing. `Interrupter` provides
`registerGpioInterrupt(pin, mode, callback)`, `registerTimer(frequencyHz,
callback)`, `setTimerAlarm(microseconds)`, and `resetTimer()`.

`src/mains-ramped.cpp` demonstrates this approach: a rising edge on GPIO7
signals a zero crossing; after the configured number of edges are skipped, a
hardware timer schedules the delay and pulse. The timer callback drives GPIO18
high and then low after the configured pulse width. The timer is registered at
1 MHz so its alarm values correspond to microseconds.

GPIO and timer callbacks run in interrupt context. Keep them short and
IRAM-safe; do not call BLE or blocking functions there. Check the `bool`
results from registration and timer methods.

## Validation, value updates, and callbacks

### Built-in range and custom validation

`NumberCapability::validate()` accepts values within the inclusive minimum
and maximum. Before validation, the library clamps an incoming number to that
range. Override `validate()` in a `NumberCapability` subclass to apply
additional rules to the clamped value or reject it:

```cpp
class EvenCapability : public NumberCapability {
public:
    using NumberCapability::NumberCapability;

    bool validate(uint32_t candidate) const override {
        return NumberCapability::validate(candidate) && candidate % 2 == 0;
    }
};

static EvenCapability divider("Divider", 2, 0, 20);
```

For example, an update of 25 is first clamped to 20, then accepted by this
validator; an update that clamps to an odd value is rejected. `Capability` is
an abstract common base and should not be instantiated directly. Toggle and
button capabilities derive directly from it and have no numeric range.

### Updating values in firmware

Prefer `getCapabilityValue(name, value)` and
`setCapabilityValue(name, value)` to access capabilities in application code.
`setCapabilityValue()` goes through the same range and custom validation as a
BLE update. Avoid assigning directly to the public `value` field: doing so
bypasses validation, change callbacks, and BLE value reporting.

### Responding to accepted changes

A callback runs only when a validated setting changes. Bind a capability to a
helper object's member function to apply the new value:

```cpp
static PulseGenerator pulseGen;
static NumberCapability pulseWidth(
    "Pulse width", 0, 0, 200, pulseGen, &PulseGenerator::setOnTimeUs);
```

Callbacks with either `void` or `bool` return types are supported. A `bool`
result is discarded; it does not reject or roll back the already accepted
capability value, and it is not returned to the BLE sender. Check hardware
application results separately if they need to be handled.

Alternatively, `setCapabilityChangeCallback(callback, context)` installs one
application-wide callback. When installed, it replaces each capability's
individual callback. The `Interrupter`'s own BLE change reporting still occurs.

## Build and flash firmware

Each PlatformIO environment selects one firmware entry point in `src/`, so
different ESP32 boards can run different interrupter modules. The shared
library and common build settings are reused. The current environments are:

- `regular` — `src/regular.cpp`, MCPWM pulse train.
- `mains-ramped` — `src/mains-ramped.cpp`, zero-cross GPIO interrupt and
  hardware-timer pulse sequence.

Build or flash an environment explicitly (`mains-ramped` is the default):

```sh
pio run -e regular
pio run -e regular -t upload

pio run -e mains-ramped
pio run -e mains-ramped -t upload
```

To create another module, add a `.cpp` entry point to `src/`, declare and
register its capabilities, implement its signal-generation behavior, and call
`begin()`. Add a PlatformIO environment with a `build_src_filter` that selects
only that entry point.
