# Mechanical Design — Printed Claw Arm & Controller Enclosure

[← Project overview](../README.md) · [System architecture](SYSTEM_ARCHITECTURE.md) · [Testing](TESTING_AND_LIMITATIONS.md)

## Mechanical assembly

The group assembled a compact, 3D-printed, **servo-driven mechanical claw arm** with a rotating base, parallel-link-style shoulder mechanism and two-finger gripper. Four SG90 servos provided the commanded functions: Servo A rotated the base; Servos B1 and B2 drove the same shoulder joint from opposite mounting orientations; Servo C operated the gripper. The system emphasized visible motion and embedded integration rather than rated lifting capability.

![Physical claw arm, isometric view](../media/Claw%20Arm%20-%20Isometric.jpg)

![Physical claw arm, side view](../media/Claw%20Arm%20-%20Side%20View.jpg)

The firmware maps the two shoulder servos to complementary electrical commands. That mapping supports coordinated motion in principle but does not establish mechanical alignment, torque sharing, validated safe end stops or measurable payload capacity. Both shoulder servos eventually failed, and the failure mechanism was not isolated.

## User-interface enclosure

The team used **SolidWorks** to design a compact two-piece enclosure with a joystick opening, a display window and mounting features. The housing was printed and physically assembled with the KY-023 joystick and original small TFT arrangement.

![Enclosure SolidWorks CAD view](../media/Joystick%20Interface%20-%20CAD.jpg)

![Joystick mounted in printed enclosure](../media/Joystick%20Interface%20-%20Internal.jpg)

![Controller enclosure assembled](../media/Joystick%20Interface%20-%20Assembled.jpg)

When the original **1.69-inch ST7789** failed and was replaced with a **2.8-inch ESP32-integrated TFT**, the already-printed housing no longer accommodated the new display. The report documents this as an unresolved packaging limitation.

## Published STL inventory

| Folder | Contents | Count |
|---|---|---:|
| [`cad/mechanical-arm/`](../cad/mechanical-arm/) | Printable arm-assembly exports with original generic `Boss-Extrude*.STL` names | **35** |
| [`cad/controller-enclosure/`](../cad/controller-enclosure/) | `Front Plate v2.STL` and `Rear Case v2.STL` | **2** |
| **Total** | | **37** |

The 35 arm files are identified by the project team as **3D-printable assembly parts**. Original source filenames are intentionally retained. They are **not accompanied by a verified part-by-part map, bill of materials, assembly order, print settings, tolerances or independent printability checks**. The STL geometry should not be interpreted as a validated, ready-to-assemble commercial kit. No native SolidWorks source is included in this public inventory.

## Engineering considerations

The physical design highlights servo mounting, linkage articulation, wiring access and the relationship between CAD geometry and assembled hardware. Before reuse, builders would need to verify each part's identity and orientation, mechanical clearances, mounting hardware, print strength, electrical cable routing and safe servo pulse limits. A future iteration should reserve envelope space for component substitutions and include mechanical end-stop checks and load measurement.

**Sources:** Project photographs, enclosure CAD image, the team's description of 37 STL exports, and the MSE 450 final report's mechanical and integration sections.
