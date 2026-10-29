# Embedded Control — ADC, PWM, Interrupts & Firmware Revisions

[← Project overview](../README.md) · [System architecture](SYSTEM_ARCHITECTURE.md) · [Testing](TESTING_AND_LIMITATIONS.md)

## Source-code status

The public code is an **archived Tiva C direct-ST7789 v2.2 development revision**, stored at [`firmware/tiva-c/direct-st7789-v2.2/`](../firmware/tiva-c/direct-st7789-v2.2/). It should **not** be mistaken for the final UART-to-ESP32 firmware used after the original TFT failed. Its build has not been independently reproduced after rearranging the files for this portfolio.

| Module | Responsibility |
|---|---|
| `main.c` | Clock initialization, ADC sampling, command mapping, servo updates, display frame call, diagnostic heartbeat |
| `motor.c` / `motor.h` | Register-level ADC, PWM1, GPIO interrupt, SysTick and motion interface |
| `display.c` / `display.h` | ST7789 SSI2/SPI initialization, graphics primitives and command-derived arm drawing |
| `README_v2.2.txt` | Early wiring, diagnostics and revision notes |
| `ccs-project/` | Archived CCS metadata, linker and TI startup material; not verified as directly importable in this rearranged layout |

## Analog acquisition and control mapping

The KY-023's analog outputs are sampled by **ADC0**, using **PD0/AIN7** and **PD1/AIN6**. The configured sample sequencer is software-triggered; readings are represented as **12-bit values from 0 to 4095**. The code maps joystick X to Servo A (base) and joystick Y to the B1/B2 shoulder pair.

The v2.2 firmware converts raw ADC input directly to pulse width with integer arithmetic:

```text
PWM clock: 80 MHz / 64 = 1.25 MHz
Servo period: 25,000 ticks / 1.25 MHz = 20 ms (50 Hz)
Pulse ticks: 750 + ((3000 - 750) × ADC) / 4095
Pulse width: about 0.6 ms to 2.4 ms
Servo B2 ADC input: 4095 - Servo B1 ADC input
```

The complementary mapping is intended to coordinate the two shoulder servos mounted in opposite directions. It is a **command relationship**, not evidence of calibrated position feedback or torque balance. The indicated pulse-width endpoints may mechanically overload a particular servo if not calibrated.

## Claw pushbutton and timing

The joystick switch connects to **PF4** with a pull-up and falling-edge GPIO interrupt. The ISR uses a SysTick millisecond counter to reject an additional edge received within **50 ms** of the previous accepted press. An accepted press toggles the claw-state flag; the main loop sets Servo C to the corresponding endpoint. The code is a basic debounce strategy, not a tested guarantee against every contact pattern.

SysTick is configured for an approximately **1 ms** increment at the 80 MHz system clock. `main.c` requests a **50 ms delay per iteration**, establishing a theoretical maximum **20 supervisory iterations per second before computation and drawing overhead**. The independent hardware PWM waveform still runs at **50 Hz**. No instrumented control-loop jitter or latency results are available.

The v2.2 code initializes the motion subsystem ahead of the direct TFT and uses the LaunchPad **PF1 red LED** to aid diagnosis of display initialization and main-loop activity. It also performs TFT colour-screen startup checks.

## Firmware history

| Version | Distinguishing implementation | Interpretation |
|---|---|---|
| **v2.0** | Added complementary B1/B2 control; floating-point percentage-to-PWM conversion; ST7789 SPI Mode 3 at 16 MHz | Earlier development archive; not published here |
| **v2.1** | Integer ADC-to-pulse calculations; diagnostic heartbeat; display initialization changes; SPI Mode 0 at 8 MHz | Earlier development archive; not published here |
| **v2.2** | Retained v2.1 design; restored `<stdbool.h>` ahead of the TivaWare header in `main.c` | **Published source revision** |
| **Final course demonstration** | Tiva C performed motor control, and a separate ESP32-integrated TFT received display information over UART | Described in the report; final source not recovered |

These are code revisions, **not a claim that every revision was successfully built and demonstrated on final hardware**. In v2.0, servo commands were percentages converted with `float`; v2.1/v2.2 use integer conversion from ADC counts. The v2.1 `main.c` lacks the `<stdbool.h>` include restored in v2.2.

## Reproducibility and dependencies

The uploaded sources reference TI-specific `driverlib` and device-register headers, requiring a compatible **Code Composer Studio/TivaWare** setup. CCS `.project`, `.cproject`, and `.ccsproject` files may still contain original relative paths; the archived `ccs-project/` directory is for provenance, not a promise of a one-click build. The TI startup file has its own license conditions, which must be respected. Do not infer a usable final ESP32 serial implementation from the earlier SPI drivers.

**Sources:** Team report; supplied v2.0, v2.1 and v2.2 firmware and revision READMEs. Related material: [Display Integration](DISPLAY_INTEGRATION.md).
