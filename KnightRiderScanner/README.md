# ESP32 Knight Rider Scanner (NeoPixel)

A KITT-style red "eye" that sweeps back and forth along a NeoPixel strip with a
fading tail. Speed is adjustable with a potentiometer or from the Serial Monitor.

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
3. Open `KnightRiderScanner.ino`, set `NUM_LEDS` to your strip length, upload.

## Speed control
- **Potentiometer:** turn clockwise for faster, anticlockwise for slower
  (range set by `MIN_DELAY_MS` / `MAX_DELAY_MS`).
- **Serial Monitor (115200 baud, newline):**
  - `s25` – set step delay to 25 ms
  - `+` / `-` – faster / slower by 5 ms
  - `?` – show current speed

  Serial control holds until you move the pot again.

## Tweaks
- `EYE_R/G/B` – colour of the eye
- `TAIL_LENGTH` – length of the fading tail
- `BRIGHTNESS` – overall brightness (0–255)
