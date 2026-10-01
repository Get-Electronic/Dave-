// Knight Rider (KITT) style scanner for ESP32 + NeoPixel (WS2812B) strip.
//
// Control it three ways:
//   1. Potentiometer on POT_PIN - turn it to change speed.
//   2. Wi-Fi web page - open http://knightrider.local (or the IP printed on
//      the Serial Monitor) for speed, brightness, colour and on/off.
//   3. Serial Monitor (115200 baud):
//        s<number>  set step delay in ms, e.g. "s25"
//        +  / -     faster / slower
//        ?          print current settings
//
// Speed set from the web page or serial holds until the potentiometer is
// physically moved again.
//
// Wi-Fi: fill in WIFI_SSID / WIFI_PASSWORD to join your network. If they are
// left blank, or the connection fails, the ESP32 creates its own hotspot
// (AP_SSID / AP_PASSWORD) - join it and browse to http://192.168.4.1
//
// Libraries: "Adafruit NeoPixel" (Library Manager). WiFi, WebServer and
// ESPmDNS come with the ESP32 board package.

#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ---------------- Wi-Fi settings -------------------
const char* WIFI_SSID     = "";               // your home Wi-Fi name
const char* WIFI_PASSWORD = "";               // your home Wi-Fi password
const char* AP_SSID       = "KnightRider";    // fallback hotspot name
const char* AP_PASSWORD   = "kitt2000";       // min 8 characters
const char* HOSTNAME      = "knightrider";    // -> http://knightrider.local

// ---------------- Hardware settings ----------------
#define LED_PIN     5     // NeoPixel data pin
#define NUM_LEDS    16    // number of LEDs in the strip
#define POT_PIN     34    // potentiometer wiper (ADC1 pin, works with Wi-Fi)

// ---------------- Effect settings ------------------
#define TAIL_LENGTH   5     // how many LEDs trail behind the eye
#define MIN_DELAY_MS  5     // fastest step time
#define MAX_DELAY_MS  150   // slowest step time

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

// Current settings (changed by pot / web / serial)
uint16_t stepDelay  = 40;          // ms between steps
uint8_t  brightness = 120;         // 0-255
uint8_t  eyeR = 255, eyeG = 0, eyeB = 0;   // classic red
bool     running    = true;

// Scanner state
uint8_t level[NUM_LEDS];           // brightness of each LED, 0-255
int position = 0;                  // current eye position
int direction = 1;                 // +1 = moving right, -1 = moving left
unsigned long lastStep = 0;

bool manualOverride = false;       // true while web/serial controls speed
int lastPotRaw = -1;

// ======================= Speed control =======================

// Read pot (averaged) and return 0-4095
int readPot() {
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(POT_PIN);
  return sum / 8;
}

void updateSpeedFromPot() {
  int raw = readPot();
  if (lastPotRaw < 0) lastPotRaw = raw;

  // If web/serial set the speed, only take back control once the pot moves.
  if (manualOverride) {
    if (abs(raw - lastPotRaw) < 100) return;
    manualOverride = false;
    Serial.println("Potentiometer control resumed");
  }
  lastPotRaw = raw;

  // Pot fully clockwise = fastest
  stepDelay = map(raw, 0, 4095, MAX_DELAY_MS, MIN_DELAY_MS);
}

void setSpeed(int ms) {
  stepDelay = constrain(ms, MIN_DELAY_MS, MAX_DELAY_MS);
  manualOverride = true;
}

// Speed as 0-100 % (100 = fastest) for the web page
int speedPercent() {
  return map(stepDelay, MAX_DELAY_MS, MIN_DELAY_MS, 0, 100);
}

// ======================= Serial =======================

void printStatus() {
  Serial.printf("Step delay: %u ms  (%s control)\n",
                stepDelay, manualOverride ? "manual" : "pot");
}

void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  char c = cmd.charAt(0);
  if (c == 's' || c == 'S')  { setSpeed(cmd.substring(1).toInt()); printStatus(); }
  else if (c == '+')         { setSpeed(stepDelay - 5);            printStatus(); }
  else if (c == '-')         { setSpeed(stepDelay + 5);            printStatus(); }
  else if (c == '?')         { printStatus(); }
  else Serial.println("Commands: s<ms>, +, -, ?");
}

// ======================= Web page =======================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Knight Rider</title>
<style>
 body{margin:0;font-family:system-ui,sans-serif;background:#111;color:#eee;
      display:flex;justify-content:center}
 .card{width:100%;max-width:420px;padding:20px 16px}
 h1{font-size:1.4em;text-align:center;letter-spacing:2px;color:#f33;margin:8px 0 20px}
 .bar{display:flex;gap:4px;justify-content:center;margin-bottom:24px}
 .bar span{width:16px;height:10px;border-radius:2px;background:#300}
 label{display:flex;justify-content:space-between;margin:18px 0 6px}
 input[type=range]{width:100%;accent-color:#f33}
 input[type=color]{width:100%;height:44px;border:0;background:none}
 button{width:100%;padding:14px;margin-top:24px;font-size:1.1em;border:0;
        border-radius:8px;background:#f33;color:#fff}
 button.off{background:#444}
 .src{font-size:.85em;color:#888;text-align:center;margin-top:16px}
</style></head><body><div class="card">
<h1>KNIGHT RIDER</h1>
<div class="bar" id="bar"></div>
<label>Speed <span id="sv"></span></label>
<input type="range" id="speed" min="0" max="100">
<label>Brightness <span id="bv"></span></label>
<input type="range" id="bright" min="5" max="255">
<label>Colour</label>
<input type="color" id="color">
<button id="power">ON</button>
<div class="src" id="src"></div>
</div>
<script>
const $=id=>document.getElementById(id);
for(let i=0;i<12;i++)$('bar').appendChild(document.createElement('span'));
let busy=false,timer;
function show(s){
  if(!busy){$('speed').value=s.speed;$('bright').value=s.brightness;$('color').value=s.color;}
  $('sv').textContent=s.speed+'% ('+s.delay+' ms)';
  $('bv').textContent=Math.round(s.brightness/2.55)+'%';
  $('power').textContent=s.running?'ON':'OFF';
  $('power').className=s.running?'':'off';
  $('src').textContent='Speed controlled by: '+(s.manual?'web/serial':'knob');
}
function send(q){fetch('/set?'+q).then(r=>r.json()).then(show).catch(()=>{});}
function slide(id,key){
  $(id).oninput=()=>{busy=true;clearTimeout(timer);
    timer=setTimeout(()=>{send(key+'='+encodeURIComponent($(id).value));busy=false;},60);};
}
slide('speed','speed');slide('bright','brightness');slide('color','color');
$('power').onclick=()=>send('toggle=1');
function poll(){fetch('/state').then(r=>r.json()).then(show).catch(()=>{});}
poll();setInterval(poll,1500);
// little animated scanner in the header
let p=0,d=1;const bs=$('bar').children;
setInterval(()=>{for(const b of bs)b.style.background='#300';
  bs[p].style.background='#f33';p+=d;if(p<=0||p>=bs.length-1)d=-d;},90);
</script></body></html>
)rawliteral";

String stateJson() {
  char color[8];
  snprintf(color, sizeof(color), "#%02x%02x%02x", eyeR, eyeG, eyeB);
  String j = "{";
  j += "\"speed\":"      + String(speedPercent());
  j += ",\"delay\":"     + String(stepDelay);
  j += ",\"brightness\":"+ String(brightness);
  j += ",\"color\":\""   + String(color) + "\"";
  j += ",\"running\":"   + String(running ? "true" : "false");
  j += ",\"manual\":"    + String(manualOverride ? "true" : "false");
  j += "}";
  return j;
}

void handleRoot()  { server.send_P(200, "text/html", INDEX_HTML); }
void handleState() { server.send(200, "application/json", stateJson()); }

void handleSet() {
  if (server.hasArg("speed")) {
    int pct = constrain(server.arg("speed").toInt(), 0, 100);
    setSpeed(map(pct, 0, 100, MAX_DELAY_MS, MIN_DELAY_MS));
  }
  if (server.hasArg("brightness")) {
    brightness = constrain(server.arg("brightness").toInt(), 0, 255);
    strip.setBrightness(brightness);
  }
  if (server.hasArg("color")) {
    String c = server.arg("color");            // "#rrggbb"
    if (c.length() == 7 && c[0] == '#') {
      long v = strtol(c.c_str() + 1, nullptr, 16);
      eyeR = (v >> 16) & 0xFF;
      eyeG = (v >> 8) & 0xFF;
      eyeB = v & 0xFF;
    }
  }
  if (server.hasArg("toggle")) {
    running = !running;
    if (!running) { memset(level, 0, sizeof(level)); strip.clear(); strip.show(); }
  }
  handleState();
}

void setupWiFi() {
  bool connected = false;

  if (strlen(WIFI_SSID) > 0) {
    Serial.printf("Connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();
    connected = WiFi.status() == WL_CONNECTED;
  }

  if (connected) {
    Serial.print("Wi-Fi connected. Open http://");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.printf("Hotspot \"%s\" (password %s). Open http://", AP_SSID, AP_PASSWORD);
    Serial.println(WiFi.softAPIP());
  }

  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("Or try http://%s.local\n", HOSTNAME);
  }

  server.on("/", handleRoot);
  server.on("/state", handleState);
  server.on("/set", handleSet);
  server.onNotFound(handleRoot);
  server.begin();
}

// ======================= Animation =======================

void drawFrame() {
  // Fade every LED a bit -> leaves a glowing tail behind the eye
  for (int i = 0; i < NUM_LEDS; i++) {
    level[i] = level[i] * (TAIL_LENGTH - 1) / (TAIL_LENGTH + 1);
  }
  level[position] = 255;   // bright eye

  // Colour is applied at draw time so colour changes affect the tail too
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, eyeR * level[i] / 255,
                           eyeG * level[i] / 255,
                           eyeB * level[i] / 255);
  }
  strip.show();

  // Move and bounce at the ends
  position += direction;
  if (position >= NUM_LEDS - 1) { position = NUM_LEDS - 1; direction = -1; }
  if (position <= 0)            { position = 0;            direction = 1;  }
}

// ======================= Main =======================

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);   // full 0-3.3V range

  strip.begin();
  strip.setBrightness(brightness);
  strip.clear();
  strip.show();

  setupWiFi();

  Serial.println("Knight Rider scanner ready");
  Serial.println("Commands: s<ms>, +, -, ?");
}

void loop() {
  server.handleClient();
  handleSerial();
  updateSpeedFromPot();

  unsigned long now = millis();
  if (running && now - lastStep >= stepDelay) {   // non-blocking timing
    lastStep = now;
    drawFrame();
  }
}
