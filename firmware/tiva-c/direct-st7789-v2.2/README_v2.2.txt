Tiva Claw Arm Controller v2.2
================================

IMPORTANT TFT WIRING
--------------------
The ST7789 module labels its SPI clock pin SCL and its SPI data pin SDA.

TFT SCL/CLK -> PB4
TFT SDA/DIN -> PB7
TFT RES     -> PA7
TFT DC      -> PA6
TFT CS      -> PB5
TFT VCC     -> 3.3 V
TFT BLK     -> 3.3 V
TFT GND     -> Tiva C GND

Do not connect PB4 to SDA or PB7 to SCL.

JOYSTICK
--------
VCC -> 3.3 V only
GND -> Tiva C GND
VRx -> PD0
VRy -> PD1

SERVOS
------
Servo A signal  -> PE4
Servo B1 signal -> PE5
Servo B2 signal -> PF3
Servo C signal  -> PF2

Power all four servos from a separate regulated 5 V supply. Connect the
servo-supply ground to Tiva C ground. The signal wires will not work without
this common ground.

DIAGNOSTICS
-----------
1. On reset, the LaunchPad red LED turns on while the TFT initializes.
2. The TFT should show red, green and blue test screens.
3. The red LED then blinks every 500 ms while the main loop runs.

Interpretation:
- No red LED at all: firmware is not running or was not flashed correctly.
- Red LED remains continuously on: execution did not return from Display_Init().
- Red LED blinks but TFT stays blank: TFT wiring, controller type, resolution,
  SPI mode, or panel offsets are incorrect.
- Red LED blinks but servos do not move: verify external 5 V servo power,
  common ground, and servo signal connections.

Version changes
---------------
- Motor initialization moved before display initialization.
- Removed floating-point control calculations.
- Added PF1 heartbeat LED.
- Changed TFT SPI to Mode 0 at 8 MHz.
- Added full ST7789 initialization sequence.
- Added TFT red/green/blue startup test.
- Added explicit ADC configuration and direct integer ADC-to-PWM mapping.

v2.2 compiler correction
------------------------
- Restored #include <stdbool.h> in main.c before driverlib/sysctl.h.
- TivaWare sysctl.h declares functions using bool and therefore requires the
  standard Boolean definition to be visible before that header is parsed.
