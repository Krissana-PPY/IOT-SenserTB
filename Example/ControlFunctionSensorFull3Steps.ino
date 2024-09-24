#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>
#include <math.h>

// MPU6050 settings
MPU6050 mpu;
bool dmpReady = false;
uint8_t mpuIntStatus;
uint8_t devStatus;
uint16_t packetSize;
uint16_t fifoCount;
uint8_t fifoBuffer[64];
Quaternion q;
VectorInt16 aa, aaReal, aaWorld;
VectorFloat gravity;
float euler[3];
float ypr[3];
volatile bool mpuInterrupt = false;

// WiFi and MQTT settings
const char* ssid = "ESP32-Access-Point";
const char* password = "123456789";
const char* mqtt_broker = "192.168.4.2";
const char* mqtt_client_id = "esp32-sensor";
const char* mqtt_topic = "measure";
const char* finish_topic = "finish";
WiFiClient espClient;
PubSubClient client(espClient);

// MQTT topics
const char* reverse_topic = "reverse";
const char* forward_topic = "forward";
const char* twofloors_topic = "2F";
const char* threefloors_topic = "3F";
const char* fourfloors_topic = "4F";
const char* UDFfloors_topic = "UDF";
const char* test_topic = "test";

// Stepper motor settings
#define PUL_PIN 26  // Pulse pin
#define DIR_PIN 27  // Direction pin
#define ENA_PIN 25  // Enable pin

// Other settings
#define RXD2 16
#define TXD2 17
#define STEP_ANGLE 0.1125 // Angle per step in degrees
bool blinkState = false;
float rotation;
float facing_up;
String stringOne;
float distanceOfnumber;
float distanceOfmotor;
int steps;
int steps_lasor;


// Function declarations
void mpu_setup();
void laser_measure1();
void mpu_measure();
void reconnect_mqtt();
void callback(char* topic, byte* payload, unsigned int length);
void control_stepper_motor(float distance);
float parse_distance(const String &str);
void stepMotor(int steps);
void stepMotorWithLaserMeasurement(int steps);

void setup() {
  Serial.begin(115200);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);

  stringOne = String("Sensor ");

  WiFi.softAP(ssid, password);
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);

  reconnect_mqtt();
  Serial2.write("O");

  mpu_setup();

  // Initialize stepper motor
  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW); // Enable the driver
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == reverse_topic) {
    StaticJsonDocument<200> doc;
    doc["rotation"] = "-1";
    doc["facing_up"] = "-1";
    doc["distance"] = "-1";
    char payload[200];
    serializeJson(doc, payload);
//    client.publish("reset", payload);

  }  else if (String(topic) == twofloors_topic) {
    digitalWrite(ENA_PIN, LOW);
    mpu_setup();
    delay(500);
    Serial2.write("D");
    delay(500);
    laser_measure();
    laser_measure();
    delay(500);
    control_stepper_motor(distanceOfmotor);
    digitalWrite(DIR_PIN, HIGH);
    stepMotorWithLaserMeasurement(steps_lasor);
    delay(500);
    client.publish(finish_topic,"");
    delay(500);
    stepMotorconvert(steps + steps - steps_lasor);
    delay(500);
    stepMotorWithLaserMeasurement(steps_lasor);
    delay(500);
    client.publish(finish_topic,"");
    digitalWrite(DIR_PIN, LOW);
    stepMotorconvert((steps * 2) + steps_lasor);
    delay(500);
    Serial2.write("O");
    client.publish(forward_topic,"");

  } else if (String(topic) == threefloors_topic) {
    digitalWrite(ENA_PIN, LOW);
    mpu_setup();
    delay(500);
    Serial2.write("D");
    delay(500);
    laser_measure();
    control_stepper_motor(distanceOfmotor);
    digitalWrite(DIR_PIN, LOW);
    stepMotorconvert(steps);
    delay(500);
    digitalWrite(DIR_PIN, HIGH);
    stepMotorWithLaserMeasurement(steps_lasor);
    delay(500);
    client.publish(finish_topic,"");
    delay(500);
    stepMotorconvert((steps * 2) - steps_lasor);
    delay(500);
    stepMotorWithLaserMeasurement(steps_lasor);
    delay(500);
    client.publish(finish_topic,"");
    delay(500);
    stepMotorconvert((steps * 2) - steps_lasor);
    delay(500);
    stepMotorWithLaserMeasurement(steps_lasor);
    delay(500);
    client.publish(finish_topic,"");
    digitalWrite(DIR_PIN, LOW);
    stepMotorconvert((steps * 3) + steps_lasor);
    delay(500);
    Serial2.write("O");
    client.publish(forward_topic,"");

  } else if (String(topic) == UDFfloors_topic) {
    digitalWrite(ENA_PIN, LOW);
    mpu_setup();
    delay(500);
    Serial2.write("D");
    delay(500);
    laser_measure();
    laser_measure();
    delay(500);
    control_stepper_motor(distanceOfmotor);
    digitalWrite(DIR_PIN, HIGH);
    stepMotorconvert(steps * 4);
    delay(500);
    stepMotorWithLaserMeasurement(steps_lasor);
    delay(500);
    client.publish(finish_topic,"");
    digitalWrite(DIR_PIN, LOW);
    stepMotorconvert((steps * 4) + steps_lasor);
    Serial2.write("O");
    client.publish(forward_topic,"");

  } else if (String(topic) == test_topic) {
    mpu_measure();
    Serial2.write("D");
    delay(500);
    while (Serial2.available()) {
        char data = Serial2.read();
        stringOne += data;
    }
    Serial.print(stringOne);
    Serial.printf("rotation %.2f\n", rotation);
    Serial.printf("facing %.2f\n", facing_up);
    stringOne = "";
  }
  
}

void mpu_setup() {
  Wire.begin();
  Wire.setClock(400000);
  mpu.initialize();
  devStatus = mpu.dmpInitialize();

  mpu.setXGyroOffset(220);
  mpu.setYGyroOffset(76);
  mpu.setZGyroOffset(-85);
  mpu.setZAccelOffset(1788);

  if (devStatus == 0) {
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    mpu.PrintActiveOffsets();
    mpu.setDMPEnabled(true);
    mpuIntStatus = mpu.getIntStatus();
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
}

void laser_measure() {
  bool success = false; // Flag to indicate whether the measurement was successful
  while (!success) {
    mpu_measure();
    Serial2.write("D");
    delay(500);
    while (Serial2.available()) {
      char data = Serial2.read();
      stringOne += data;
    }

    if (stringOne.startsWith(":Er")) {
      continue; // Restart the loop, effectively re-calling laser_measure1st()
    }
    // If no error, proceed with processing
    distanceOfmotor = parse_distance(stringOne); // Convert stringOne to float distance
    stringOne = "";
    success = true; // Mark success as true to exit the loop
  }
}

void laser_measure1() {
  bool success = false; // Flag to indicate whether the measurement was successful
  while (!success) {
    mpu_measure();
    Serial2.write("D");
    delay(500);
    while (Serial2.available()) {
      char data = Serial2.read();
      stringOne += data;
    }
      if (stringOne.startsWith(":Er")) {
      continue; // Restart the loop, effectively re-calling laser_measure1st()
    }
    Serial.print(stringOne);
    distanceOfnumber = parse_distance(stringOne); // Convert stringOne to float distance
    StaticJsonDocument<200> doc;
    doc["rotation"] = abs(rotation);
    doc["facing_up"] = abs(facing_up);
    doc["distance"] = distanceOfnumber;
    char payload[200];
    serializeJson(doc, payload);
    client.publish(mqtt_topic, payload);
    
  //  Serial.printf("rotation %.2f\n", rotation);
  //  Serial.printf("facing %.2f\n", facing_up);
  //  Serial.print("distance");
  //  Serial.println(distanceOfnumber);
    stringOne = "";
    success = true;
  }
}

void mpu_measure() {
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    rotation = ypr[0] * 180 / M_PI;
    facing_up = ypr[2] * 180 / M_PI;
    delay(500);
  }
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("Connected to MQTT");
      client.subscribe(reverse_topic);
      client.subscribe(forward_topic);
      client.subscribe(twofloors_topic);
      client.subscribe(threefloors_topic);
      client.subscribe(fourfloors_topic);
      client.subscribe(UDFfloors_topic);
      client.subscribe(test_topic);
    } else {
      Serial.print("Failed to connect to MQTT. State: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void control_stepper_motor(float distanceOfmotor) {
  if (distanceOfmotor > 0) {
    float angle_rad = atan(0.15 / (distanceOfmotor));
    float angle_rad_lasor = atan(0.07 / (distanceOfmotor));
    float angle_deg = angle_rad * 180 / M_PI;
    float angle_deg_lasor = angle_rad_lasor * 180 / M_PI;
    steps = floor(angle_deg / STEP_ANGLE); // Calculate the number of steps needed
    steps_lasor= floor(angle_deg_lasor / STEP_ANGLE); // Calculate the number of steps needed
  }
}

float parse_distance(const String &str) {
  int start = str.indexOf(":") + 1;
  int end = str.indexOf("m");
  String distanceStr = str.substring(start, end);
  return distanceStr.toFloat();
}

void stepMotorconvert (int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(PUL_PIN, HIGH);
    delayMicroseconds(1000);
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(1000);
  }
}

void stepMotorWithLaserMeasurement(int steps) {
  int delayPerStep = (6000000) / (2 * steps); // Delay per step in microseconds for 6 seconds total
  int stepslaser = (2000000) / (2 * delayPerStep); // 2 seconds in milliseconds
  int stepscheck = stepslaser;

  for (int i = 0; i < steps; i++) {
    digitalWrite(PUL_PIN, HIGH);
    delayMicroseconds(delayPerStep);
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(delayPerStep);
    if (i == stepscheck) {
      laser_measure1();
      stepscheck += stepslaser;
    }
  }
}