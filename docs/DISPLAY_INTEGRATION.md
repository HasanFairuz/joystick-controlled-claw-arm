# Display Integration — ST7789 Development & ESP32/UART Recovery

[← Project overview](../README.md) · [System architecture](SYSTEM_ARCHITECTURE.md) · [Embedded control](EMBEDDED_CONTROL.md)

## Objective and limits

The interface was intended to show **commanded** arm orientation and claw state during joystick operation. These graphics were **not** independent measurements of servo shaft position. The team implemented two distinct architectures during the project: direct ST7789 control from the Tiva C and, following a hardware failure, an ESP32-integrated TFT receiving information over UART.

## Earlier architecture: Tiva C directly driving ST7789

The original display was a **1.69-inch, 240 × 280 ST7789**. Archived [`display.c`](../firmware/tiva-c/direct-st7789-v2.2/display.c) contains the display init sequence and pixel rendering logic; [`main.c`](../firmware/tiva-c/direct-st7789-v2.2/main.c) invokes the renderer. The interface used **SSI2/SPI** with SCL/CLK on PB4, SDA/DIN on PB7, CS on PB5, DC on PA6 and reset on PA7. The original TFT power/logic arrangement was 3.3 V.

The v2.2 code configures SSI2 for **SPI Mode 0 at 8 MHz**. Its display driver supplies a compact font, filled primitive drawing, integer sine lookup values, an oblique projected robot-arm illustration, a heading indicator, claw animation and command-derived angular labels. Diagnostic startup screens in red, green and blue aided fault isolation.

The arm illustration uses simplified geometric and rendering parameters; it is **not a geometrically calibrated digital twin** or verified physical position sensor.

## Hardware failure and final redesign

Late in integration, the original TFT stopped working. Rather than rebuilding the same direct display path, the group switched to a **2.8-inch, 240 × 320 TFT module with an integrated ESP32**. The Tiva C remained responsible for joystick sampling, PWM and claw logic. It **sent joint-command and claw-state information over UART**; the ESP32 rendered the display locally. Touch input was not required.

```text
KY-023 joystick ──> Tiva C (ADC / GPIO / PWM) ──> Four servos
                           |
               UART: command-derived state
                           v
                 ESP32-integrated TFT
                    visual feedback
```

The final Tiva UART transmitter code, ESP32 receiver/renderer code, exact UART pin allocation, baud rate, and serial framing are **not available** in the recovered source set. Therefore the final display cannot be rebuilt from this GitHub repository alone. Do not substitute the archived SPI source for the missing UART implementation.

## Mechanical interface mismatch

The team had already designed and printed the joystick enclosure around the **smaller ST7789**. After the change, the larger ESP32-equipped TFT did **not** fit the original housing, and a revised enclosure was not completed.

![SolidWorks CAD view of original controller enclosure](../media/Joystick%20Interface%20-%20CAD.jpg)

![Original controller enclosure with small display](../media/Joystick%20Interface%20-%20Assembled.jpg)

## Design lessons and future work

The display migration demonstrates a **fault-driven architectural adaptation** that preserved the graphical-feedback objective. Its trade-offs were incomplete physical packaging, unrecovered reproducibility details and lack of quantitative display latency measurements. A subsequent version could define a framed UART protocol with checksums, separate control from display scheduling, record serial/display latency, and redesign the enclosure around the replacement TFT.

**Sources:** MSE 450 Group 3 Final Project Report, especially display implementation/integration sections; archived v2.2 `display.c`, `main.c`, and version README. See [Testing and Limitations](TESTING_AND_LIMITATIONS.md).
