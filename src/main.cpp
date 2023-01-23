#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#define STRIP_PIN 14
#define MODE_PIN 13
#define COLOR_PIN 12

int color = 0;
int animation = 0;
long unsigned int lastActionTime = millis();
const int actionDelay = 500;

boolean untransmittedSettings = false;
long unsigned int lastTransmitTime = millis();
const int transmitDelay = 1000;

// REPLACE WITH THE MAC Address of your receiver
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Variable to store if sending data was successful
String success;

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message
{
  int color;
  int animation;
} struct_message;

// Create a struct_message called transmitSettings to hold own settings
struct_message ownSettings;

// Create a struct_message to hold incoming settings
struct_message incomingSettings;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(6, STRIP_PIN, NEO_GRB + NEO_KHZ800);

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0)
  {
    Serial.println("Delivery success");
    untransmittedSettings = false;
  }
  else
  {
    Serial.println("Delivery fail");
  }
}

// Callback when data is received
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
  memcpy(&incomingSettings, incomingData, sizeof(incomingSettings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  color = incomingSettings.color;
  animation = incomingSettings.animation;
  untransmittedSettings = false;
}

uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint32_t currentColor()
{
  switch (color)
  {
  case 0:
    return strip.Color(255, 0, 0);
    break;
  case 1:
    return strip.Color(0, 255, 0);
    break;
  case 2:
    return strip.Color(0, 0, 255);
    break;
  case 3:
    return strip.Color(255, 255, 0);
    break;
  case 4:
    return strip.Color(0, 255, 255);
    break;
  case 5:
    return strip.Color(255, 0, 255);
    break;
  case 6:
    return Wheel(millis() % 255);
    break;

  default:
    return strip.Color(255, 255, 255);
    break;
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait)
{
  uint16_t j = (uint16_t)(millis() / wait) % 256;
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
  }
  strip.show();
}

void solid()
{
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, currentColor());
  }
  strip.show();
}

void flash(int wait)
{
  if (millis() % (2 * wait) > wait)
  {
    for (uint16_t i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, currentColor());
    }
    strip.show();
  }
  else
  {
    for (uint16_t i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
  }
}

void strobe(uint8_t wait)
{
}

void heartbeat(uint8_t wait)
{
}

void spinner(uint8_t wait)
{
}

void runCurrentAnimation()
{
  switch (animation)
  {
  case 0:
    rainbowCycle(20);
    break;
  case 1:
    solid();
    break;
  case 2:
    flash(500);
    break;
  case 3:
    flash(200);
    break;
  case 4:
    strobe(200);
    break;
  case 5:
    heartbeat(20);
    break;
  case 6:
    spinner(20);
    break;

  default:
    solid();
    break;
  }
}

IRAM_ATTR void changeMode()
{
  if (millis() > lastActionTime + actionDelay)
  {
    animation = (animation + 1) % 7;
    lastActionTime = millis();
    lastTransmitTime = millis();
    untransmittedSettings = true;
  }
}

IRAM_ATTR void changeColor()
{
  if (millis() > lastActionTime + actionDelay)
  {
    color = (color + 1) % 7;
    lastActionTime = millis();
    lastTransmitTime = millis();
    untransmittedSettings = true;
  }
}

void transmitSettings()
{
  if (untransmittedSettings && millis() > lastTransmitTime + transmitDelay)
  {
    ownSettings.color = color;
    ownSettings.animation = animation;

    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *)&ownSettings, sizeof(ownSettings));
    lastTransmitTime = millis();
  }
}

void setup()
{
  pinMode(MODE_PIN, INPUT_PULLUP);
  pinMode(COLOR_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(MODE_PIN), changeMode, FALLING);
  attachInterrupt(digitalPinToInterrupt(COLOR_PIN), changeColor, FALLING);

  strip.begin();
  strip.setBrightness(50);
  strip.show();

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
  runCurrentAnimation();
  transmitSettings();
}