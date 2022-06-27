#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <SimpleDHT.h>

/*
  Global Variable
*/
// DHT 11
float temperature = 0;
float humidity = 0;

// Soil Moisture
const int AirValue = 785;
const int WaterValue = 380;
const int SensorPin = A0;
int soilMoistureValue = 0;
int soilmoisturepercent = 0;
int relaypin = D5;

SimpleDHT11 dht11(D7);
/* -----------------------------*/

/*
  Configure For Local
*/

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
// DHT Configuration
void sensorDHT01()
{
  int err = SimpleDHTErrSuccess;

  if ((err = dht11.read2(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
  {
    Serial.print("Pembacaan DHT11 gagal, err=");
    Serial.println(err);
    delay(1000);
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print((float)temperature);
  lcd.print(" C ");
  lcd.print((float)humidity);
  lcd.print(" H ");
}

// Soil Moisture Configuration
void soilMoisture01()
{
  soilMoistureValue = analogRead(SensorPin); // put Sensor insert into soil
  Serial.println(soilMoistureValue);

  soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);

  if (soilmoisturepercent >= 0 && soilmoisturepercent <= 100)
  {
    lcd.setCursor(0, 1);
    lcd.print("Soil RH : ");
    lcd.print(soilmoisturepercent);
  }

  if (soilmoisturepercent >= 0 && soilmoisturepercent <= 30)
  {
    digitalWrite(relaypin, HIGH);
    Serial.println("Motor is ON");
  }
  else if (soilmoisturepercent > 30 && soilmoisturepercent <= 100)
  {
    digitalWrite(relaypin, LOW);
    Serial.println("Motor is OFF");
  }
}

/* -----------------------------*/

/*
  Configure For NODE-RED

*/

// Mobile
const char *ssid = "Redmi Note 9 Pro";         // sesuaikan dengan username wifi
const char *password = "internet99";           // sesuaikan dengan password wifi
const char *mqtt_server = "broker.hivemq.com"; // isikan server broker

WiFiClient espClient;
PubSubClient client(espClient);

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

void sensorDHT02()
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

void soilMoisture02()
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

void setup()
{
  Serial.begin(115200);
  // Local
  pinMode(relaypin, OUTPUT);
  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.clear();
  lcd.home();

  // Node-RED
  Serial.println("Mqtt Node-RED");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop()
{
  // Local
  sensorDHT01();
  soilMoisture01();

  // Node-RED
  sensorDHT02();
  soilMoisture02();
}
