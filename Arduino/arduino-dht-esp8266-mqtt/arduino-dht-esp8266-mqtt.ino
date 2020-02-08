#include "DHT.h"
#include <WiFiEspClient.h>
#include <WiFiEsp.h>
#include <WiFiEspUdp.h>
#include "SoftwareSerial.h"
#include <PubSubClient.h>
//#include <ThingsBoard.h>

#define WIFI_AP "jharaspi"//"YOUR_WIFI_AP"
#define WIFI_PASSWORD "01ohjaamo10"//"YOUR_WIFI_PASSWORD"

#define TOKEN "YOUR_ACCESS_TOKEN"

// DHT
#define DHTPIN 4
#define DHTTYPE DHT11

//char thingsboardServer[] = "192.168.6.1";//"YOUR_THINGSBOARD_HOST_OR_IP";
char mqttServer[] = "192.168.6.1";//"YOUR_THINGSBOARD_HOST_OR_IP";
unsigned int mqttPort = 1883;

// Initialize the Ethernet client object
WiFiEspClient espClient;

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

//ThingsBoard tb(espClient);
PubSubClient mqtt(espClient);

SoftwareSerial soft(2, 3); // RX, TX

int status = WL_IDLE_STATUS;
unsigned long lastSend;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ((char)payload[0] == '1') {
    digitalWrite(7, HIGH);
  } else {
    digitalWrite(7, LOW);
  }

}
void setup() {

  pinMode(7, OUTPUT);
  
  // initialize serial for debugging
  Serial.begin(115200);
  dht.begin();
  InitWiFi();

  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(callback);
  
  lastSend = 0;
}

void loop() {
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    while ( status != WL_CONNECTED) {
      Serial.print("Attempting to connect to WPA SSID: ");
      Serial.println(WIFI_AP);
      // Connect to WPA/WPA2 network
      status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      delay(500);
    }
    Serial.println("Connected to AP");
  }

//  if ( !tb.connected() ) {
  if ( !mqtt.connected() ) {
    reconnect();
  }

  if ( millis() - lastSend > 1000 ) { // Update and send only after 1 seconds
    getAndSendTemperatureAndHumidityData();
    lastSend = millis();
  }

  delay(100);
//  tb.loop();
  mqtt.loop();
}

void getAndSendTemperatureAndHumidityData()
{
  Serial.println("Collecting temperature data.");

  // Reading temperature or humidity takes about 250 milliseconds!
  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.println("Sending data to ThingsBoard:");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C ");

//  tb.sendTelemetryFloat("temperature", temperature);
//  tb.sendTelemetryFloat("humidity", humidity);
  mqtt.publish("temperature", (byte*) &temperature, (char)sizeof(temperature));
  mqtt.publish("humidity", (byte*) &humidity, (char)sizeof(humidity));
}

void InitWiFi()
{
  // initialize serial for ESP module
  Serial.println("Start InitWifi()");
  soft.begin(9600);
  // initialize ESP module
  WiFi.init(&soft);
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(WIFI_AP);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    delay(500);
  }
  Serial.println("Connected to AP");
  
}

void reconnect() {
  // Loop until we're reconnected
  /*
  while (!tb.connected()) {
    Serial.print("Connecting to ThingsBoard node ...");
    // Attempt to connect (clientId, username, password)
    if ( tb.connect(thingsboardServer, TOKEN) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED]" );
      Serial.println( " : retrying in 5 seconds" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
  */
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqtt.publish("hello", "hello world");
      // ... and resubscribe
      mqtt.subscribe("heatingActive");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }  
}
