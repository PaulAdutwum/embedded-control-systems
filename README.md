#  Autonomous Lidar Control System

Project: Autonomous Lidar & Electro-Mechanical Control System
Technical Overview
This project implements a high-reliability real-time control system designed for automated obstacle detection and actuation. The system integrates Lidar/ultrasonic sensor data via UART and GPIO protocols to map environment obstruction zones with centimeter-level precision. The core architecture is built on a bare-metal C firmware foundation, utilizing Interrupt Service Routines (ISRs) to maintain a deterministic system response time of <50ms.

Engineering Specifications
Closed-Loop Actuation: 180-degree automated sweep (15° to 165°) driven by a servo motor utilizing precise Pulse Width Modulation (PWM) control.

Low-Latency Processing: Optimized state-machine transitions ensure real-time distance calculation and sensor-to-motor reaction speeds suitable for safety-critical environments.

Hardware-Software Integration: Implemented a robust data pipeline to ingest raw distance signals, applying threshold-based logic to trigger a synchronized response across a piezo buzzer, LED array, and motor drivetrain.

Signal Calibration: Integrated a potentiometer-based tuning system to allow for dynamic adjustment of the detection sensitivity (5cm to 50cm) based on environment noise and operational requirements.

Validation & Visualization: Real-time telemetry data is streamed via serial communication to a custom Python-based GUI for system characterization and performance monitoring.

Technical Stack & Tools
Languages: Bare-metal C, Python (Telemetry Visualization)

Protocols: UART, PWM, GPIO, Serial Communication

Hardware Debugging: Logic Analyzers (Signal Timing), Oscilloscopes (Signal Integrity), Multimeters

Control Theory: State-Machine Architecture, Deterministic Timing, ISR Management

---

## 🔧 Hardware Requirements

| Component          | Quantity | Description                           |
| ------------------ | -------- | ------------------------------------- |
| Arduino Uno/Nano   | 1        | Main microcontroller                  |
| HC-SR04            | 1        | Ultrasonic distance sensor / LIDAR          |
| SG90 Servo         | 1        | 180° rotation servo motor             |
| LDR                | 1        | Light-dependent resistor              |
| Potentiometer      | 1        | 10kΩ for sensitivity control          |
| LED                | 1        | 5mm indicator LED                     |
| Passive Buzzer     | 1        | Alarm output                          |
| Motor + Transistor | 1        | High-power actuation (NPN transistor) |
| Resistors          | Various  | 220Ω for LED, 10kΩ for LDR            |






##  Communication Protocol

Data is transmitted via UART at **9600 baud** in the following format:

```
<angle>,<distance>.
```

| Field    | Type | Range  | Description                       |
| -------- | ---- | ------ | --------------------------------- |
| angle    | int  | 15-165 | Current servo position in degrees |
| distance | int  | 0-400  | Measured distance in centimeters  |

**Example:** `90,25.` → Angle: 90°, Distance: 25cm

---



---

## Performance Specifications

| Metric             | Value                   |
| ------------------ | ----------------------- |
| Sweep Range        | 150° (15° - 165°)       |
| Angular Resolution | 1° per step             |
| Response Time      | <50ms                   |
| Detection Range    | 2cm - 400cm             |
| Detection Accuracy | ±3mm                    |
| Sweep Period       | ~9 seconds (full cycle) |
| Baud Rate          | 9600 bps                |

---

##  Future Improvements

- [ ] Multiple ultrasonic sensors for wider coverage
- [ ] Data logging to SD card
- [ ] WiFi connectivity for remote monitoring
- [ ] Machine learning for object classification
- [ ] 3D-printed enclosure design

---

##  Project Structure

```
Arduino/
├── README.md                    # This file
├── SETUP_GUIDE.md               # Step-by-step setup instructions
├── images/                      # Project photos and demos
│   ├── hardware.jpeg            # Hardware setup photo
│   ├── hardware1.jpeg           # System assembly photo
│   └── display.jpeg             # Radar visualization screenshot
├── Radar_Code/
│   └── Radar_Code.ino           # Arduino firmware
├── Lidar_Control_System_C/      # Comprehensive C implementation
│   ├── src/                     # Source files
│   │   ├── main.c               # Main application
│   │   ├── hal.c                # Hardware abstraction layer
│   │   └── state_machine.c      # State machine logic
│   ├── drivers/                 # Hardware drivers
│   │   ├── uart.c/h             # UART communication
│   │   ├── pwm.c/h              # Motor control
│   │   └── sensor.c/h           # Sensor interface
│   ├── include/                 # Header files
│   │   ├── config.h             # System configuration
│   │   └── hal.h                # HAL interface
│   └── Makefile                 # Build system
├── radar.py                     # Python/Pygame visualization
└── Radar_Visualization.pde      # Processing visualization
```


