
---

## 📦 **Components**

| Qty | Component                |
|----:|-------------------------|
| 1  | Arduino Nano             |
| 2  | BTS7960 motor driver     |
| 2  | AD5171 digital potentiometer |
| 1  | DPDT momentary-off-momentary switch |
| 1  | Throttle twist pot (0–4V)|
| 2  | DC motors (up to ~43A with BTS7960)|
| –  | Jumper wires, power supply |

---

## 🧰 **How to use**

* Upload the Arduino sketch.
* Connect throttle to A0.
* Connect DPDT momentary-off-momentary switch to D2 (left) and D4 (right).
* Press & release:
  * Left → run sequence forward.
  * Right → run sequence backward.
* Tap again while running → stop immediately.
* Tap again (same side) → resume.
* Tap again (other side) → restart in other direction.

---

## ✏️ **Configuration**

In the sketch:

```cpp
unsigned long motor1_time = 3000; // first motor runtime in ms
unsigned long motor2_time = 5000; // second motor runtime in ms
const int pwmValue = 200;         // PWM power (0–255)
