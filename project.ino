#include <PubSubClient.h>
#include <Wire.h>
#include <ThingSpeak.h>
#include <SHT2x.h>
#include <WebServer.h>
#include <WiFi.h>

char ssid[] = "Nikhil";
char pass[] = "Pandu@123";
int soil_moisture_pin = 33;
int water_sensor = 34;
WebServer onServer(80);
int relay=5;
int buzzer=27;
int check=0;
int buzzcheck=0;
SHT2x sht20;
float Temperature;
int moisture_percentage;
int water_percent;
#define sda_pin 21
#define scl_pin 22

void connectWifi() {
  Serial.print("Connecting to Wi-Fi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 5) {
    WiFi.begin(ssid, pass);
    delay(2000);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi.");
  } else {
    Serial.println("Failed to connect to Wi-Fi. Please check your credentials or connection.");
  }
}

#define mqttPort 1883
WiFiClient client;
const char* server = "mqtt3.thingspeak.com";
int status = WL_IDLE_STATUS;
long lastPublishMillis = 0;
long lastwatered=0;
long lastbuzz=0;
int updateInterval = 60;
int waterinterval=3600;
int buzzinterval=1200;
PubSubClient mqttClient(client);

#define channelID 2501522
const char mqttUserName[] = "GDc1HAMeGSMoJDcmMCYHDiY";
const char clientID[] = "GDc1HAMeGSMoJDcmMCYHDiY";
const char mqttPass[] = "YF9xXG8kM5Lx4RJ1XiWNLusL";

void mqttConnect() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect(clientID, mqttUserName, mqttPass)) {
      Serial.print("MQTT connected to ");
      Serial.print(server);
      Serial.print(" at port ");
      Serial.println(mqttPort);
      mqttSubscribe(channelID);
    } else {
      Serial.print("MQTT connection failed, rc = ");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void mqttSubscriptionCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void mqttSubscribe(long subChannelID) {
  String myTopic = "channels/" + String(subChannelID) + "/subscribe";
  mqttClient.subscribe(myTopic.c_str());
}

void mqttPublish(long pubChannelID, String field1, String value1, String field2, String value2, String field3, String value3) {
  String message = field1 + "=" + value1 + "&" + field2 + "=" + value2 + "&" + field3 + "=" + value3;
  String topicString = "channels/" + String(pubChannelID) + "/publish";
  mqttClient.publish(topicString.c_str(), message.c_str());
}


void setup() {
  Serial.begin(115200);
  pinMode(relay,OUTPUT);
  digitalWrite(relay,LOW);
  pinMode(buzzer,OUTPUT);
  digitalWrite(buzzer,HIGH);
  pinMode(soil_moisture_pin, INPUT);
  pinMode(water_sensor, INPUT);
  delay(3000);
  connectWifi();
  mqttClient.setServer(server, mqttPort);
  mqttClient.setCallback(mqttSubscriptionCallback);
  mqttClient.setBufferSize(2048);
  Wire.begin(sda_pin, scl_pin);
  if (!sht20.begin()) {
    Serial.println("Failed to initialize SHT20 sensor!");
    while (1);
  }
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  onServer.on("/", handleRoot);  
  onServer.on("/H", Watering);
  onServer.begin();
  Serial.println("HTTP server started");
}
void handleRoot() {
  digitalWrite(relay, LOW);
  String responseHTML = "<!DOCTYPE html><html><head><title>ESP32 Sensor Data</title>";
  responseHTML += "<script>setInterval(function() { location.reload(); }, 5000);</script>"; // JavaScript for refreshing
  responseHTML += "</head><body>";
  responseHTML += "<h1>ESP32 Sensor Data</h1>";
  responseHTML += "<p>Moisture Level: " + String(moisture_percentage) + "</p>";
  responseHTML += "<p>Temperature: " + String(Temperature) + "</p>";
  responseHTML += "<p>Water Level: " + String(water_percent) + "</p>";
  responseHTML += "<button><a href='/H'>Manual Watering Button</a></button>";
  responseHTML += "</body></html>";
  onServer.send(200, "text/html", responseHTML);
}


void Watering() {
  digitalWrite(relay, HIGH);
  onServer.send(200, "text/html", "<h1>The plant is being watered</h1><button><a href='/'>Stop Watering the plant</a></button>");
}

void loop() {
  onServer.handleClient();
  digitalWrite(buzzer,HIGH);
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }

  if (!mqttClient.connected()) {
    mqttConnect();
  }

  mqttClient.loop();
  sht20.read();
  if (abs(long(millis()) - lastPublishMillis) > updateInterval * 1000) {
    int moisture_level = analogRead(soil_moisture_pin);
    int water_level = analogRead(water_sensor);
    Temperature = sht20.getTemperature();
    moisture_percentage=map(moisture_level,0,4095,0,100);
    moisture_percentage=100-moisture_percentage;
    water_percent=map(water_level,0,2600,0,100);
    Serial.print("Soil Moisture Level: ");
    Serial.println(moisture_percentage);
    Serial.print("Temperature: ");
    Serial.println(Temperature);
    Serial.print("Water_level: ");
    Serial.println(water_percent);
    mqttPublish(channelID, "field1",String(moisture_percentage) ,"field2",String(Temperature),"field3",String(water_percent));
    lastPublishMillis = millis();
    if(abs(long(millis())-lastwatered)>waterinterval*1000 || check==0)
    {
      if(moisture_percentage<=50)
      {
        if(Temperature>=40)
        {
          digitalWrite(relay,HIGH);
          delay(7500);
          digitalWrite(relay,LOW);
        }
        else if(Temperature>=36 && Temperature<40)
        {
          digitalWrite(relay,HIGH);
          delay(5000);
          digitalWrite(relay,LOW);
        }
        else if(Temperature>=32 && Temperature<36)
        {
          digitalWrite(relay,HIGH);
          delay(4000);
          digitalWrite(relay,LOW);
        }
        else if(Temperature>=26 && Temperature<32)
        {
          digitalWrite(relay,HIGH);
          delay(3500);
          digitalWrite(relay,LOW);
        }
        else if(Temperature<26)
        {
          digitalWrite(relay,HIGH);
          delay(3000);
          digitalWrite(relay,LOW);
        }
        lastwatered=millis();
        check=1;
      }
    }
    if(abs(long(millis())-lastbuzz)>buzzinterval*1000 || buzzcheck==0)
    {
      if(water_percent<15)
      {
        digitalWrite(buzzer,LOW);
        delay(1500);
        digitalWrite(buzzer,HIGH);
        delay(500);
        digitalWrite(buzzer,LOW);
        delay(1500);
        digitalWrite(buzzer,HIGH);
        delay(500);
        digitalWrite(buzzer,LOW);
        delay(1500);
        digitalWrite(buzzer,HIGH);
        buzzcheck=1;
        lastbuzz=millis();
      }
      else if(water_percent<30)
      {
        digitalWrite(buzzer,LOW);
        delay(2000);
        digitalWrite(buzzer,HIGH);
        delay(500);
        digitalWrite(buzzer,LOW);
        delay(2000);
        digitalWrite(buzzer,HIGH);
        buzzcheck=1;
        lastbuzz=millis();
      }
      else if(water_percent<50)
      {
        digitalWrite(buzzer,LOW);
        delay(3000);
        digitalWrite(buzzer,HIGH);
        buzzcheck=1;
        lastbuzz=millis();
      }
    }
  }
}