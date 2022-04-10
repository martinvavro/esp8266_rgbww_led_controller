#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

// -----------------------------------------------------

#define COLD D2
#define WARM D3
#define BLUE D4
#define GREEN D5
#define RED D6
#define PUSHBUTTON D7
#define WIFI_AP_SSID ""
#define WIFI_AP_PASS ""
#define DEBOUNCE 200

// -----------------------------------------------------

// For debugging messages uncomment the line below
// #define DEBUG

// -----------------------------------------------------

boolean CURRENT_STATE = false;
int OVERRIDE = 0;
time_t LAST_BUTTON_PRESS_TIME;
int COLOR_VALUES[5] = {0, 0, 0, 0, 0};
uint8_t COLOR_PINS[5] = {
    COLD,
    WARM,
    RED,
    GREEN,
    BLUE};

enum colors
{
  cold,
  warm,
  red,
  green,
  blue
} color;
ESP8266WebServer server(80);
ESP8266WiFiMulti wifiMulti;

// -----------------------------------------------------

// values between 0 -  255
void changeColorIntensity(int color, int intensity)
{
  COLOR_VALUES[color] = intensity;
  if (COLOR_VALUES[color] > 255)
  {
    COLOR_VALUES[color] = 255;
  }

  if (COLOR_VALUES[color] < 0)
  {
    COLOR_VALUES[color] = 0;
  }
}

void writePwmLed(int color)
{
  analogWrite(COLOR_PINS[color], COLOR_VALUES[color]);
}

void writeAllLeds()
{
  for (int i = 0; i < 5; i++)
  {
    writePwmLed(i);
  }
}

void turnLightsOff()
{
  for (int i = 0; i < 5; i++)
  {
    analogWrite(COLOR_PINS[i], 0);
  }
}

void turnJustWarmWhiteLedOn()
{
  analogWrite(COLOR_PINS[warm], 200);
}

void blinkTwice(int color)
{
  turnLightsOff();
  for (int i = 0; i < 2; i++)
  {
    for (int j = 25; j < 60; j++)
    {
      analogWrite(COLOR_PINS[color], j);
    }
    delay(100);
    analogWrite(COLOR_PINS[color], 0);
    delay(100);
  }
}

void handleColorArgs()
{
  for (int i = 0; i < server.args(); i++)
  {
    int value = server.arg(i).toInt();
    String argument = server.argName(i);
    if (argument.equals("red"))
      changeColorIntensity(red, value);
    if (argument.equals("green"))
      changeColorIntensity(green, value);
    if (argument.equals("blue"))
      changeColorIntensity(blue, value);
    if (argument.equals("cold"))
      changeColorIntensity(cold, value);
    if (argument.equals("warm"))
      changeColorIntensity(warm, value);
  }
}

String SendHTML()
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>LED Control</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #1abc9c;}\n";
  ptr += ".button-on:active {background-color: #16a085;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>ESP8266 Led Control</h1>\n";
  ptr += "<h3>Using Access Point(AP) Mode</h3>\n";
  ptr += "<p>RED Status: " + String(COLOR_VALUES[red]);
  ptr += " </p>\n";
  ptr += "<p>GREEN Status: " + String(COLOR_VALUES[green]);
  ptr += " </p>\n";
  ptr += "<p>BLUE Status: " + String(COLOR_VALUES[blue]);
  ptr += " </p>\n";
  ptr += "<p>WARM Status: " + String(COLOR_VALUES[warm]);
  ptr += " </p>\n";
  ptr += "<p>COLD Status: " + String(COLOR_VALUES[cold]);
  ptr += " </p>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

String SendJSON()
{
  String ptr = "{";
  ptr += "\"success\":1,";
  ptr += "\"red\":" + String(COLOR_VALUES[red]) + ",";
  ptr += "\"green\":" + String(COLOR_VALUES[green]) + ",";
  ptr += "\"blue\":" + String(COLOR_VALUES[blue]) + ",";
  ptr += "\"warm\":" + String(COLOR_VALUES[warm]) + ",";
  ptr += "\"cold\":" + String(COLOR_VALUES[cold]);
  ptr += "}";
  return ptr;
}

void handle_OnConnect()
{
  server.send(200, "text/html", SendHTML());
}

void handle_led()
{
  handleColorArgs();
  writeAllLeds();
  server.send(200, "text/plain", SendJSON());
}

void handle_allOff()
{
  turnLightsOff();
  server.send(200, "text/plain", SendJSON());
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

void setup()
{
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("SERVER INIT");
#endif

  pinMode(PUSHBUTTON, INPUT_PULLUP);

  for (int i = 0; i < 5; i++)
  {
    pinMode(COLOR_PINS[i], OUTPUT);
  }

  LAST_BUTTON_PRESS_TIME = millis();

  writeAllLeds();

  wifiMulti.addAP(WIFI_AP_SSID, WIFI_AP_PASS);

  while (wifiMulti.run() != WL_CONNECTED)
  {
    delay(250);

#ifdef DEBUG
    Serial.println("Connecting ...");
    Serial.print('.');
#endif
  }

  server.on("/", handle_OnConnect);
  server.on("/led", handle_led);
  server.on("/ledoff", handle_allOff);
  server.onNotFound(handle_NotFound);
  server.enableCORS(true);
  server.begin();

#ifdef DEBUG
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif
}

void loop()
{
  // Pushbutton logic.
  if (digitalRead(PUSHBUTTON) == LOW && millis() - LAST_BUTTON_PRESS_TIME >= DEBOUNCE)
  {
    LAST_BUTTON_PRESS_TIME = millis();
    switch (++OVERRIDE)
    {
    case 1:
      blinkTwice(red);
      break;
    case 2:
      blinkTwice(warm);
      turnJustWarmWhiteLedOn();
      break;
    default:
      blinkTwice(green);
      OVERRIDE = 0;
      writeAllLeds();
    }
  }

  if (OVERRIDE == 0)
  {
    server.handleClient();
  }
}
