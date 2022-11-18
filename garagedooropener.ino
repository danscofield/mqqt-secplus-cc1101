#include "secrets.h"
#include "constants.h"
#include "radiostuff.h"

#include <WiFiClientSecure.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"




WiFiClientSecure net = WiFiClientSecure();
MqttClient awsmqtt(net);

const int hb_every_n_tick = 10; // send a heartbeat every n-th iteration of the loop
int hb_counter = 0;


int connectWiFi(int max_retries) {
  int counter = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);


  while (WiFi.status() != WL_CONNECTED && counter < max_retries) {
    delay(500);
    counter++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    delay(500);
    return 1;
  }
  return -1;
}

int connectAWS(int max_retries) {
  int counter = 0;
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  while (counter < max_retries) {
    counter++;
    if (awsmqtt.connect(AWS_IOT_ENDPOINT, AWS_IOT_PORT)) {
      sleep(1);
      awsmqtt.onMessage(messageHandler);
      awsmqtt.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
      return 1;

    } else {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(awsmqtt.connectError());
    }
    delay(500);
  }
  return -1;
}

void handle_request(unsigned int fixed, unsigned int rolling, const char* request_id) {
  uint8_t secplus_buffer[226] = { 0 };
  memset(secplus_buffer, 0x0, sizeof(secplus_buffer));

  build_secplus_packet(rolling, fixed, secplus_buffer, sizeof(secplus_buffer));

  StaticJsonDocument<200> response;
  char buff[512];
  memset(buff, 0x0, sizeof(buff));
  response["request_id"] = request_id;
  response["fixed"] = fixed;
  response["rolling"] = rolling;
  serializeJson(response, buff, sizeof(buff));
  publish(AWS_IOT_PUBLISH_TOPIC, buff);
}



void messageHandler(int messageSize) {
  String mqqt_message = awsmqtt.readStringUntil('\0');
  StaticJsonDocument<512> inbound_message;
  deserializeJson(inbound_message, mqqt_message);
  if (inbound_message.containsKey("fixed") && inbound_message.containsKey("rolling"), inbound_message.containsKey("version"), inbound_message.containsKey("request_id")) {
    const unsigned int fixed = inbound_message["fixed"];
    const unsigned int rolling = inbound_message["rolling"];
    const bool v1 = inbound_message["version"] == 2 ? false : true;
    const char* request_id = inbound_message["request_id"];
    Serial.printf("[%s]: %s -- tx (%d, %d)\n", request_id, v1 ? "Secplus 1.0" : "Secplus 2.0", fixed, rolling);
    // there is a todo here to actually imlpement v2
    handle_request(fixed, rolling, request_id);
  }
}

void publish(char* topic, char* message) {
  awsmqtt.beginMessage(topic);
  awsmqtt.print(message);
  awsmqtt.endMessage();
}
void send_healthcheck() {
  Serial.println("Sending Health Check");
  StaticJsonDocument<200> doc;
  char jsonBuffer[512];
  memset(jsonBuffer, 0x0, sizeof(jsonBuffer));
  doc["device_id"] = HEALTHCHECK_DEVICE_NAME;
  serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
  publish(AWS_IOT_HEALTHCHECK_TOPIC, jsonBuffer);
}

void setup() {
  sleep(5);
  configure_radio();
  Serial.begin(9600);
}

void loop() {
  delay(1000);
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi(10);
    if (WiFi.status() != WL_CONNECTED) {
      Serial.printf("Cant connect to wifi, restarting loop\n");
      return;
    }
  }
  if (!awsmqtt.connected()) {
    connectAWS(10);
    if (!awsmqtt.connected()) {
      Serial.printf("Cant connect to aws, restarting loop\n");
      return;
    }
  }

  awsmqtt.poll();
  hb_counter++;
  if (hb_counter >= hb_every_n_tick) {
    send_healthcheck();
    hb_counter = 0;
  }
}