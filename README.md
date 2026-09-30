# STM32F401RE I2C Master-Slave Communication

Bare-metal register-level implementation of I2C communication between two STM32F401RE (NUCLEO-F401RE) microcontrollers.

---

## Overview

- **Master (`master.c`)**: Pressing the on-board blue user button **B1 (PC13)** triggers an External Interrupt (**EXTI13**). The master transmits the message string `"TURN LED ON"` over **I2C1** to slave address `0x32`.
- **Slave (`slave.c`)**: Listens on **I2C1** (7-bit address `0x32`). When `"TURN LED ON"` is received, it toggles the on-board Green LED **LD2 (PA5)**.

---

## Hardware Configuration & Pinout

| Signal | Master Pin (Nucleo) | Slave Pin (Nucleo) | Notes |
|---|---|---|---|
| **I2C1 SCL** | `PB8` (D15) | `PB8` (D15) | Open-drain, AF4 |
| **I2C1 SDA** | `PB9` (D14) | `PB9` (D14) | Open-drain, AF4 |
| **GND** | `GND` | `GND` | Common ground reference |
| **Pull-ups** | 4.7 kΩ to +3.3V | 4.7 kΩ to +3.3V | One pair on SCL/SDA lines |

- **User Button (Master)**: `PC13` (active low, falling-edge trigger on EXTI13)
- **Status LED (Slave)**: `PA5` (LD2 on Nucleo-F401RE, push-pull output)
- **HSI Clock**: Default 16 MHz internal RC oscillator without PLL.
- **I2C Speed**: ~50 kHz standard mode.

---

## File Structure

```text
├── master.c   # I2C master with EXTI push-button trigger
├── slave.c    # I2C slave with string command parsing and LED toggle
└── README.md  # Project documentation and wiring guide
```

---

## How to Build in STM32CubeIDE

1. Create a new STM32 project selecting **STM32F401RETx** (or **NUCLEO-F401RE**).
2. Replace `Core/Src/main.c` with either `master.c` or `slave.c`.
3. Build and flash onto the respective Nucleo board.
