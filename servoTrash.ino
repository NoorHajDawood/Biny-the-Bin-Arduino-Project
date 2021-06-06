#include <ESP32Servo.h> //servo library
Servo servo;
// i/o pins
int trigPin = 27;
int echoPin = 14;
int servoPin = 25;
int redLED = 26;
int led = 10;
// distance vars
long duration, dist, average;
long aver[3]; //array for average

//#include <ESP8266WiFi.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/********* WiFi Access Point ***********/

#define WLAN_SSID "Shenkar-New"
#define WLAN_PASS "Shenkarwifi"

/********* Adafruit.io Setup ***********/

#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883 // use 8883 for SSL
#define AIO_USERNAME "NoorX"
#define AIO_KEY "aio_KFxV53xCPxN8NZDzx4Bv2yC67Pph"

/**** Global State (you don't need to change this!) ******/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/********** Feeds *************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish light_level = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/trash-distance-servo");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe distance_sub = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/trash-distance-servo");

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

/********* WiFi Access Point ***********/

#define WLAN_SSID "Shenkar-New"
#define WLAN_PASS "Shenkarwifi"

/********* Adafruit.io Setup ***********/

#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883 // use 8883 for SSL
#define AIO_USERNAME "NoorX"
#define AIO_KEY "aio_KFxV53xCPxN8NZDzx4Bv2yC67Pph"

void setup()
{
  Serial.begin(9600);
  Serial.println(F("Starting..."));
  delay(1000);
  Serial.println(F("\n\n##################################"));
  Serial.println(F("Trash"));
  Serial.println(F("Noor & Bader & ADI & NOY"));
  Serial.println(F("##################################"));
  // Connect to WiFi access point.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  // SETUP ALL THE SUBSRCIPTIONS HERE
  mqtt.subscribe(&distance_sub); // Setup MQTT subscription for onoff feed.

  // setup pins
  servo.attach(servoPin);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(redLED, OUTPUT);
  servo.write(0); //close cap on power on
  delay(100);
  servo.detach();
}

void measure()  // measure distance detected by sensor
{
  digitalWrite(10, HIGH);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(15);
  digitalWrite(trigPin, LOW);
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
  dist = (duration / 2) / 29.1; //obtain distance
}

void loop()
{
  // We must keep this for now
  MQTT_connect();

  int flag = 0;

  // getting average distance from sensor to prevent false-positives
  for (int i = 0; i <= 2; i++)
  { //average distance
    measure();
    aver[i] = dist;
    delay(10); //delay between measurements
  }
  dist = (aver[0] + aver[1] + aver[2]) / 3;

  // condition to open trash's cover
  if (dist < 50)
  {
    digitalWrite(redLED, LOW); // When the Red condition is met, the Green LED should turn off digitalWrite(redLED,LOW);
    Serial.print(dist);
    Serial.println(" cm");
    int feedDist = dist;
    // upload to cloud that cover got opened
    light_level.publish(feedDist);
    //Change distance as per your need
    servo.attach(servoPin);
    delay(1);
    servo.write(0);
    delay(3000);
    servo.write(150);
    delay(1000);
    servo.detach();
    flag = 0;
  }
  else
  {
    flag++;
  }
  if (flag == 1)  // publish only once for closed cover while it didn't open
  {
    light_level.publish(0);
    delay(2000);
  }

  // log to serial monitor
  if (dist >= 200 || dist <= 0)
  {
    Serial.println("Out of range");
  }
  else
  {
    Serial.print(dist);
    Serial.println(" cm");
  }

  // reading from cloud's subscription
  Serial.println(F("Reading From Cloud:"));
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(500)))
  {
    if (subscription == &distance_sub)
    {
      Serial.print(F("Got: "));
      Serial.println((char *)distance_sub.lastread);
    }
  }
}

void MQTT_connect()
{
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected())
  {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0)
  { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000); // wait 5 seconds
    retries--;
    if (retries == 0)
    {
      // basically die and wait for WDT to reset me
      while (1)
        ;
    }
  }
  Serial.println("MQTT Connected!");
}
