# Arduino Linear Actuator Controller (BTS7960 + Hall speed control)

## Overview

This project controls a DC motor or linear actuator using a BTS7960 motor driver, Hall-effect position feedback, current monitoring, travel limits, and status LEDs.

### Features

* Forward and reverse control using two external switches
* Hall sensor position feedback with low-pass filtering
* Analog PWM position output (0–255)
* Adjustable travel limit protection
* Motor current monitoring
* Soft current limiting
* Hard overcurrent shutdown
* Heartbeat LED for system status
* Direction indicator LEDs
* Error LED for fault conditions

---

## Hardware Connections

### Motor Driver (BTS7960)

| Function | Arduino Pin |
| -------- | ----------- |
| LPWM     | D5          |
| RPWM     | D3          |
| Enable   | D10         |

---

### Control Inputs

| Function       | Arduino Pin |
| -------------- | ----------- |
| Forward Switch | D2          |
| Reverse Switch | D4          |

Switches are configured with `INPUT_PULLUP`.

Active state = **LOW**

---

### Hall Sensor

| Function      | Arduino Pin |
| ------------- | ----------- |
| Hall Feedback | A0          |

Measured operating range:

```cpp
hallMin = 180
hallMax = 860
```

Travel stop limits:

```cpp
hallUpLimit = 200
hallDownLimit = 840
```

---

### Current Sense Inputs

| Function            | Arduino Pin |
| ------------------- | ----------- |
| Left Current Sense  | A1          |
| Right Current Sense | A2          |

---

### LEDs

| Function          | Arduino Pin |
| ----------------- | ----------- |
| Forward Indicator | D7          |
| Reverse Indicator | D8          |
| Heartbeat         | D11         |
| Error             | D12         |

---

### PWM Output

| Function            | Arduino Pin |
| ------------------- | ----------- |
| Position PWM Output | D6          |

The Hall sensor position is mapped to:

```cpp
0 → 255 PWM
```

using:

```cpp
map(hallValue, hallMin, hallMax, 0, 255)
```

---

## Operating Logic

### Forward Motion

When the forward switch is pressed:

* BTS7960 is enabled
* LPWM = 200
* RPWM = 0
* Forward LED turns ON

Movement stops automatically when:

```cpp
hallValue <= hallUpLimit
```

---

### Reverse Motion

When the reverse switch is pressed:

* BTS7960 is enabled
* LPWM = 0
* RPWM = 200
* Reverse LED turns ON

Movement stops automatically when:

```cpp
hallValue >= hallDownLimit
```

---

### Simultaneous Button Presses

If both direction switches are pressed:

```cpp
Forward + Reverse = STOP
```

The motor is immediately stopped.

---

## Hall Sensor Filtering

A simple exponential moving average is used:

```cpp
hallFiltered =
hallFiltered +
hallAlpha * (raw - hallFiltered);
```

Current setting:

```cpp
hallAlpha = 0.20
```

This reduces sensor noise and prevents jitter.

---

## Current Monitoring

Current is sampled from both BTS7960 current sense outputs.

### Averaging

Each reading is averaged over:

```cpp
10 samples
```

to reduce noise.

---

### Soft Current Limit

```cpp
currentSoftLimit = 750
```

When exceeded:

* Motor speed is reduced
* PWM drops from 200 to 120

Forward:

```cpp
analogWrite(motorLPWM, 120);
```

Reverse:

```cpp
analogWrite(motorRPWM, 120);
```

---

### Hard Current Limit

```cpp
currentHardLimit = 900
```

When exceeded:

```cpp
currentFault() == true
```

The intention is to trigger a fault shutdown condition.

> Note: The current fault function exists but is not currently called from the main loop. To enable overcurrent shutdown, call `currentFault()` during runtime and set `errorState = true` when a fault occurs.

Example:

```cpp
if (currentFault()) {
    errorState = true;
}
```

---

## Heartbeat LED

Heartbeat indicates normal operation.

Pattern:

* ON for 150 ms
* OFF for 2000 ms

This confirms that the controller is running normally.

---

## Error State

When:

```cpp
errorState == true
```

the controller:

* Stops the motor
* Disables BTS7960
* Turns ON Error LED
* Forces PWM output to 0

The system remains locked in the error state until reset.

---

## Serial Debug Output

The controller outputs diagnostic information at:

```cpp
115200 baud
```

Example:

```text
F=1 B=0 DIR=1
L=432 R=428 MAX=432
```

Where:

* F = Forward command
* B = Reverse command
* DIR = Current motion direction
* L = Left current sense value
* R = Right current sense value
* MAX = Highest measured current

---

## Pin Summary

| Pin | Function            |
| --- | ------------------- |
| D2  | Forward Switch      |
| D3  | BTS7960 RPWM        |
| D4  | Reverse Switch      |
| D5  | BTS7960 LPWM        |
| D6  | Position PWM Output |
| D7  | Forward LED         |
| D8  | Reverse LED         |
| D10 | BTS7960 Enable      |
| D11 | Heartbeat LED       |
| D12 | Error LED           |
| A0  | Hall Sensor         |
| A1  | Current Sense Left  |
| A2  | Current Sense Right |

---

## Future Improvements

* Enable automatic hard-current fault shutdown
* Add fault recovery/reset button
* Implement configurable speed ramping
* Add EEPROM calibration storage
* Add serial commands for tuning limits
* Add watchdog timer protection
* Add position control mode using Hall feedback

---

## License

This project is provided as-is for educational and development purposes. Modify and use freely for your own projects.
