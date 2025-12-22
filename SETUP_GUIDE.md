# 🚀 LIDAR System Setup Guide

## Complete guide to connect and run your Autonomous LIDAR Control System

---

## 📦 What You Need

### Hardware
| Item | Purpose |
|------|---------|
| Arduino Uno/Nano | Main controller |
| HC-SR04 Ultrasonic Sensor | Distance measurement |
| SG90 Servo Motor | 180° scanning |
| Breadboard + Jumper wires | Connections |
| USB Cable | Power + Data |

### Software
| Software | Download |
|----------|----------|
| Arduino IDE | [arduino.cc/en/software](https://www.arduino.cc/en/software) |
| Python 3 | [python.org](https://www.python.org/downloads/) |
| Processing (optional) | [processing.org](https://processing.org/download) |

---

## 🔌 STEP 1: Wire the Hardware

```
                    WIRING DIAGRAM
    ┌─────────────────────────────────────────────┐
    │                                             │
    │   HC-SR04 SENSOR          SERVO MOTOR       │
    │   ┌───────────┐           ┌──────────┐      │
    │   │ VCC ──────┼───5V──────┤ Red (5V) │      │
    │   │ GND ──────┼───GND─────┤ Brown(G) │      │
    │   │ TRIG ─────┼───Pin 12  │          │      │
    │   │ ECHO ─────┼───Pin 13  │          │      │
    │   └───────────┘           │ Orange───┼──Pin 2│
    │                           └──────────┘      │
    │                                             │
    │              ARDUINO UNO                    │
    │   ┌────────────────────────────┐            │
    │   │  [USB]              [PWR]  │            │
    │   │                            │            │
    │   │  Pin 2  ← Servo Signal     │            │
    │   │  Pin 12 ← Trig             │            │
    │   │  Pin 13 ← Echo             │            │
    │   │  5V     → Power            │            │
    │   │  GND    → Ground           │            │
    │   └────────────────────────────┘            │
    └─────────────────────────────────────────────┘
```

### Quick Pin Reference:
```
HC-SR04 Ultrasonic Sensor:
  VCC  → Arduino 5V
  GND  → Arduino GND
  TRIG → Arduino Pin 12
  ECHO → Arduino Pin 13

Servo Motor (SG90):
  Red    → Arduino 5V
  Brown  → Arduino GND  
  Orange → Arduino Pin 2
```

---

## 💻 STEP 2: Upload Arduino Code

### 2.1 Open Arduino IDE
```bash
# macOS: Open from Applications
# Or install via brew:
brew install --cask arduino
```

### 2.2 Open the Sketch
1. Open Arduino IDE
2. File → Open → Select `Radar_Code/Radar_Code.ino`

### 2.3 Select Board & Port
1. Tools → Board → **Arduino Uno** (or Nano)
2. Tools → Port → Select your Arduino port:
   - macOS: `/dev/cu.usbmodem...` or `/dev/cu.usbserial...`
   - Windows: `COM3`, `COM4`, etc.
   - Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`

### 2.4 Upload
1. Click **Upload** button (→ arrow)
2. Wait for "Done uploading"

### 2.5 Test It
1. Open Serial Monitor (Tools → Serial Monitor)
2. Set baud rate to **9600**
3. You should see: `angle,distance.` data streaming

```
15,45.
16,44.
17,42.
...
```

---

## 🐍 STEP 3: Run Python Visualization

### 3.1 Install Dependencies
```bash
# Open Terminal and run:
pip install pygame pyserial
```

### 3.2 Find Your Serial Port
```bash
# macOS/Linux:
ls /dev/cu.usb*

# Example output:
# /dev/cu.usbmodem2101
```

### 3.3 Update the Port in Code
Edit `radar.py` line 12:
```python
# Change this to YOUR port:
SERIAL_PORT = "/dev/cu.usbmodem2101"  # ← Update this!
```

### 3.4 Run the Visualization
```bash
cd /Users/pauladutwum/Documents/Arduino
python radar.py
```

### 3.5 What You'll See
```
┌────────────────────────────────────────┐
│                                        │
│         RADAR VISUALIZATION            │
│                                        │
│              ╱ ╲                       │
│            ╱     ╲                     │
│          ╱    •    ╲   ← Objects show  │
│        ╱             ╲     in RED      │
│      ╱                 ╲               │
│    ──────────●──────────               │
│           SWEEP LINE                   │
│                                        │
│   Angle: 90°    Distance: 25cm         │
└────────────────────────────────────────┘
```

---

## 🎮 STEP 4: How It Works Together

```
┌──────────────────────────────────────────────────────────────┐
│                    DATA FLOW                                  │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌──────────┐  │
│  │ SERVO   │───▶│ SENSOR  │───▶│ ARDUINO │───▶│ COMPUTER │  │
│  │ rotates │    │measures │    │processes│    │visualizes│  │
│  └─────────┘    └─────────┘    └─────────┘    └──────────┘  │
│       │              │              │              │         │
│       ▼              ▼              ▼              ▼         │
│   Angle 0-180    Distance      "90,25."      Radar Display  │
│                  in cm         via USB                       │
│                                                              │
└──────────────────────────────────────────────────────────────┘

TIMING:
  1. Servo moves 1° ────────────────────▶ 30ms
  2. Sensor sends ultrasonic pulse ─────▶ ~0.1ms
  3. Wait for echo ─────────────────────▶ up to 30ms
  4. Calculate distance ────────────────▶ instant
  5. Send via UART ─────────────────────▶ ~1ms
  6. Repeat for next angle
  
  Full 180° sweep ≈ 9 seconds
```

---

## 🔧 STEP 5: Troubleshooting

### ❌ "Port not found" or "Permission denied"
```bash
# macOS/Linux - give permission:
sudo chmod 666 /dev/cu.usbmodem*

# Or add yourself to dialout group (Linux):
sudo usermod -a -G dialout $USER
```

### ❌ "No data received"
1. Check Arduino is connected
2. Check Serial Monitor shows data
3. Make sure Python script port matches Arduino port
4. Close Serial Monitor before running Python (only one can use port)

### ❌ "Servo not moving"
1. Check servo wiring (orange to Pin 2)
2. Make sure 5V power is connected
3. Try external 5V power for servo if USB isn't enough

### ❌ "Distance always 0"
1. Check HC-SR04 wiring
2. Make sure Trig→Pin 12, Echo→Pin 13
3. Point sensor at a wall/object 10-100cm away

---

## 🎯 STEP 6: Quick Start Commands

### All-in-one (after hardware is connected):
```bash
# 1. Install Python packages (first time only)
pip install pygame pyserial

# 2. Navigate to project
cd /Users/pauladutwum/Documents/Arduino

# 3. Update port in radar.py, then run:
python radar.py
```

---

## 📊 Understanding the Output

### Serial Data Format
```
angle,distance.
  │       │
  │       └── Distance in centimeters (0-400)
  └────────── Servo angle in degrees (15-165)

Example: "90,25." means:
  - Servo at 90° (pointing straight ahead)
  - Object detected at 25cm distance
```

### Radar Display Colors
| Color | Meaning |
|-------|---------|
| 🟢 Green | Scanning line (no object nearby) |
| 🔴 Red | Object detected within threshold |
| ⚫ Black | Background / no reading |

---

## 🏆 You're Ready!

Once everything is connected:
1. Power Arduino via USB
2. Run `python radar.py`
3. Watch the radar scan in real-time!
4. Move your hand in front of the sensor to see detection

**Have fun with your LIDAR system!** 🎉

