# System Architecture — Joystick-Controlled Mechanical Claw Arm

[← Project overview](../README.md) · [Embedded control](EMBEDDED_CONTROL.md) · [Display integration](DISPLAY_INTEGRATION.md)

## Project scope

This **SFU MSE 450 (Summer 2026), Group 3** project integrated a single-joystick interface, a Tiva C embedded controller, four SG90 actuators, 3D-printed hardware, and graphical command feedback. It was a classroom **real-time embedded-control demonstrator**; it was not validated as a payload-handling manipulator. The final arrangement described here is based on the team's project report. The available v2.2 firmware instead implements the earlier direct-ST7789 configuration.

## Reported final architecture

```text
KY-023 joystick: VRx, VRy, pushbutton
          |          |
     ADC0 inputs   GPIO PF4 falling-edge interrupt
          \          /
        TM4C123GXL Tiva C LaunchPad
          |                         |
   50 Hz PWM outputs          UART joint-command /
          |                   claw-state messages
  A (base), B1/B2                 |
 (opposed shoulder), C      ESP32-integrated TFT
         (claw)             2.8 in, 240 x 320
          ^
 External regulated 5 V supply
 with common Tiva C ground
```

The ESP32 module was the **display processor**, not the primary motion controller. The project report does not establish a reproducible UART protocol specification or provide the final transmitter/receiver code.

## Hardware inventory

| Hardware | Role |
|---|---|
| TM4C123GXL Tiva C LaunchPad | ADC acquisition, servo commands, pushbutton interrupt, timing and final UART transmission |
| KY-023 two-axis joystick | Analog X and Y inputs plus active-low pushbutton |
| Four SG90 servos | A: base; B1/B2: mechanically opposed shoulder pair; C: claw |
| Regulated 5 V external supply | Powers servo subsystem; ground shared with controller |
| 1.69-inch 240 × 280 ST7789 TFT | Earlier direct SSI2/SPI display; later failed |
| 2.8-inch 240 × 320 ESP32-integrated TFT | Replacement UART-connected graphical display |
| 3D-printed structure and two-piece UI housing | Mechanical arm, joystick support and original display housing |

## Pin mapping for available v2.2 firmware

| Signal | Tiva pin/peripheral | Use |
|---|---|---|
| Joystick X | PD0 / AIN7 | Base command |
| Joystick Y | PD1 / AIN6 | Shoulder command |
| Joystick switch | PF4, internal pull-up | Interrupt-driven claw toggle |
| Servo A | PE4 / M1PWM2 | Base rotation |
| Servo B1 | PE5 / M1PWM3 | Shoulder |
| Servo B2 | PF3 / M1PWM7 | Opposed shoulder command |
| Servo C | PF2 / M1PWM6 | Claw |
| Status LED | PF1 | Initialization and heartbeat |
| Direct display SPI clock/data | PB4 / PB7 via SSI2 | **Earlier firmware only** |
| Direct display CS/DC/reset | PB5 / PA6 / PA7 | **Earlier firmware only** |
| UART to ESP32 | Not recoverable as an exact pin/baud configuration | **Reported final system only** |

The earlier TFT operated at **3.3 V logic**. The servo power rail was separate; its **ground had to be connected to Tiva C ground** for the servo signal references to work. Do not power four servos from the LaunchPad 3.3 V output. Servo end stops and safe travel limits require calibration before anyone attempts to reproduce this setup.

## System boundaries

- The system used **command-derived** displayed angles, not encoder or closed-loop measurements.
- The published archived C source does not implement the final ESP32/UART architecture.
- No final wiring schematic, measured latency, exact UART framing, or validated electrical power budget is supplied.
- Both shoulder servos failed late in the project; operation of that joint was not available for the final demonstration.

**Sources:** MSE 450 Group 3 Final Project Report (9 August 2026), especially the architecture and I/O sections; archived `main.c`, `motor.c`, `display.c`, and `README_v2.2.txt`. See [testing boundaries](TESTING_AND_LIMITATIONS.md).
