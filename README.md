<div align="center">

# Joystick-Controlled Mechanical Claw Arm

**TM4C123GXL Tiva C · Embedded C · ADC · Hardware PWM · GPIO Interrupts · SysTick · UART · ESP32 · SolidWorks · 3D Printing**

A real-time embedded-control and mechatronic integration project featuring a four-servo mechanical claw arm, single-joystick operation, a custom 3D-printed controller enclosure, and command-based graphical feedback.


</div>

---

## Project Overview

Developed as a **Group 3 project for MSE 450: Real-Time and Embedded Control Systems at Simon Fraser University (Summer 2026)**, this prototype integrated low-level microcontroller programming, analog sensing, multi-servo actuation, serial communication, mechanical assembly, and test-driven troubleshooting.

A **TM4C123GXL Tiva C LaunchPad** sampled a **KY-023 two-axis joystick** and commanded four **SG90 servos**: one for base rotation, two mechanically opposed servos for shoulder elevation, and one for opening or closing the claw. The joystick pushbutton toggled claw state. The project also investigated a graphical representation of **commanded** joint positions and claw state.

**Demonstrated outcomes:** base rotation, claw actuation, joystick-command mapping, and graphical feedback. Shoulder movement worked during integration, but **both shoulder servos failed near the end of the project and were not operational in the final hardware demonstration**. The system was designed as an **embedded-control demonstrator**, not a validated payload-handling manipulator.

<p align="center">
  <img src="media/Claw%20Arm%20-%20Isometric.jpg" alt="3D-printed mechanical claw arm prototype with SG90 servos on a laboratory bench" width="680" />
</p>

### Project at a Glance

| Subsystem | Implementation |
|:--|:--|
| **Primary controller** | Texas Instruments TM4C123GXL Tiva C LaunchPad, 80 MHz system-clock configuration |
| **User input** | KY-023 joystick: two analog axes and an active-low pushbutton |
| **Actuation** | Four SG90 servos: base, opposed shoulder pair, and claw |
| **Servo interface** | Hardware PWM at 50 Hz; approximately 0.6–2.4 ms commanded pulses in the archived firmware |
| **Embedded peripherals** | 12-bit ADC, PWM1, GPIO interrupt/debounce, SysTick, SSI2/SPI; UART in the reported final display architecture |
| **Visual feedback** | Initially direct-drive 1.69-inch ST7789 TFT; later a separate ESP32-integrated 2.8-inch TFT receiving Tiva C command data over UART |
| **Fabrication** | 3D-printed robotic arm and custom SolidWorks-designed controller enclosure; assembly, wiring, and soldering |
| **Public artifacts** | Archived Tiva C v2.2 direct-ST7789 firmware, STL exports, project photography, and engineering documentation |

## System Architecture

The final reported arrangement separated the motion controller from the graphical display:

```text
         KY-023 joystick
      X axis / Y axis / button
                  |
          ADC0 + GPIO interrupt
                  v
       TM4C123GXL Tiva C
       |                 |
       | PWM, 50 Hz      | UART: commanded joint/claw state
       v                 v
  Four SG90 servos   ESP32-integrated 2.8-inch TFT
  - Base rotation    - Command-derived angle readouts
  - Shoulder B1/B2   - Arm/claw visualization
  - Claw open/close
       ^
       |
 Regulated external 5 V servo supply
 (common ground with Tiva C)
```

**Source availability distinction:** The included **v2.2 firmware uses the *earlier* direct-ST7789 SSI2/SPI architecture**, not the final UART-to-ESP32 arrangement shown above. The final UART transmitter and ESP32 receiver/display source files were not recovered from the laboratory computers. The final architecture is documented from the group report; it is **not directly reproducible from the included firmware alone**.

For a breakdown of peripherals, signal paths, and implementation boundaries, see [System Architecture](docs/SYSTEM_ARCHITECTURE.md) and [Display Integration](docs/DISPLAY_INTEGRATION.md).

## Embedded Control

The archived Tiva C v2.2 source illustrates peripheral-level embedded control with a modular `main.c` / `motor.c` / `display.c` design.

| Control path | Behavior documented in the firmware |
|:--|:--|
| Joystick X | `PD0 / AIN7` → ADC → base Servo A, `PE4 / M1PWM2` |
| Joystick Y | `PD1 / AIN6` → ADC → shoulder Servo B1, `PE5 / M1PWM3`, and complementary Servo B2, `PF3 / M1PWM7` |
| Pushbutton | `PF4` falling-edge GPIO interrupt → 50 ms software debounce → claw-state toggle |
| Claw output | Servo C, `PF2 / M1PWM6` → one of two commanded positions |
| Timing | SysTick-based millisecond counter; 50 ms nominal loop delay in the archived main loop |
| Diagnostics | Onboard PF1 red LED heartbeat and display startup diagnostics |

With the 80 MHz system clock divided by 64 for PWM, the configured timer clock is **1.25 MHz**. A **25,000-count** PWM period corresponds to **20 ms (50 Hz)**. In v2.2, the nominal pulse mapping is:

```text
pulse_ticks = 750 + ((3000 - 750) × ADC_value) / 4095
shoulder_B2_input = 4095 - shoulder_B1_input
```

This is a **command mapping**, not verified physical joint-angle measurement or a calibrated safe-motion envelope for all servos.

### Firmware Development History

| Revision | Engineering change | Status in this repository |
|:--|:--|:--|
| **v2.0** | Integrated opposed shoulder pair; float-based percentage-to-PWM commands; direct ST7789 interface using SPI Mode 3 at 16 MHz | Historical reference, not included |
| **v2.1** | Integer ADC-to-PWM control, initialization/heartbeat diagnostics, more complete ST7789 setup, SPI Mode 0 at 8 MHz | Historical reference, not included |
| **v2.2** | Retained v2.1 architecture and corrected `<stdbool.h>` inclusion for TivaWare header compatibility | **Included archived source** |
| **Final demonstration** | Retained Tiva C motion control; moved graphical rendering to ESP32-integrated TFT via UART after original display failure | **Documented; final firmware unavailable** |

**Explore:** [`firmware/tiva-c/direct-st7789-v2.2/`](firmware/tiva-c/direct-st7789-v2.2/) · [Embedded Control Notes](docs/EMBEDDED_CONTROL.md)

The `ccs-project/` subfolder preserves available Code Composer Studio project metadata and linker configuration. The vendor-supplied TI startup source is retained locally but excluded from this public repository. It is **archival**: source paths, toolchain prerequisites, TivaWare dependencies, and successful import/build have **not been independently verified** in this rearranged repository structure. Third-party notices and usage restrictions remain applicable.

## Mechanical Design & Fabrication

The arm combines a rotating base, paired shoulder linkages, and a two-finger gripper. The team assembled the printed structure with SG90 servos and tested actuator responses incrementally. A separate **SolidWorks-designed controller enclosure** was developed to integrate the KY-023 joystick and the original smaller TFT.

<details>
<summary><strong>View the physical arm and controller enclosure</strong></summary>

<p align="center">
  <img src="media/Claw%20Arm%20-%20Side%20View.jpg" alt="Side view of the printed claw-arm linkages and servos" width="490" />
</p>

<p align="center">
  <img src="media/Joystick%20Interface%20-%20CAD.jpg" alt="CAD view of the joystick and original display enclosure" width="680" />
</p>

<p align="center">
  <img src="media/Joystick%20Interface%20-%20Internal.jpg" alt="Joystick mounted inside the 3D-printed enclosure" width="450" />
</p>

<p align="center">
  <img src="media/Joystick%20Interface%20-%20Assembled.jpg" alt="Assembled controller enclosure for the original display arrangement" width="450" />
</p>

</details>

**Printable geometry:** [`cad/mechanical-arm/`](cad/mechanical-arm/) contains **35 STL exports** for the mechanical arm assembly, while [`cad/controller-enclosure/`](cad/controller-enclosure/) contains `Front Plate v2.STL` and `Rear Case v2.STL`. The generically named `Boss-Extrude*.STL` parts retain their export filenames; an independently verified assembly/print sequence and individual part mapping are not supplied.

**Design limitation:** The printed UI enclosure was sized for the original 1.69-inch display and **did not fit the larger 2.8-inch replacement TFT** after the display architecture changed.

See [Mechanical Design](docs/MECHANICAL_DESIGN.md).

## Display Redesign & Integration

The initial graphical user interface was written for the Tiva C to operate a **240 × 280 ST7789 TFT** directly over SSI2/SPI. The archived firmware contains startup colour screens, basic graphical primitives, font rendering, integer trigonometric lookup values, and an oblique arm rendering derived from joystick commands.

After the original TFT failed, the team adopted a **240 × 320 ESP32-integrated 2.8-inch display**. The Tiva C remained the motion controller; UART transmitted joint-command and claw-state information to the ESP32 for rendering. This adaptation recovered the project's visual-feedback objective but required a display arrangement different from the printed enclosure.

**Note:** Values shown on either display represent *command-derived positions*, **not closed-loop measurements from encoders or servo feedback**. See [Display Integration](docs/DISPLAY_INTEGRATION.md).

## Testing, Results & Limitations

The project was evaluated through staged bench testing, including startup checks, analog joystick response, PWM servo actuation, complementary shoulder motion, claw-button behavior, display feedback, and integration troubleshooting.

| Capability | Recorded outcome |
|:--|:--|
| Base rotation | Successfully demonstrated |
| Claw actuation and pushbutton control | Successfully demonstrated |
| Single-joystick input mapping | Successfully demonstrated |
| Graphical feedback | Successfully demonstrated after the ESP32 display redesign |
| Shoulder movement | Worked during integration; both shoulder servos failed before the final demonstration |
| Payload lifting and grip force | Not validated |
| Measured joint-angle feedback | Not implemented; displayed angles were command-derived |
| Quantified response time, accuracy, or endurance | No independently verified metrics provided |

These limitations are part of the engineering outcome rather than omitted failures. See [Testing and Limitations](docs/TESTING_AND_LIMITATIONS.md).

## Repository Contents

```text
joystick-controlled-claw-arm/
├── README.md
├── .gitignore
├── cad/
│   ├── controller-enclosure/        # Front Plate v2 and Rear Case v2 STL exports
│   └── mechanical-arm/             # 35 printable arm STL exports
├── docs/
│   ├── SYSTEM_ARCHITECTURE.md
│   ├── EMBEDDED_CONTROL.md
│   ├── DISPLAY_INTEGRATION.md
│   ├── MECHANICAL_DESIGN.md
│   └── TESTING_AND_LIMITATIONS.md
├── firmware/
│   ├── tiva-c/
│   │   └── direct-st7789-v2.2/
│   │       ├── main.c
│   │       ├── motor.c
│   │       ├── motor.h
│   │       ├── display.c
│   │       ├── display.h
│   │       ├── README_v2.2.txt
│   │       └── ccs-project/        # Archived CCS metadata and linker file
└── media/
    ├── Claw Arm - Isometric.jpg
    ├── Claw Arm - Side View.jpg
    ├── Joystick Interface - CAD.jpg
    ├── Joystick Interface - Internal.jpg
    └── Joystick Interface - Assembled.jpg
```

> The final ESP32 display source was not recovered and is not included. The TI startup source is excluded from version control and retained only in the local development archive.

## Technical Documentation

| Document | Contents |
|:--|:--|
| [System Architecture](docs/SYSTEM_ARCHITECTURE.md) | Hardware blocks, power, joystick/servo connections, final signal flow |
| [Embedded Control](docs/EMBEDDED_CONTROL.md) | ADC, PWM, interrupts, SysTick, firmware revision progression |
| [Display Integration](docs/DISPLAY_INTEGRATION.md) | Direct ST7789 interface and ESP32/UART recovery |
| [Mechanical Design](docs/MECHANICAL_DESIGN.md) | Linkage, CAD enclosure, STL inventory, additive manufacturing |
| [Testing & Limitations](docs/TESTING_AND_LIMITATIONS.md) | Verification approach, observed failures, demonstrated capabilities |

## Collaboration, Source Provenance & Reuse

This was a **team project** for **SFU MSE 450: Real-Time and Embedded Control Systems**, Group 3. The final report credits **Santiago Fabian Huarcaya Chavez, Hasan Fairuz, and Ali Shahryar Syed**. Unless otherwise established, the project outcomes are described as **team achievements**.

The public repository is a curated portfolio record, **not a turnkey, independently verified build**. The full project report is not reproduced here because it contains student identifiers. The available v2.2 source corresponds to an **earlier display revision**, and the final UART/ESP32 firmware was not recovered.

The Tiva C project relies on the external **TI TivaWare/Code Composer Studio** toolchain. The vendor startup source is not included in the public repository; external TI libraries and tooling remain subject to their applicable license terms. Public availability of this repository does not itself grant a blanket reuse license. The robot-arm geometry may also have upstream design provenance that should be checked before redistribution or reuse.

## Portfolio

**Hasan Fairuz** — Mechatronic Systems Engineering

[Engineering Portfolio](https://hasanfairuz.com/) · [GitHub Profile](https://github.com/HasanFairuz)
