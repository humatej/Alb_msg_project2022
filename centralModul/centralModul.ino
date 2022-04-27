#include <painlessMesh.h>
#include <Adafruit_MPU6050.h>
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>

#define   MESH_PREFIX     "MESH SSD"
#define   MESH_PASSWORD   "MESH PASSWORD"
#define   MESH_PORT       5555
#define   MESH_SENDING_PERIOD 300000

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;
// temporary data save
unsigned int i = 0;
String data[] = {"","","","","","","","","",""};

SoftwareSerial gsmSerial(13, 15);

#define ANALOG_PIN A0

//-- Input parameters:
#define DHTPIN 14
#define DHTTYPE DHT22
//battery constant
#define VMAX 1024
#define VMIN 698.1


DHT dht(DHTPIN, DHTTYPE); 
const String modul_num = "MG";
bool is_running = false;
// User stub
Adafruit_MPU6050 mpu;
//my methods
bool isFallen();
double absolute(float);
float batPer();
int nodeIndex(String);
//painless methods
void sendMessage() ; // Prototype so PlatformIO doesn't complain
//void sendAlldata();
void sendData();

Task taskSendMessage(  100 , TASK_FOREVER, &sendMessage );
//Task sd(100, TASK_FOREVER, &sendData);
//Task forik(TASK_IMMEDIATE, 10, &forloop);

void sendMessage() {
  Serial.println(i);
  if(i == 0){
    String msg = "";
    msg += "f=" + String(isFallen());
    if (!isnan(dht.readTemperature()) && !isnan(dht.readHumidity())){
      String t = String(dht.readTemperature()), h= String(dht.readHumidity());
      msg += "&t=" + t.substring(0,4);
      msg += "&h=" + h.substring(0,4);
    }
    msg += "&b="+ String(batPer());
    msg += "&m=" + modul_num;
    data[0] = msg;
  }
  if(i > 9){
    i = 1;
    taskSendMessage.delay(MESH_SENDING_PERIOD);
  }
  else{
    if(data[i] != ""){
      sendData();
    }
    else{
      i++;
    }
  }

}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  //sendData(msg);
  unsigned int index = nodeIndex(msg);
  data[index] = msg;
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

  userScheduler.addTask(taskSendMessage);
  //userScheduler.addTask(tsd);
  //userScheduler.addTask(sAlldata);
  taskSendMessage.enable();
  //tsd.enable();
  //sAlldata.enable();

 dht.begin();

  if (mpu.begin()) {
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
  const float batper = 100 * (analogRead(ANALOG_PIN) - VMIN)/(VMAX - VMIN);
  if(batper > 100){
    return 100;
  }
  if(batper < 0 ){
    return 0;
  }
  
  return batper;
}


int nodeIndex(String msg){
  const int num = msg[msg.length()-1] - 48;
  return (num - 1);
}

void updateSerial()
{
  while (Serial.available()) 
  {
    gsmSerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(gsmSerial.available()) 
  {
    Serial.write(gsmSerial.read());//Forward what Software Serial received to Serial Port
  }
}

void sendData(){
  gsmSerial.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
  updateSerial();
  taskSendMessage.setCallback(&sendData2);
  taskSendMessage.delay(2000);
}
void sendData2(){
  gsmSerial.println(F("AT+SAPBR=3,1,\"APN\",\"internet\""));
  updateSerial();
  taskSendMessage.setCallback(&sendData3);
  taskSendMessage.delay(3000);
}
void sendData3(){
  gsmSerial.println(F("AT+SAPBR=1,1"));
  updateSerial();
  taskSendMessage.setCallback(&sendData4);
  taskSendMessage.delay(2000);
}
void sendData4(){
  gsmSerial.println(F("AT+SAPBR=2,1"));
  updateSerial();
  taskSendMessage.setCallback(&sendData5);
  taskSendMessage.delay(2000);
}
void sendData5(){
  gsmSerial.println(F("AT+HTTPINIT"));
  updateSerial();
  taskSendMessage.setCallback(&sendData6);
  taskSendMessage.delay(2000);
}
void sendData6(){
  gsmSerial.println(F("AT+HTTPPARA=\"CID\",1"));
  updateSerial();
  taskSendMessage.setCallback(&sendData7);
  taskSendMessage.delay(3000);
}
void sendData7(){
  Serial.println(data[i]);
  gsmSerial.println("AT+HTTPPARA=\"URL\",\"www.sstv-prax.tk:1880/send?"+data[i]+"\"");
  updateSerial();
  taskSendMessage.setCallback(&sendData8);
  taskSendMessage.delay(5000);
}
void sendData8(){
  gsmSerial.println(F("AT+HTTPPARA=\"CONTENT\",\"application/json\""));
  updateSerial();
  taskSendMessage.delay(4000);
  taskSendMessage.setCallback(&sendData9);
}
void sendData9(){
  gsmSerial.println(F("AT+HTTPACTION=0"));
  updateSerial();
  taskSendMessage.setCallback(&sendData10);
  taskSendMessage.delay(6000);
}
void sendData10(){
  gsmSerial.println(F("AT+HTTPREAD"));
  updateSerial();
  taskSendMessage.setCallback(&sendData11);
  taskSendMessage.delay(10000);
}
void sendData11(){
  data[i] = "";
  i++;
  taskSendMessage.setCallback(&sendMessage);
}
