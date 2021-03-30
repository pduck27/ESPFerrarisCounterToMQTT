#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <Credentials.h> // Should link to your own credentials.h file. See Credentials_sample.h in include-directory.

// Sensor
#define SENSOR_DIGITAL_PIN 27
#define SENSOR_ANALOG_PIN 33
#define LED_PIN 32
#define sensorTreshold 30 // must be set individually, based on measures of analogue sensor
#define roundPerkWh 75  // get from your counter --> 75 rounds / kWh for me --> one round 0,0133333 kWh
#define roundInOneMinuteInWatt 800 // 75 rounds have 1kWh --> 1000 Watt hours --> 60000 Watt minutes --> 1 round per minute has 800 Watt (german explanation --> http://www.simla-ev.de/files/content/Mowast_Update_18128/Leistung_ermitteln_18121.pdf )
#define delayForNextRoundDay 20 // Around 10 seconds for a delay(500)
#define delayForNextRoundNight 30 // Around 15 seconds for a delay(500)
#define toHighPowerToBeReal 4800 // Sometimes it is too slow, then I get 4800W or higher as currPower (approx. fits to a read occuring directly after the delays above). So I will ignore it. Setting delay higher can make trouble for fast turns instead :()

// Wifi
const char ssid[] = WIFI_SSID; // Define in include/Credentials.h
const char pass[] = WIFI_PASSWD; // Define in include/Credentials.h


// Variables
int value_D0, value_A0, last_value_D0; // Sensor values
int value_t1, value_t2, value_t3, value_t4; // History sensor values
int delayAfterLastRound, totalRounds, dayRounds, currDay; // working variables
float tresholdCheck, currPower, totalConsumption, dayConsumption; // working variables
long unsigned int startTimestamp, lastTimestamp, currTimestamp; // working variables


// NTP
WiFiUDP ntpUDP;
// if offset is needed replace with bottom line then.
// const long utcOffsetInSeconds = 3600;
// NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds); 
NTPClient timeClient(ntpUDP, "pool.ntp.org");


// MQTT
const char* mqttServer = MQTT_SERVER; // Define in include/Credentials.h
const char* mqttUser = MQTT_USER; // Define in include/Credentials.h
const char* mqttPassword = MQTT_PWD; // Define in include/Credentials.h
const int   mqttPort = 1883; 
const char* mqttPub = "espferraris1/status"; 

WiFiClient espClient;
PubSubClient client(espClient);


// Wifi procedures
void wifiInitialization(){
  
  if (WiFi.status() != WL_CONNECT_FAILED){
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    
    Serial.print("Checking WiFi ...");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
    }
  }

  Serial.print(" connected with IP ");
  Serial.println(WiFi.localIP());   
}


// MQTT procedures
void connectToMQTTBroker(){  
  if (!client.connected()) {
    while (!client.connected()) {
      if(client.connect("ESP8266Client", mqttUser, mqttPassword)){
        Serial.println("Connected to MQTT Broker.");
      } else {
        Serial.print("MQTT connection to broker failed with state ");
        Serial.println(client.state());
        delay(2000);
      }
      delay(100);
    }
  }
}

void sendMQTTPayload(){
  connectToMQTTBroker();
  
  const int capacity = JSON_OBJECT_SIZE(8);
  StaticJsonDocument<capacity> doc;
  doc["startTimestamp"] = startTimestamp;
  doc["updateTimestamp"] = currTimestamp;
  doc["treshholdValue"] = tresholdCheck;
  doc["updatesTotal"] = totalRounds;
  doc["updatesToday"] = dayRounds;
  doc["powerConsumptionTotal"] = totalConsumption;
  doc["powerConsumptionToday"] = dayConsumption;
  doc["currPower"] = currPower;  
  
  char JSONmessageBuffer[300];
  serializeJson(doc, JSONmessageBuffer, sizeof(JSONmessageBuffer));
  
  if (client.publish(mqttPub, JSONmessageBuffer, true)){
    Serial.print("Send successfull to MQTT Broker: ");
    Serial.println(JSONmessageBuffer);
    Serial.println();
  } else {
    Serial.print("Error publishing MQTT message.");
  }
}

void mqttInitialization(){
  client.setServer(mqttServer, mqttPort); 
  connectToMQTTBroker(); 
}

void ntpTimeInitialization(){
  startTimestamp = 0; 
  timeClient.setUpdateInterval(43200); 
  timeClient.begin();
  timeClient.update();
  startTimestamp = timeClient.getEpochTime();
  while (startTimestamp < 1609502400) {
    timeClient.update();
    startTimestamp = timeClient.getEpochTime();
    Serial.println("Wait for valid NTP time.");
    delay(3000);
  }
  
  Serial.println(startTimestamp);
  lastTimestamp = 0;
  currTimestamp = 0;
}

// Setup
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(SENSOR_DIGITAL_PIN, INPUT);
  pinMode(SENSOR_ANALOG_PIN, INPUT); 

  wifiInitialization();

  mqttInitialization(); 

  ntpTimeInitialization(); 

  // Default values
  value_t1 = 0;
  value_t2 = 0;
  value_t3 = 0;
  value_t4 = 0;
  dayConsumption = 0;
  dayRounds = 0;
  totalConsumption = 0;
  totalRounds = 0;
  delayAfterLastRound = 0;  
}

// Loop
void loop() { 
  float distance = 0;    
  int i = 1; 
     
  // Get digital input. Not used anymore in the following code
  value_D0 = digitalRead(SENSOR_DIGITAL_PIN); 
     
  // Get avg. distance by 50 measurements for better precision.
  // Original code used from here: https://wiki.hshl.de/wiki/index.php/Abstands-_und_Farberkennungssensor:_TCRT5000
  for (i = 1; i <= 50; i++)
  {
    value_A0 = analogRead(SENSOR_ANALOG_PIN); 
    distance = distance + value_A0;
  }
  distance = distance / i; 
         
  // Check if distance is higher than treshold
  // tresholdCheck = ((distance + value_t1 + value_t2 + value_t3 + value_t4) / 5);  // Could make sense if sensors data is close to normal data. Replace with bottom line then.
  tresholdCheck = distance;
  if ((tresholdCheck >= sensorTreshold) && (delayAfterLastRound <= 0)) {            

    // Calculate current power
    timeClient.update();
    currTimestamp = timeClient.getEpochTime();
    if (lastTimestamp > 0) {  
      currPower = (float)60 * (float)roundInOneMinuteInWatt / (currTimestamp - lastTimestamp); // Current power in Watt
    }
                
    // Day bill check
    timeClient.setTimeOffset(3600); // I need day check in GMT here, but epcoh vai MQTT is UTC
    if (timeClient.getDay() != currDay){
      dayConsumption = 0;
      dayRounds = 0;
      currDay = timeClient.getDay();
    }

    // Longer delay in night hours to prevent false measurements
    if ((timeClient.getHours() >= 22) || (timeClient.getHours() <= 7)){
      delayAfterLastRound = delayForNextRoundNight;
    } else{
      delayAfterLastRound = delayForNextRoundDay;
    }
    timeClient.setTimeOffset(0);
    
    // Check if maybe false detection
    if (currPower < toHighPowerToBeReal) {

      // Consumption calculation
      lastTimestamp = currTimestamp;
      totalRounds += 1; // Total rounds since start
      totalConsumption = totalRounds / (float)roundPerkWh;  // Total consumption since start
      dayRounds   += 1; // Rounds since midnight
      dayConsumption = dayRounds / (float)roundPerkWh;  // Daily consumption
      
      // Payload & LED
      sendMQTTPayload();  
      digitalWrite(LED_PIN, HIGH);   
    }
  }   
 
  // Monitor output
  Serial.print(value_D0);
  Serial.print(" ");
  Serial.print(value_A0);
  Serial.print(" ");
  Serial.print(distance);
  Serial.print(" ");
  Serial.print(tresholdCheck);
  Serial.print(" ");   
  Serial.print(currPower);   
  Serial.println();

  // History values if needed above
  value_t4 = value_t3;
  value_t3 = value_t2;
  value_t2 = value_t1;
  value_t1 = distance;

  // Delay control after last positive read
  if (delayAfterLastRound > 0) {
    delayAfterLastRound -= 1;    
    if (delayAfterLastRound < 10) {
      digitalWrite(LED_PIN, LOW);
    }
  } 

  delay(500);
}