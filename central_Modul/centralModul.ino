#include "painlessMesh.h"
#include <Adafruit_MPU6050.h>
#include "SoftwareSerial.h"
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
void dly(int time);
void sendData(String data);

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
  dly(100);
  gsmSerial.print(F("AT+CREG?\r\n")); //Check whether it has registered in the network
  dly(200);
  gsmSerial.print(F("AT+CGATT=1\r\n")); //Attach GPRS (data comunications)
  dly(500);
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
void dly(int time){
  unsigned long int start = millis();
  while(millis()- start < time + 10){
  } 
}
void sendData(String data){
  gsmSerial.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
  Serial.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
  dly(2000);
  gsmSerial.println(F("AT+SAPBR=3,1,\"APN\",\"internet\""));
  Serial.println(F("AT+SAPBR=3,1,\"APN\",\"internet\""));
  dly(2000);
  gsmSerial.println(F("AT+SAPBR=1,1"));
  Serial.println(F("AT+SAPBR=1,1"));
  dly(2000);
  gsmSerial.println(F("AT+SAPBR=2,1"));
  Serial.println(F("AT+SAPBR=2,1"));
  dly(2000);
  gsmSerial.println(F("AT+HTTPINIT"));
  Serial.println(F("AT+HTTPINIT"));
  dly(2000);
  gsmSerial.println(F("AT+HTTPPARA=\"CID\",1"));
  Serial.println(F("AT+HTTPPARA=\"CID\",1"));
  dly(3000);
  gsmSerial.print(F("AT+HTTPPARA=\"URL\",\"www.sstv-prax.tk:1880/send?"));
  Serial.println(F("AT+HTTPPARA=\"URL\",\"www.sstv-prax.tk:1880/send?"));
  gsmSerial.print(data);
  gsmSerial.print(F("\r\n"));
  dly(2000);
  gsmSerial.print(F("AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n"));
  Serial.print(F("AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n"));
  dly(5000);
  gsmSerial.print(F("AT+HTTPACTION=0"));
  Serial.println(F("AT+HTTPACTION=0"));
  dly(6000);
  gsmSerial.print(F("AT+HTTPTERM"));
  Serial.println(F("AT+HTTPTERM"));
  dly(2000);
}
