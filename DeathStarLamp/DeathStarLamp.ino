/*
 * Death Star Lamp
 * 
 * Firmware to connect my Death Star lamp device to the cloud using a MKR1000.
 * Subscribes to a real-time messaging channel on PubNub and parses messages.
 * Adds logic to turn lamp on/off and move stepper motor in between ten levels.
 * 
 * You need to add your WiFi and device details in lines 30-32.
 * 
 * Signal outputs on the Arduino MKR1000:
 *    - Pin 2 --> Motor driver / pin 8
 *    - Pin 3 --> Motor driver / pin 7
 *    - Pin 4 --> Motor driver / pin 6
 *    - Pin 5 --> Motor driver / pin 5
 *    - Pin 7 --> SSR relay / CH1
 * 
 * Created Apr 1, 2018
 * By Adi Singh
 * 
 * https://github.com/adi3/death_star_lamp
 *
 */

#include <WiFi101.h>
#include <Stepper.h>
#include <ArduinoJson.h>
#include <FlashAsEEPROM.h>

/*********************** This portion to be filled in *****************************/
#define WIFI_SSID       "WIFI_SSID"               // your WiFi connection name
#define WIFI_PASSWORD   "WIFI_PASSWORD"           // your WiFi connection password
#define DEVICE_ID       "DEVICE_ID"               // device ID provided by Alexa
/**********************************************************************************/

const char* server = "ps.pndsn.com";

const int dirPin = 2;
const int stepPin = 3;
const int sleepPin = 4;
const int resetPin = 5;
const int relayPin = 7;
const int STEPS_PER_LVL = 2800;

String timeToken = "0";
WiFiClient client;
int currGlow;


/* 
 *  Set output pins and connect to WiFi.
 */
void setup(void){
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(dirPin,OUTPUT);
  pinMode(stepPin,OUTPUT); 

  pinMode(sleepPin, OUTPUT);
  digitalWrite(sleepPin, LOW);
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, HIGH);
  
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  
  Serial.begin(9600);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Turn on LED once online
  digitalWrite(LED_BUILTIN, HIGH);
  
  initGlow();
}


/* 
 *  Get stored glow to sync with actual lamp level.
 */
void initGlow() {
  if (!EEPROM.isValid()) {
    Serial.println("Initing EEPROM...");
    EEPROM.write(0,0);
    EEPROM.commit();
  }
  currGlow = (int)EEPROM.read(0);
  Serial.println(currGlow);
}


/* 
 *  Subscribe to pubnub and handle messages.
 */
void loop(void){
  if (client.connect(server, 80)) {
    Serial.println("Reconnecting...");

    String url = "/subscribe";
    url += "/sub-c-6cb01a82-feaf-11e6-a656-02ee2ddab7fe";
    url += "/";
    url += DEVICE_ID;
    url += "/0/";
    url += timeToken;
    Serial.println(url);
  
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(100);

    while (client.available()) {
      String line = client.readStringUntil('\r');
      if (line.endsWith("]")) {
        Serial.println(line);
        cmdHandler(strParser(line));
      }
    }
  } else {
    Serial.println("Pubnub Connection Failed");
  }
  
  Serial.println("Closing connection");
  client.stop();
  delay(1000);
}


/* 
 *  Parse commands and invoke relevant function.
 */
uint8_t cmdHandler(String pJson) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& parsedJson = jsonBuffer.parseObject(pJson);
  if (!parsedJson.success())
  {
    Serial.println("Parsing failed.");
    return 0;
  }
  String param = parsedJson["param"];
  String cmd = parsedJson["cmd"];
  Serial.println(param);
  Serial.println(cmd);

  if (param == "state") {
    handleState(cmd);
  } else if (param == "glow") {
    handleGlow(cmd);
  } else if (param == "effects") {
    handleEffects(cmd);
  }
}


/* 
 *  Switch the bulb on or off via solid-state relay.
 */
void handleState(String cmd) {
  if (cmd == "1") {
    digitalWrite(relayPin, LOW);
  } else {
    digitalWrite(relayPin, HIGH);
  }
}


/*
 *  Turn stepper for opening or closing lamp to designated level.
 */
void handleGlow(String cmd) {
  int glow = cmd.toInt();
  if (glow == currGlow) {
    Serial.println("Glow level already at " + cmd);
  } else {
    setMotorDirection(glow);
    int steps = getMotorSteps(glow);
    moveMotor(steps);
    storeGlowLevel(glow);
  }
}


/* 
 *  Determine turn direction by comparing current and designated levels.
 */
void setMotorDirection(int glow) {
  int sig = (currGlow > glow) ? HIGH : LOW;
  digitalWrite(dirPin, sig);
  Serial.println("Dir: " + sig);
}


/* 
 *  Determine number of steps needed to reach designated level.
 */
int getMotorSteps(int glow) {
  int delta = abs(currGlow - glow);
  Serial.println(STEPS_PER_LVL * delta);
  Serial.println("-----");
  return STEPS_PER_LVL * delta;
}


/* 
 *  Wake stepper, then move in set direction by given number of steps.
 */
void moveMotor(int steps) {
  digitalWrite(sleepPin, HIGH);
  delay(10);
  
  for (int i=0; i <= steps; i++){
    digitalWrite(stepPin,HIGH); 
    delayMicroseconds(500); 
    digitalWrite(stepPin,LOW); 
    delayMicroseconds(500);
  }
  digitalWrite(sleepPin, LOW);
}


/* 
 *  Save glow level to sync firmware and hardware after device reboot. 
 */
void storeGlowLevel(int glow) {
  currGlow = glow;
  EEPROM.write(0,currGlow);
  EEPROM.commit();
  Serial.println("EEPROM data: " + String(EEPROM.read(0)));
}


/* 
 *  A quirky feature for a cool light-n-sound show.
 *  Alexa plays the Imperial March during this function.
 */
void handleEffects(String cmd) {
  flicker(7);
  handleGlow("6");
  flicker(7);
  handleGlow("4");
  flicker(2);
  delay(3000);
  handleGlow("10");
  flicker(3);
}


/* 
 *  Random pauses between successive on/offs to induce flickering.
 */
void flicker(int time) {
  for (int i=0; i<(2*time); i++) {
    delay(random(100, 400));
    handleState("1");
    delay(random(100, 400));
    handleState("0");
  }
  // always leave flicker mode with light on
  handleState("1");
}


/* 
 *  Parse raw pubnub stream data into structured JSON.
 */
String strParser(String data) {
  char buffer[200];
  memset(buffer, '\0', 200);

  int count = 0;
  String json;

  typedef enum {
    STATE_INIT = 0,
    STATE_START_JSON,
    STATE_END_JSON,
    STATE_SKIP_JSON,
    STATE_INIT_TIME_TOKEN,
    STATE_START_TIME_TOKEN,
    STATE_END_TIME_TOKEN
  } STATE;
  STATE state = STATE_INIT;

  data.toCharArray(buffer, 200);

  for (int i = 1; i <= strlen(buffer); i++) {
    if (buffer[i] == '[' && buffer[i + 2] == '{') {
      state = STATE_START_JSON;
      json = "\0";
      continue;
    }
    else if (buffer[i] == '[' && buffer[i + 4] == '"') {
      state = STATE_INIT_TIME_TOKEN;
      json = "{}";
      timeToken = "\0";
      continue;
    }
    switch (state) {
      case STATE_START_JSON:
        if (buffer[i] == '[') {
          break;
        }
        else if (buffer[i] == '{') {
          count++;
          json += buffer[i];
        }
        else if (buffer[i] == '}') {
          count--;
          json += buffer[i];
          if (count <= 0) {
            state = STATE_END_JSON;
          }
        }
        else {
          json += buffer[i];
          if (buffer[i] == '}') {
            state = STATE_END_JSON;
          }
        }
        break;
      case STATE_END_JSON:
        if (buffer[i] == ']' && buffer[i + 2] == '"') {
          state = STATE_INIT_TIME_TOKEN;
          timeToken = "\0";
        }
        break;
      case STATE_INIT_TIME_TOKEN:
        if (buffer[i] == '"') {
          state = STATE_START_TIME_TOKEN;
          timeToken = "\0";
        }
        break;
      case STATE_START_TIME_TOKEN:
        if (buffer[i] == '"' && buffer[i + 1] == ']') {
          state = STATE_END_TIME_TOKEN;
          break;
        }
        timeToken += buffer[i];
        break;
      case STATE_END_TIME_TOKEN:
        break;
      default:
        break;
    }
  }
  return json;
}

