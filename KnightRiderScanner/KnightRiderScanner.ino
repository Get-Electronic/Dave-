// Knight Rider (KITT) style scanner for ESP32 + NeoPixel (WS2812B) strip.
//
// Speed control:
//   - Potentiometer on POT_PIN: turn it to speed up / slow down the scan.
//   - Serial monitor (115200 baud) commands:
//       s<number>  set step delay in ms, e.g. "s25"
//       +  / -     faster / slower
//       ?          print current settings
//     A serial command takes over until the potentiometer is moved again.
//
// Library: "Adafruit NeoPixel" (install from Arduino Library Manager).

#include <Adafruit_NeoPixel.h>

// ---------------- Hardware settings ----------------
#define LED_PIN     5     // NeoPixel data pin
#define NUM_LEDS    16    // number of LEDs in the strip
#define POT_PIN     34    // potentiometer wiper (ADC1 pin, input only)
#define BRIGHTNESS  120   // 0-255 global brightness

// ---------------- Effect settings ------------------
#define TAIL_LENGTH   5     // how many LEDs trail behind the eye
#define MIN_DELAY_MS  5     // fastest step time
#define MAX_DELAY_MS  150   // slowest step time
const uint8_t EYE_R = 255, EYE_G = 0, EYE_B = 0;   // classic red

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

int position = 0;              // current eye position
int direction = 1;             // +1 = moving right, -1 = moving left
uint16_t stepDelay = 40;       // ms between steps
unsigned long lastStep = 0;

bool serialOverride = false;   // true while a serial command controls speed
int lastPotRaw = -1;

// Read pot (averaged) and return 0-4095
int readPot() {
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(POT_PIN);
  return sum / 8;
}

void updateSpeedFromPot() {
  int raw = readPot();
  if (lastPotRaw < 0) lastPotRaw = raw;

  // If a serial command set the speed, only take back control
  // once the pot is physically moved.
  if (serialOverride) {
    if (abs(raw - lastPotRaw) < 100) return;
    serialOverride = false;
    Serial.println("Potentiometer control resumed");
  }
  lastPotRaw = raw;

  // Pot fully clockwise = fastest
  stepDelay = map(raw, 0, 4095, MAX_DELAY_MS, MIN_DELAY_MS);
}

void printStatus() {
  Serial.printf("Step delay: %u ms  (%s control)\n",
                stepDelay, serialOverride ? "serial" : "pot");
}

void setSpeed(int ms) {
  stepDelay = constrain(ms, MIN_DELAY_MS, MAX_DELAY_MS);
  serialOverride = true;
  printStatus();
}

void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  char c = cmd.charAt(0);
  if (c == 's' || c == 'S') {
    setSpeed(cmd.substring(1).toInt());
  } else if (c == '+') {
    setSpeed(stepDelay - 5);
  } else if (c == '-') {
    setSpeed(stepDelay + 5);
  } else if (c == '?') {
    printStatus();
  } else {
    Serial.println("Commands: s<ms>, +, -, ?");
  }
}

void drawFrame() {
  // Fade every LED a bit -> leaves a glowing tail behind the eye
  for (int i = 0; i < NUM_LEDS; i++) {
    uint32_t col = strip.getPixelColor(i);
    uint8_t r = (col >> 16) & 0xFF;
    uint8_t g = (col >> 8) & 0xFF;
    uint8_t b = col & 0xFF;
    // Scale so the tail lasts roughly TAIL_LENGTH steps
    r = r * (TAIL_LENGTH - 1) / (TAIL_LENGTH + 1);
    g = g * (TAIL_LENGTH - 1) / (TAIL_LENGTH + 1);
    b = b * (TAIL_LENGTH - 1) / (TAIL_LENGTH + 1);
    strip.setPixelColor(i, r, g, b);
  }

  // Bright eye
  strip.setPixelColor(position, EYE_R, EYE_G, EYE_B);
  strip.show();

  // Move and bounce at the ends
  position += direction;
  if (position >= NUM_LEDS - 1) { position = NUM_LEDS - 1; direction = -1; }
  if (position <= 0)            { position = 0;            direction = 1;  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);   // full 0-3.3V range

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();

  Serial.println("Knight Rider scanner ready");
  Serial.println("Commands: s<ms>, +, -, ?");
}

void loop() {
  handleSerial();
  updateSpeedFromPot();

  unsigned long now = millis();
  if (now - lastStep >= stepDelay) {   // non-blocking timing
    lastStep = now;
    drawFrame();
  }
}
