#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

TaskHandle_t temperature_task;
TaskHandle_t humidity_task;
TaskHandle_t transfer_task;
TaskHandle_t led_task;

float temperature;
float humidity;

SemaphoreHandle_t mutex;

const char* ssid = "Nguyen Phuong Duy";
const char* password = "12345678";

#define MQTT_SERVER "broker.mqttdashboard.com"
#define MQTT_PORT 1883
#define MQTT_USER "ohtech.vn"
#define MQTT_PASSWORD "66668888"
#define MQTT_TEMPERATURE_TOPIC "nodeWiFi32/dht11/temperature"
#define MQTT_HUMIDITY_TOPIC "nodeWiFi32/dht11/humidity"
#define MQTT_LED_TOPIC "nodeWiFi32/led"

#define LEDPIN 25
#define DHTPIN 17
#define DHTTYPE DHT11

int current_ledState = LOW;
int last_ledState = LOW;

WiFiClient wifiClient;
PubSubClient client(wifiClient);
DHT dht(DHTPIN, DHTTYPE);

void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
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

void connect_to_broker() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "nodeWiFi32";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      client.subscribe(MQTT_LED_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void callback(char* topic, byte *payload, unsigned int length) {
  Serial.println("-------new message from broker-----");
  Serial.print("topic: ");
  Serial.println(topic);
  Serial.print("message: ");
  Serial.write(payload, length);
  Serial.println();
  if (*payload == '1') current_ledState = HIGH;
  if (*payload == '0') current_ledState = LOW;
}


void setup() {
  Serial.begin(115200);
  //Serial.setTimeout(500);
  mutex = xSemaphoreCreateMutex();
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT );
  client.setCallback(callback);
  connect_to_broker();
  dht.begin();
  digitalWrite(LEDPIN, current_ledState);
  delay(500);

  xTaskCreatePinnedToCore(
                    temperature_code,   /* Task function. */
                    "TemperatureTask",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    4,           /* priority of the task */
                    &temperature_task,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */                  
  delay(500); 

  xTaskCreatePinnedToCore(
                    humidity_code,   /* Task function. */
                    "HumidityTask",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    3,           /* priority of the task */
                    &humidity_task,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */     
  delay(500);

  xTaskCreatePinnedToCore(
                    led_code,   /* Task function. */
                    "LedTask",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    2,           /* priority of the task */
                    &led_task,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */     
  delay(500);

  xTaskCreatePinnedToCore(
                    transfer_code,   /* Task function. */
                    "TransferTask",     /* name of task. */
                    100000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &transfer_task,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */     
  delay(500);
}

/***********************************************/
void temperature_code(void *parameter)
{
  for(;;){
    temperature = dht.readTemperature();
    Serial.print("Get Temperature: ");
    Serial.println(temperature);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

/**/
void humidity_code(void *parameter)
{
  for(;;){
    humidity = dht.readHumidity(); 
    Serial.print("Get Humidity: ");
    Serial.println(humidity);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

/**/
void led_code (void *parameter)
{
  for(;;){
    Serial.println("In Led code");
    if (last_ledState != current_ledState) {
      last_ledState = current_ledState;
      digitalWrite(LEDPIN, current_ledState);
      Serial.print("LED state is ");
      Serial.println(current_ledState);
    }
      vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

/**/
void transfer_code(void *parameter)
{
  for(;;){
    Serial.println("In transfer code");
    client.loop();
    if (!client.connected()) {
      xSemaphoreTake(mutex, portMAX_DELAY);
      connect_to_broker();
      vTaskDelay(2500 / portTICK_PERIOD_MS);
      xSemaphoreGive(mutex);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }  
    
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
    } 
    else {
        client.publish(MQTT_TEMPERATURE_TOPIC, String(temperature).c_str());
        client.publish(MQTT_HUMIDITY_TOPIC, String(humidity).c_str());
    }
    Serial.println("Done transfer code");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}
/***********************************************/

void loop() {
}