# Dual Linear Actuator Controller with BTS7960 and Arduino Nano

This project controls **two linear actuators** using **two BTS7960 motor drivers** and an **Arduino Nano**, with a **momentary ON-OFF-ON switch** as input. The actuators move with **smooth acceleration/deceleration**, and the second actuator starts **after the first completes**. Direction is saved in **EEPROM**, and **LEDs indicate movement and error states**.

---

## 🚀 Features

- 🕹️ Momentary ON-OFF-ON switch for directional control
- ⚙️ Smooth acceleration and deceleration (0.5s total)
- ➡️ Sequential actuator motion (Motor 1 → Motor 2)
- 🔁 EEPROM memory of last direction (survives reboot)
- 💡 LED indicators for direction and error state
- ⛔ Emergency interrupt: changes direction mid-motion aborts all movement

---

## 🔌 Wiring

### 🧠 Control Inputs

| Pin | Description               |
|-----|---------------------------|
| D2  | MOM switch – Direction 1 (Left) |
| D4  | MOM switch – Direction 2 (Right) |

### ⚙️ Motor Drivers (BTS7960)

**Motor 1:**
| Arduino Pin | BTS7960 Pin |
|-------------|-------------|
| D3          | R_PWM       |
| D5          | L_PWM       |
| D10         | R_EN + L_EN |

**Motor 2:**
| Arduino Pin | BTS7960 Pin |
|-------------|-------------|
| D6          | R_PWM       |
| D9          | L_PWM       |
| D11         | R_EN + L_EN |

### 💡 LEDs

| Pin | Function              | Description               |
|-----|------------------------|---------------------------|
| A0  | Left Direction LED     | Solid ON if last direction was left |
| A1  | Right Direction LED    | Solid ON if last direction was right |
| A2  | Error LED              | ON if movement was interrupted |

Use a 330Ω resistor in series with each LED to GND.

---

## 📦 Hardware Required

- Arduino Nano (or compatible)
- 2x BTS7960 motor drivers
- 2x linear actuators with internal limit switches
- 1x Momentary ON-OFF-ON toggle switch
- 3x 5mm LEDs (Red/Green/Yellow recommended)
- 3x 330Ω resistors
- External power supply for actuators (12V+ depending on actuators)

---

## 🧾 Code Summary

- **State Machine** handles IDLE → Motor1 moving → Motor2 moving
- EEPROM stores last direction (`0 = left`, `1 = right`)
- If switch is flipped during motion, both motors stop and error LED lights up
- LED state is updated based on last direction

---

## 🧠 EEPROM Behavior

- Address `0` stores direction:
  - `0`: Last movement was to the left
  - `1`: Last movement was to the right

---

## 📁 File List

```bash
DualActuatorController/
├── README.md          # Project overview and wiring
└── DualActuator.ino   # Arduino code
