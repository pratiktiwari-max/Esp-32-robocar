# ESP32-Based Multi-Mode Robot Car

An ESP32-based robotic car developed with three operating modes: **Line Following, Wall Following, and Manual Bluetooth Control**. The project combines embedded programming, sensor integration, motor control, and basic autonomous navigation.

## 🚗 Project Overview

This project demonstrates how an ESP32 microcontroller can be used to build a robot capable of autonomous movement and wireless manual control.

The robot uses infrared (IR) sensors for line detection, ultrasonic sensors for obstacle-distance measurement, and an L298N motor driver to control its DC motors. Manual operation is supported through Bluetooth communication with a smartphone.

## ✨ Features

- **Line Following:** Uses three IR sensors to detect a line and adjust the robot's movement accordingly.
- **Wall Following:** Uses ultrasonic distance measurements to help the robot follow a wall.
- **Manual Bluetooth Control:** Allows the robot to be controlled wirelessly from a smartphone using Bluetooth.
- **Obstacle Detection:** Uses three ultrasonic sensors positioned at the front, left, and right.
- **Differential Motor Control:** Controls the left and right motors to perform forward movement, turning, and direction correction.
- **ESP32-Based Control:** Uses the ESP32's built-in Bluetooth capability for wireless communication.

## 🧰 Hardware Components

| Component | Purpose |
|---|---|
| ESP32 development board | Main microcontroller |
| L298N motor driver | DC motor direction and speed control |
| 3 IR sensors | Line detection |
| 3 ultrasonic sensors | Distance measurement and obstacle detection |
| 2 DC geared motors | Robot propulsion |
| Robot chassis | Mechanical structure |
| Front support wheel | Balance and support |
| Battery and connecting wires | Power supply and connections |

## 🔌 Pin Configuration

The following pin assignments are used in the project.

### IR Sensors

| Connection | ESP32 GPIO |
|---|---:|
| IR Sensor 1 | GPIO 13 |
| IR Sensor 2 | GPIO 4 |
| IR Sensor 3 | GPIO 27 |

### Ultrasonic Sensors

| Sensor | Trigger | Echo |
|---|---:|---:|
| Left | GPIO 26 | GPIO 25 |
| Front | GPIO 33 | GPIO 32 |
| Right | GPIO 15 | GPIO 35 |

### L298N Motor Driver

| Motor driver pin | ESP32 GPIO |
|---|---:|
| IN1 | GPIO 21 |
| IN2 | GPIO 22 |
| IN3 | GPIO 18 |
| IN4 | GPIO 19 |
| ENA | GPIO 16 |
| ENB | GPIO 17 |

**Note:** GPIO 35 is input-only, which is suitable for an ultrasonic sensor's Echo signal. Ensure the Echo voltage is compatible with the ESP32's 3.3 V GPIO inputs; use a voltage divider or level shifter where required.

## 💻 Software and Technologies

- **Microcontroller:** ESP32
- **Programming environment:** Arduino IDE
- **Programming language:** C/C++ for Arduino
- **Libraries:** `Arduino.h` and `BluetoothSerial.h`
- **Wireless communication:** Bluetooth Classic
- **Bluetooth device name:** `RoboCar_BT`
- **Motor control:** PWM-based speed control at 5 kHz with 8-bit resolution

## 📱 Bluetooth Control

The ESP32 advertises itself as `RoboCar_BT`. A compatible smartphone Bluetooth-control application can connect to the robot and send movement commands.

The Bluetooth implementation uses the ESP32's Bluetooth Classic functionality through the `BluetoothSerial` library. Wi-Fi is not used by the current implementation.

## ⚙️ How It Works

1. The ESP32 reads the IR sensors and ultrasonic distance measurements.
2. In line-following mode, the IR sensor readings help determine the appropriate motor response.
3. In wall-following mode, ultrasonic readings guide the robot's movement relative to a nearby wall.
4. In manual mode, Bluetooth commands received from a smartphone control the robot's movement.
5. The ESP32 adjusts the motor driver's direction inputs and PWM signals to control movement.

The control logic combines sensor readings, conditional decision-making, and differential motor-speed adjustment to support the robot's different operating modes.

## 🎯 Learning Outcomes

Through this project, we explored:

- ESP32 programming and embedded systems
- IR and ultrasonic sensor integration
- DC motor control using an L298N driver
- PWM-based motor-speed control
- Bluetooth communication between a microcontroller and smartphone
- Basic autonomous navigation and rule-based control logic
- Hardware–software integration and circuit connections

## 🚀 Future Improvements

- Add PID-based line-following control for smoother navigation.
- Improve obstacle avoidance and wall-distance regulation.
- Develop a dedicated smartphone control interface.
- Improve power management and motor-control efficiency.
- Add telemetry and sensor-status monitoring.

## 👨‍💻 Project Information

**Project type:** Embedded Systems and Robotics  
**Platform:** ESP32  
**Development environment:** Arduino IDE

This project was developed as a hands-on learning experience in embedded programming, sensor-based navigation, wireless control, and electronics.

