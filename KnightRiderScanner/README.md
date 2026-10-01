# ESP32 Knight Rider Scanner (NeoPixel)

A KITT-style red "eye" that sweeps back and forth along a NeoPixel strip with a
fading tail. Control it with a potentiometer, a Wi-Fi web page on your phone,
or the Serial Monitor.

## Parts
- ESP32 dev board
- WS2812B / NeoPixel strip (default sketch: 16 LEDs)
- 10k potentiometer
- 330–470 Ω resistor (data line), 1000 µF capacitor (across strip power)
- 5 V power supply sized for the strip (~60 mA per LED at full white)

## Wiring
| From                  | To                                   |
|-----------------------|--------------------------------------|
| ESP32 GPIO5           | 330 Ω resistor → strip **DIN**       |
| 5 V supply +          | strip **5V**                         |
| 5 V supply −          | strip **GND** and ESP32 **GND**      |
| Pot outer leg 1       | ESP32 **3V3**                        |
| Pot outer leg 2       | ESP32 **GND**                        |
| Pot middle (wiper)    | ESP32 **GPIO34**                     |

Grounds must be shared. If the strip flickers, add a 3.3 V → 5 V level shifter
(e.g. 74AHCT125) on the data line.

## Software
1. Arduino IDE → Boards Manager → install **esp32** by Espressif.
2. Library Manager → install **Adafruit NeoPixel**.
3. Open `KnightRiderScanner.ino`, set `NUM_LEDS` to your strip length.
4. (Optional) put your Wi-Fi name/password in `WIFI_SSID` / `WIFI_PASSWORD`.
5. Upload, then open the Serial Monitor at 115200 baud to see the web address.

## Wi-Fi web page
- **Home Wi-Fi set:** the ESP32 joins your network. Open the IP address shown in
  the Serial Monitor, or `http://knightrider.local`.
- **Home Wi-Fi blank or not reachable:** the ESP32 makes its own hotspot called
  **KnightRider** (password `kitt2000`). Join it, then open `http://192.168.4.1`.

The page has:
- Speed slider (0–100 %)
- Brightness slider
- Colour picker for the scanner eye
- On/Off button
- A note showing whether the knob or the web page is currently setting the speed

## Speed control
- **Potentiometer:** turn clockwise for faster, anticlockwise for slower
  (range set by `MIN_DELAY_MS` / `MAX_DELAY_MS`).
- **Serial Monitor (115200 baud, newline):**
  - `s25` – set step delay to 25 ms
  - `+` / `-` – faster / slower by 5 ms
  - `?` – show current speed

  Speed set from the web page or serial holds until you move the knob again.

## Tweaks
- `eyeR/eyeG/eyeB` – starting colour of the eye
- `brightness` – starting brightness (0–255)
- `TAIL_LENGTH` – length of the fading tail
- `AP_SSID` / `AP_PASSWORD` / `HOSTNAME` – hotspot name, password and `.local` name
