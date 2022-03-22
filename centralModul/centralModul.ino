#include <Arduino.h>
#include <painlessMesh.h>
#include <Adafruit_MPU6050.h>
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>

#define   MESH_PREFIX     "albMsg"
#define   MESH_PASSWORD   "h1kKffg4ghPds"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

SoftwareSerial gsmSerial(13, 15);

#define ANALOG_PIN A0

//-- Input parameters:
#define DHTPIN 14
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE); 
const String modul_num = "MG";
// User stub
Adafruit_MPU6050 mpu;
bool mpuError = false;
//my methods
bool isFallen();
double absolute(float);
float batPer();
void sendData(String data);
//painless methods
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 60 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  mesh.sendBroadcast("OK");
  taskSendMessage.setInterval( random( TASK_SECOND * 59, TASK_SECOND * 61 ));
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  sendData(msg);
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);
  gsmSerial.begin(115200);
//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();

 dht.begin();

  if (!mpu.begin()) {
      mpuError = true;
  }
  else{
    //setupt motion detection
    mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
    mpu.setMotionDetectionThreshold(1);
    mpu.setMotionDetectionDuration(20);
    mpu.setInterruptPinLatch(true);  // Keep it latched.  Will turn off when reinitialized.
    mpu.setInterruptPinPolarity(true);
    mpu.setMotionInterrupt(true);
  }
  gsmSerial.println(F("AT"));
  updateSerial();
  gsmSerial.print(F("AT+CREG?\r\n")); //Check whether it has registered in the network
  delay(200);
  updateSerial();
  gsmSerial.print(F("AT+CGATT=1\r\n")); //Attach GPRS (data comunications)
  updateSerial();
  delay(500);
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}

bool isFallen(){
   // Get new sensor events with the readings
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    if(absolute(a.acceleration.x) > 5 || absolute(a.acceleration.y) > 5){
      return 1;
    }
    return 0;
}
double absolute(float n){
  if(n<0){
    return -1.0*n;
  }
  else{
    return n;
  } 
}
float batPer(){
  return 100.0 * (analogRead(ANALOG_PIN)-695,07)/204.8;
}
void sendData(String data){
  gsmSerial.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
  delay(2000);
  gsmSerial.println(F("AT+SAPBR=3,1,\"APN\",\"internet\""));
  delay(2000);
  gsmSerial.println(F("AT+SAPBR=1,1"));
  delay(2000);
  gsmSerial.println(F("AT+SAPBR=2,1"));
  delay(2000);
  gsmSerial.println(F("AT+HTTPINIT"));
  delay(2000);
  gsmSerial.println(F("AT+HTTPPARA=\"CID\",1"));
  delay(3000);
  gsmSerial.print(F("AT+HTTPPARA=\"URL\",\"www.sstv-prax.tk:1880/send?"));
  gsmSerial.print(data);
  gsmSerial.print(F("\r\n"));
  delay(2000);
  gsmSerial.print(F("AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n"));
  delay(5000);
  gsmSerial.println(F("AT+HTTPACTION=0"));
  delay(6000);
  gsmSerial.println(F("AT+HTTPTERM"));
  delay(2000);
}
