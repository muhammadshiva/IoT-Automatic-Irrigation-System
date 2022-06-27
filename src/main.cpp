#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>

// Mobile
const char *ssid = "Redmi Note 9 Pro";         // sesuaikan dengan username wifi
const char *password = "internet99";           // sesuaikan dengan password wifi
const char *mqtt_server = "broker.hivemq.com"; // isikan server broker

WiFiClient espClient;
PubSubClient client(espClient);

SimpleDHT11 dht11(D7);

// Soil Moisture
const int AirValue = 751;
const int WaterValue = 355;
const int SensorPin = A0;
int soilMoistureValue = 0;
int soilmoisturepercent = 0;
int relaypin = D5;

long now = millis();
long lastMeasure = 0;
String macAddr = "";

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
  macAddr = WiFi.macAddress();
  Serial.println(macAddr);
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(macAddr.c_str()))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Mqtt Node-RED");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void connect()
{
  if (!client.connected())
  {
    reconnect();
  }
  if (!client.loop())
  {
    client.connect(macAddr.c_str());
  }
  now = millis();
}

void sensorDht11()
{
  connect();
  if (now - lastMeasure > 5000)
  {
    lastMeasure = now;
    int err = SimpleDHTErrSuccess;

    byte temperature = 0;
    byte humidity = 0;
    if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
    {
      Serial.print("Pembacaan DHT11 gagal, err=");
      Serial.println(err);
      delay(1000);
      return;
    }
    static char temperatureTemp[7];
    dtostrf(temperature, 4, 2, temperatureTemp);
    Serial.println(temperatureTemp);

    static char temperatureHumidity[7];
    dtostrf(humidity, 4, 2, temperatureHumidity);
    Serial.println(temperatureHumidity);

    client.publish("1941720005/temp", temperatureTemp);
    client.publish("1941720005/humidity", temperatureHumidity);
  }
}

void soilMoisture()
{
  connect();
  if (now - lastMeasure > 5000)
  {
    soilMoistureValue = analogRead(SensorPin); // put Sensor insert into soil
    Serial.println(soilMoistureValue);

    soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
    if (soilmoisturepercent >= 0 && soilmoisturepercent <= 100)
    {
      // lcd.setCursor(0, 1);
      // lcd.print("Soil RH : ");
      // lcd.print(soilmoisturepercent);
      static char soilTemp[7];
      dtostrf(soilmoisturepercent, 4, 2, soilTemp);
      Serial.println(soilTemp);
      client.publish("1941720005/soil", soilTemp);

      if (soilmoisturepercent >= 0 && soilmoisturepercent <= 30)
      {
        digitalWrite(relaypin, HIGH);
        client.publish("1941720005/motor", "On");
        Serial.println("Motor is ON");
      }
      else if (soilmoisturepercent > 30 && soilmoisturepercent <= 100)
      {
        digitalWrite(relaypin, LOW);
        client.publish("1941720005/motor", "Off");
        Serial.println("Motor is OFF");
      }
    }
  }
}

void loop()
{
  sensorDht11();
  soilMoisture();
}

// #define relay D5

// void setup()
// {
//   Serial.begin(115200);
//   pinMode(relay, OUTPUT);
// }

// void loop()
// {
//   digitalWrite(relay, HIGH);
//   Serial.println("Relay High");
//   delay(1000);
//   digitalWrite(relay, LOW);
//   Serial.println("Relay Low");
//   delay(1000);
// }