#include <Servo.h>


const int trigPin = 12;
const int echoPin = 13;

const int ledPin = 3;     // The Small LED on the board
const int motorPin = 8;   // The High-Power Motor (via Transistor)
const int buzzerPin = 4;  // Passive Buzzer

const int ldrPin = A0;    // Light Sensor
const int potPin = A1;    // Sensitivity Knob

// --- VARIABLES ---
long duration;
int distance;
int alarmThreshold = 20;  
int lightLevel = 0;       
Servo myServo; 

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  pinMode(ledPin, OUTPUT);   // LED Setup
  pinMode(motorPin, OUTPUT); // Motor Setup
  pinMode(buzzerPin, OUTPUT); 
  
  Serial.begin(9600); 
  myServo.attach(2);
}

void loop() {
  updateSmartSettings();

  // Sweep Left -> Right
  for(int i=15; i<=165; i++){  
    myServo.write(i);
    delay(30);
    distance = calculateDistance();
    checkAlarm(i, distance); 
    sendData(i, distance);
  }
  
  // Sweep Right -> Left
  for(int i=165; i>15; i--){  
    myServo.write(i);
    delay(30);
    distance = calculateDistance();
    checkAlarm(i, distance); 
    sendData(i, distance);
  }
}

void updateSmartSettings() {
  // Map Knob (0-1023) to Range (5cm-50cm)
  int potValue = analogRead(potPin);
  alarmThreshold = map(potValue, 0, 1023, 5, 50);
  
  // Read Light Sensor
  lightLevel = analogRead(ldrPin);
}

void checkAlarm(int currentAngle, int currentDist) {
  
  bool objectDetected = (currentDist < alarmThreshold && currentDist > 0);
  bool isNightTime = (lightLevel < 900); 
  
  while (objectDetected) {
    updateSmartSettings(); 
    
  
    
  
    digitalWrite(ledPin, HIGH);
    
    // (Motor Fan)
    digitalWrite(motorPin, HIGH);
    
    // TRIGGER  ALARM (Only if Dark)
    if (isNightTime) {
       tone(buzzerPin, 1000); 
    } else {
       noTone(buzzerPin); 
    }
    
    sendData(currentAngle, currentDist);
    
  
    delay(100); 
    noTone(buzzerPin);                
    
    
    distance = calculateDistance();
    objectDetected = (distance < alarmThreshold && distance > 0);
    delay(100);
  }
  
  
  digitalWrite(ledPin, LOW);
  digitalWrite(motorPin, LOW); 
  noTone(buzzerPin);         
}

void sendData(int angle, int dist){
  Serial.print(angle);
  Serial.print(",");
  Serial.print(dist);
  Serial.print(".");
}

int calculateDistance(){ 
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); 
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance= duration*0.034/2;
  return distance;
}