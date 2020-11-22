#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHTsens.h>
#include <capMoisture.h>
#include <LightSens.h>

#define MSG_BUFFER_SIZE 50
#define DHTPIN D5
#define DHTTYPE DHT11
#define DHTUNITS 0    // Fahrenheit
#define LSPIN D0      // pin for energize photoresistor
//ADC pins
#define MINTPIN 0
#define BASILPIN 1
#define LIGHTPIN 2

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;     /* Use thi for the 12-bit version */

capMoisture mintMstr(&ads,MINTPIN); // Initialize moisture sensor for mint
capMoisture basilMstr(&ads,BASILPIN); // initialize moisture sensor for basil
DHTsens th1 = DHTsens(DHTPIN,DHTTYPE,DHTUNITS); // Initialize Temp/Humidity Sensor
LightSens ls1 = LightSens(&ads,LIGHTPIN,LSPIN); // Initialize light sensor


// Update these with values suitable for your network.

const char* ssid = "ManigliaNet";
const char* password = "TLU-723_WS";
const char* mqtt_server = "192.168.1.201";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("Maniglia_out", "hello world");
      // ... and resubscribe
      client.subscribe("Maniglia_in");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() 
{
  setup_wifi();
  Serial.begin(115200);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);  
  Serial.println("Hello!");
  
  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");

  ads.begin();
  th1.setup();
  ls1.setup();
}

void loop() 
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //Read Sensor Values
  mintMstr.read(); // Read moisture sensor #1
  Serial.print("Moisure Voltage: "); Serial.println(mintMstr.val); // Print moisture voltage to Serial
  basilMstr.read(); // Read moisture sensor #2
  Serial.print("Moisure Voltage: "); Serial.println(basilMstr.val); // Print moisture voltage to Serial
  th1.read(); // Take temp and humidity reading from T&H sensor #1
  Serial.print("Temperature (F): "); Serial.println(th1.t); // Print temperature to Serial
  Serial.print("Humidity: "); Serial.println(th1.h); // Print humidity to Serial
  Serial.print("Heat Index (F): "); Serial.println(th1.ind); // Print heat index to Serial
  ls1.read(); // Take a photoresistor reading
  Serial.print("Photoresistor Resistance = "); Serial.print(ls1.val); Serial.println(" ohms");
  // publish sensor values to MQTT Broker topics
  client.publish("Sensor1/mint/moisture",mintMstr.msg);
  
  client.publish("Sensor1/basil/moisture",basilMstr.msg);

  client.publish("Sensor1/temp",th1.tMsg);

  client.publish("Sensor1/humidity",th1.hMsg); 

  client.publish("Sensor1/heat_index",th1.indMsg);  

  client.publish("Sensor1/light",ls1.msg);
  
  delay(5000);
}
