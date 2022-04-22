#include "painlessMesh.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>

#define   MESH_PREFIX     "MESH SSD"
#define   MESH_PASSWORD   "MESH PASSWORD"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

#define ANALOG_PIN A0

//-- Input parameters:
#define DHTPIN 14
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE); 
const String modul_num = "M1";
// User stub
Adafruit_MPU6050 mpu;
bool mpuError = false;

bool isFallen();
double absolute(float);
float batPer();

void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 60 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  String msg = "";
  msg += "f=" + String(isFallen());
  if (!isnan(dht.readTemperature()) && !isnan(dht.readHumidity())){
    String t = String(dht.readTemperature()), h= String(dht.readHumidity());
    msg += "&t=" + t.substring(0,4);
    msg += "&h=" + h.substring(0,4);
  }
  msg += "&b="+ String(batPer());
  msg += "&m=" + modul_num;
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 59, TASK_SECOND * 61 ));
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
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
  return (100.0 * (analogRead(ANALOG_PIN)-695,07)/204.8);
}
