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
// uint8_t broadcastAddress[] = {0xC8, 0xC9, 0xA3, 0x72, 0xBE, 0x13};
uint8_t broadcastAddress[] = {0xC8, 0xC9, 0xA3, 0x73, 0x2A, 0x01};

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
  }
  else
  {
    Serial.println("Delivery fail");
    lastTransmitTime = millis();
    untransmittedSettings = true;
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

uint32_t scaledColor(uint32_t color, byte scale)
{
  uint8_t r = map(scale, 0, 255, 0, (uint8_t)(color >> 16));
  uint8_t g = map(scale, 0, 255, 0, (uint8_t)(color >> 8));
  uint8_t b = map(scale, 0, 255, 0, (uint8_t)color);

  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
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
    return Wheel(int(millis() / 50) % 255);
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

void strobe(int wait)
{
  int t = millis() % wait;
  const int step = wait / 12;

  if (t < step || (t > step * 2 && t < step * 3) || (t > step * 4 && t < step * 5))
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

void heartbeat(int wait)
{
  int t = millis() % wait;
  const int step = wait / 8;
  const int minimum = 50;
  int scale = minimum;

  if (t < step)
  {
    scale = map(t, 0, step, minimum, 255);
  }
  if (t >= step && t < (step * 2))
  {
    scale = map(t, step, step * 2, 255, minimum);
  }
  if (t >= (step * 2) && t < (step * 3))
  {
    scale = map(t, step * 2, step * 3, minimum, 255);
  }
  if (t >= (step * 3) && t < (step * 4))
  {
    scale = map(t, step * 3, step * 4, 255, minimum);
  }

  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, scaledColor(currentColor(), scale));
  }
  strip.show();
}

void spinner(int wait)
{
  int t = millis() % wait;
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    uint8_t pos = map((t + (wait / strip.numPixels() * i)) % wait, 0, wait, 255, 0);
    strip.setPixelColor(strip.numPixels() - 1 - i, scaledColor(currentColor(), pos));
  }
  strip.show();
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
    strobe(500);
    break;
  case 5:
    heartbeat(3000);
    break;
  case 6:
    spinner(3000);
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
    untransmittedSettings = true;
    lastTransmitTime = millis() - transmitDelay;
  }
}

IRAM_ATTR void changeColor()
{
  if (millis() > lastActionTime + actionDelay)
  {
    color = (color + 1) % 7;
    lastActionTime = millis();
    untransmittedSettings = true;
    lastTransmitTime = millis() - transmitDelay;
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
    untransmittedSettings = false;
  }
}

void setup()
{
  pinMode(MODE_PIN, INPUT_PULLUP);
  pinMode(COLOR_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(MODE_PIN), changeMode, FALLING);
  attachInterrupt(digitalPinToInterrupt(COLOR_PIN), changeColor, FALLING);

  strip.begin();
  strip.setBrightness(20);
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
  delay(20);
}
