#include <Arduino.h>
#include "BluetoothSerial.h" // Added for SriTu App Control

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to enable it
#endif

BluetoothSerial SerialBT;

// =====================================================================
// PIN CONFIGURATIONS
// =====================================================================

// ---------------- IR Sensor Pins (Source 1) ----------------
#define IR_LEFT   13
#define IR_CENTER 4
#define IR_RIGHT  27

// ---------------- Ultrasonic Pins (Source 2) ----------------
const int leftSensorTrig  = 26;
const int leftSensorEcho  = 25;
const int frontSensorTrig = 33;
const int frontSensorEcho = 32;
const int rightSensorTrig = 15;  
const int rightSensorEcho = 35;  

// ---------------- L298N Motor Driver Pins ----------------
#define IN1 21   // Right motor forward
#define IN2 22   // Right motor backward
#define IN3 18   // Left motor forward
#define IN4 19   // Left motor backward
#define ENA 16   // PWM - Right motor speed
#define ENB 17   // PWM - Left motor speed

const int motorLeftForward   = IN3;
const int motorLeftBackward  = IN4;
const int motorRightForward  = IN1;
const int motorRightBackward = IN2;
const int MOTOR_L_PWM = ENB;
const int MOTOR_R_PWM = ENA;

// =====================================================================
// CONSTANTS & VARIABLES
// =====================================================================

// --- ESP32 PWM Settings (Core 3.x API) ---
const int PWM_FREQ = 5000;
const int PWM_RES  = 8;

// --- IR Tracking Constants (Source 1) ---
const int baseSpeed       = 180;  
const int correctionSpeed = 5;   
const int turnSpeed       = 160;  
const int creepSpeed      = 130;  
int lastTurnDirection = 0; 
unsigned long lineLostTime = 0;
bool wasOnLine = true;

// --- Ultrasonic Avoidance Constants (Source 2) ---
const int NORMAL_SPEED = 100;         
const int FAST_SPEED = 200;           
const float CRITICAL_SIDE = 12.0;     
const float CRITICAL_FRONT = 17.0;    
const float FAST_THRESHOLD = 30.0;    
const float CENTER_TOLERANCE = 4.0;   

// --- Global Mode States ---
bool isUltraMode = false;
bool isManualMode = false;     // Added for Bluetooth Overrides
int manualSpeed = 200;         // Default locked speed for manual mode

// Forward Declarations for Clean Compilation
void ir_stopMotors();
void runIRMode();
void runUltraMode();
float getDistance(int trig, int echo);
void readLineSensors(bool &leftLine, bool &midLine, bool &rightLine);
void recoverLine();
void ir_moveForward(int leftSpeed, int rightSpeed);
void ir_moveBackward(int leftSpeed, int rightSpeed);
void ir_pivotLeft(int speed);
void ir_pivotRight(int speed);
void processManualCommand(char cmd);
void manual_move(bool leftFwd, bool leftBwd, bool rightFwd, bool rightBwd, int leftSpeed, int rightSpeed);

// =====================================================================
// SETUP
// =====================================================================
void setup() {
  Serial.begin(115200);

  // Setup IR Pins
  pinMode(IR_LEFT,   INPUT);
  pinMode(IR_CENTER, INPUT);
  pinMode(IR_RIGHT,  INPUT);

  // Setup Ultrasonic Pins
  pinMode(leftSensorTrig, OUTPUT);  pinMode(leftSensorEcho, INPUT);
  pinMode(frontSensorTrig, OUTPUT); pinMode(frontSensorEcho, INPUT);
  pinMode(rightSensorTrig, OUTPUT); pinMode(rightSensorEcho, INPUT);

  // Setup Motor Driver Output Pins
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // Attach ESP32 Hardware PWM Channels
  ledcAttach(ENA, PWM_FREQ, PWM_RES);
  ledcAttach(ENB, PWM_FREQ, PWM_RES);

  // Start Bluetooth Classic Serial
  SerialBT.begin("RoboCar_BT");

  ir_stopMotors();
  Serial.println("Robot initialized. Bluetooth Active. Starting in Autonomous Mode...");
  delay(1000); 
}

// =====================================================================
// MAIN LOOP & MODE SWITCHING
// =====================================================================
void loop() {
  // 1. Always check for runtime Bluetooth Commands from SriTu App
  if (SerialBT.available()) {
    char cmd = SerialBT.read();
    
    // --- MODE SWITCHING (CHANNEL 1) ---
    if (cmd == '1') { 
      isManualMode = true;
      ir_stopMotors();
      Serial.println(">>> Shifting to Manual Mode (Channel 1 ON) <<<");
    } 
    else if (cmd == '2') {
      isManualMode = false;
      ir_stopMotors();
      Serial.println(">>> Returning to Autonomous Routine (Channel 1 OFF) <<<");
    }
    
    // --- EXTRA APP BUTTON (CHANNEL 2) ---
    else if (cmd == '3') {
      // Channel 2 ON (Add code here later if you want to turn on lights/buzzer)
      Serial.println("Channel 2 ON");
    }
    else if (cmd == '4') {
      // Channel 2 OFF (Add code here later if you want to turn off lights/buzzer)
      Serial.println("Channel 2 OFF");
    }
    
    // --- MANUAL DRIVING ---
    // Process incoming driving commands ONLY if manual mode is active
    else if (isManualMode) {
      processManualCommand(cmd);
    }
  }

  // 2. Execute Motor Output Paths based on the Active Mode
  if (!isManualMode) {
    if (!isUltraMode) {
      // Check condition to transition to Ultra mode permanently
      float distanceLeft  = getDistance(leftSensorTrig, leftSensorEcho);
      float distanceRight = getDistance(rightSensorTrig, rightSensorEcho);

      if (distanceLeft < 18.0 && distanceRight < 18.0) {
        isUltraMode = true;
        ir_stopMotors();
        delay(200);
        Serial.println("Threshold met. Switching to Ultrasonic Mode permanently.");
      } else {
        runIRMode(); // Execute normal Source 1 logic (Line Follower)
      }
    } else {
      runUltraMode(); // Execute normal Source 2 logic (Wall Follower) indefinitely
    }
  }
}

// =====================================================================
// BLUETOOTH MANUAL DRIVING ENGINE
// =====================================================================
void processManualCommand(char cmd) {
  switch(cmd) {
    // Pure Directional Commands
    case 'U': case 'u': manual_move(true, false, true, false, manualSpeed, manualSpeed); break; // Forward
    case 'D': case 'd': manual_move(false, true, false, true, manualSpeed, manualSpeed); break; // Backward
    case 'L': case 'l': manual_move(false, true, true, false, manualSpeed * 0.8, manualSpeed * 0.8); break; // Sharp Turn Left
    case 'R': case 'r': manual_move(true, false, false, true, manualSpeed * 0.8, manualSpeed * 0.8); break; // Sharp Turn Right
    case 'S': case 's': manual_move(false, false, false, false, 0, 0); break;                   // Stop

    // Diagonal Slanted Directions (Inner motor dropped to 40% speed for smooth curving)
    case 'F': case 'f': manual_move(true, false, true, false, manualSpeed *0.5, manualSpeed * 0.9); break; // Forward-Left 
    case 'T': case 't': manual_move(true, false, true, false, manualSpeed * 0.9, manualSpeed *0.5); break; // Forward-Right
    case 'G': case 'g': manual_move(false, true, true, false, manualSpeed *0.5, manualSpeed * 0.9); break; // Back-Left
    case 'H': case 'h': manual_move(false, true, false, true, manualSpeed * 0.9, manualSpeed *0.5); break; // Back-Right
  }
}

// Low-Level Manual Pin Driver Utility
void manual_move(bool leftFwd, bool leftBwd, bool rightFwd, bool rightBwd, int leftSpeed, int rightSpeed) {
  digitalWrite(IN3, leftFwd ? HIGH : LOW);
  digitalWrite(IN4, leftBwd ? HIGH : LOW);
  digitalWrite(IN1, rightFwd ? HIGH : LOW);
  digitalWrite(IN2, rightBwd ? HIGH : LOW);
  ledcWrite(ENB, constrain(leftSpeed, 0, 255));
  ledcWrite(ENA, constrain(rightSpeed, 0, 255));
}

// =====================================================================
// IR MODE LOGIC (From Source 1)
// =====================================================================
void runIRMode() {
  bool leftLine, midLine, rightLine;
  readLineSensors(leftLine, midLine, rightLine);

  if (leftLine || midLine || rightLine) {
    wasOnLine = true;
  }

  if (midLine && !leftLine && !rightLine) {
    lastTurnDirection = 0;
    ir_moveForward(baseSpeed, baseSpeed);
  } else if (leftLine && midLine && !rightLine) {
    lastTurnDirection = -1;
    ir_pivotLeft(turnSpeed);
  } else if (rightLine && midLine && !leftLine) {
    lastTurnDirection = +1;
    ir_pivotRight(turnSpeed);
  } else if (leftLine && !midLine && !rightLine) {
    lastTurnDirection = -1;
    ir_moveForward(baseSpeed, correctionSpeed);
  } else if (rightLine && !midLine && !leftLine) {
    lastTurnDirection = +1;
    ir_moveForward(correctionSpeed, baseSpeed);
  } else if (leftLine && rightLine && !midLine) {
    ir_moveForward(baseSpeed, baseSpeed);
  } else if (leftLine && midLine && rightLine) {
    ir_moveForward(baseSpeed, baseSpeed);
  } else {
    if (wasOnLine) {
      lineLostTime = millis(); 
      wasOnLine = false;
    }
    recoverLine();
  }
  delay(10);
}

void readLineSensors(bool &leftLine, bool &midLine, bool &rightLine) {
  leftLine  = (digitalRead(IR_LEFT)   == LOW);
  midLine   = (digitalRead(IR_CENTER) == LOW);
  rightLine = (digitalRead(IR_RIGHT)  == LOW);
}

void recoverLine() {
  if (millis() - lineLostTime < 1000) {
    ir_moveBackward(creepSpeed, creepSpeed);
  } else {
    if (lastTurnDirection < 0) {
      ir_pivotLeft(creepSpeed);   
    } else if (lastTurnDirection > 0) {
      ir_pivotRight(creepSpeed);  
    } else {
      ir_moveBackward(creepSpeed, creepSpeed); 
    }
  }
}

// Source 1 Specific Movement Functions
void ir_moveForward(int leftSpeed, int rightSpeed) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);   
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);   
  ledcWrite(ENA, rightSpeed);
  ledcWrite(ENB, leftSpeed);
}
void ir_moveBackward(int leftSpeed, int rightSpeed) {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);   
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);   
  ledcWrite(ENA, rightSpeed);
  ledcWrite(ENB, leftSpeed);
}
void ir_pivotLeft(int speed) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  ledcWrite(ENA, speed);
  ledcWrite(ENB, speed);
}
void ir_pivotRight(int speed) {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(ENA, speed);
  ledcWrite(ENB, speed);
}
void ir_stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
}

// =====================================================================
// ULTRASONIC MODE LOGIC (From Source 2)
// =====================================================================
void runUltraMode() {
  float distanceFront = getDistance(frontSensorTrig, frontSensorEcho); delay(4);
  float distanceLeft  = getDistance(leftSensorTrig, leftSensorEcho);   delay(4);
  float distanceRight = getDistance(rightSensorTrig, rightSensorEcho); delay(4);

  if (distanceFront <= CRITICAL_FRONT) {
    ledcWrite(MOTOR_L_PWM, 0); ledcWrite(MOTOR_R_PWM, 0);
    digitalWrite(motorLeftForward, LOW);  digitalWrite(motorLeftBackward, LOW);
    digitalWrite(motorRightForward, LOW); digitalWrite(motorRightBackward, LOW);
    delay(10);
    
    if (distanceLeft >= distanceRight) {
      digitalWrite(motorLeftForward, LOW);    digitalWrite(motorLeftBackward, HIGH);
      digitalWrite(motorRightForward, HIGH);  digitalWrite(motorRightBackward, LOW);
      ledcWrite(MOTOR_L_PWM, NORMAL_SPEED);
      ledcWrite(MOTOR_R_PWM, NORMAL_SPEED);  
    } else {
      digitalWrite(motorLeftForward, HIGH);   digitalWrite(motorLeftBackward, LOW);
      digitalWrite(motorRightForward, LOW);    digitalWrite(motorRightBackward, HIGH);
      ledcWrite(MOTOR_L_PWM, NORMAL_SPEED);
      ledcWrite(MOTOR_R_PWM, NORMAL_SPEED); 
    }
    delay(100); 
  } else {
    int currentSpeed = NORMAL_SPEED;
    if (distanceFront > FAST_THRESHOLD) { 
      currentSpeed = FAST_SPEED;
    }
    
    if (distanceLeft <= CRITICAL_SIDE || distanceLeft < (distanceRight - CENTER_TOLERANCE)) {
      digitalWrite(motorLeftForward, HIGH);   digitalWrite(motorLeftBackward, LOW);
      digitalWrite(motorRightForward, HIGH);  digitalWrite(motorRightBackward, LOW);
      ledcWrite(MOTOR_L_PWM, currentSpeed / 2);      
      ledcWrite(MOTOR_R_PWM, currentSpeed);           
    } else if (distanceRight <= CRITICAL_SIDE || distanceRight < (distanceLeft - CENTER_TOLERANCE)) {
      digitalWrite(motorLeftForward, HIGH);   digitalWrite(motorLeftBackward, LOW);
      digitalWrite(motorRightForward, HIGH);  digitalWrite(motorRightBackward, LOW);
      ledcWrite(MOTOR_L_PWM, currentSpeed);          
      ledcWrite(MOTOR_R_PWM, currentSpeed / 2);      
    } else {
      digitalWrite(motorLeftForward, HIGH);   digitalWrite(motorLeftBackward, LOW);
      digitalWrite(motorRightForward, HIGH);  digitalWrite(motorRightBackward, LOW);
      ledcWrite(MOTOR_L_PWM, currentSpeed);
      ledcWrite(MOTOR_R_PWM, currentSpeed);
    }
  }
}

float getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 5000UL);
  if (duration == 0) return 150.0;             
  return duration * 0.0343 / 2.0;
}