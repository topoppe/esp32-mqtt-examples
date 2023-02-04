#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>

int ledPin = 19;

// WiFi-Credentials
const char *ssid = "HSRW-FabLab";
const char *password = "57289887252221648185";

// MQTT Broker IP address
const char *mqtt_identifier = "tobias_1";
const char *mqtt_server = "192.168.188.10";
const char *mqtt_user = "mqtt-user";
const char *mqtt_pass = "mqtt";

int sending_mqtt_every_ms = 5000;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
char topic[50];
int value = 0;

Adafruit_BME280 bme; // I2C
float temperature = 0;
float humidity = 0;

void setup_wifi()
{
    delay(10);
    // We start by connecting to a WiFi network
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
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();

    // Feel free to add more if statements to control more GPIOs with MQTT

    // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
    // Changes the output state according to the message
    strcpy(topic, mqtt_identifier);
    strcat(topic, "/output");
    if (String(topic) == topic)
    {
        Serial.print("Changing output to ");
        if (messageTemp == "ON")
        {
            Serial.println("on");
            digitalWrite(ledPin, HIGH);
        }
        else if (messageTemp == "OFF")
        {
            Serial.println("off");
            digitalWrite(ledPin, LOW);
        }
    }
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect(mqtt_identifier, mqtt_user, mqtt_pass))
        {
            Serial.println("connected");
            // Subscribe
            strcpy(topic, mqtt_identifier);
            strcat(topic, "/output");
            client.subscribe(topic);
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup(void)
{
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    Wire.begin(21, 22);
    if (!bme.begin(0x76))
    {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1)
            ;
    }
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    long now = millis();
    if (now - lastMsg > sending_mqtt_every_ms)
    {
        lastMsg = now;

        // Temperature in Celsius
        temperature = bme.readTemperature();
        // Uncomment the next line to set temperature in Fahrenheit
        // (and comment the previous temperature line)
        // temperature = 1.8 * bme.readTemperature() + 32; // Temperature in Fahrenheit

        // Convert the value to a char array
        char tempString[8];
        dtostrf(temperature, 1, 2, tempString);
        Serial.print("Temperature: ");
        Serial.println(tempString);
        strcpy(topic, mqtt_identifier);
        strcat(topic, "/temperature");
        client.publish(topic, tempString);

        humidity = bme.readHumidity();

        // Convert the value to a char array
        char humString[8];
        dtostrf(humidity, 1, 2, humString);
        Serial.print("Humidity: ");
        Serial.println(humString);
        strcpy(topic, mqtt_identifier);
        strcat(topic, "/humidity");
        client.publish(topic, humString);
    }
}