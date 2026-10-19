/*
  DC Voltmeter 0-50V
  Arduino Nano + SSD1306 128x64 I2C OLED

  Wiring:
    Vin+ -- R1 (100k) --+-- R2 (10k) -- GND(Vin-)
                        |
                        +-- A0   (add 0.1uF cap + 5.1V zener to GND here)
    OLED: VCC->5V, GND->GND, SDA->A4, SCL->A5
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C   // try 0x3D if the display stays blank

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- Calibration ----------
const float VREF = 4.31;      // measure the Nano 5V pin and enter real value
const float R1   = 97800.0;  // top resistor (ohms)
const float R2   = 9700.0;   // bottom resistor (ohms)
const float MAX_V = 50.0;
// ---------------------------------

const int   PIN_V   = A0;
const float DIV     = (R1 + R2) / R2;
float filtered = 0;

float readVoltage() {
  long sum = 0;
  for (int i = 0; i < 64; i++) {
    sum += analogRead(PIN_V);
    delayMicroseconds(200);
  }
  float adc = sum / 64.0;
  return (adc * VREF / 1023.0) * DIV;
}

void drawUI(float v) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Header
  display.setTextSize(1);
  display.setCursor(22, 0);
  display.print(F("DC VOLTMETER"));
  display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);

  // Big reading
  bool over = v > MAX_V + 1.0;
  if (over) {
    if ((millis() / 400) % 2) {           // blink warning
      display.setTextSize(3);
      display.setCursor(22, 18);
      display.print(F("OVER!"));
    }
  } else {
    display.setTextSize(3);
    display.setCursor(6, 16);
    display.print(v, 1);
    display.setTextSize(2);
    display.setCursor(108, 24);
    display.print(F("V"));
  }

  // Bar graph
  float shown = constrain(v, 0, MAX_V);
  int barW = (int)(shown / MAX_V * (SCREEN_WIDTH - 4));
  display.drawRoundRect(0, 44, SCREEN_WIDTH, 10, 3, SSD1306_WHITE);
  display.fillRect(2, 46, barW, 6, SSD1306_WHITE);

  // Ticks every 10V
  for (int i = 0; i <= 5; i++) {
    int x = 2 + i * (SCREEN_WIDTH - 4) / 5;
    display.drawFastVLine(x, 54, 3, SSD1306_WHITE);
  }

  // Scale labels
  display.setTextSize(1);
  display.setCursor(0, 57);
  display.print(F("0"));
  display.setCursor(58, 57);
  display.print(F("25"));
  display.setCursor(116, 57);
  display.print(F("50"));

  display.display();
}

void setup() {
  analogReference(DEFAULT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    while (true);   // OLED not found
  }

  // Splash screen
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 14);
  display.print(F("VOLTMETER"));
  display.setTextSize(1);
  display.setCursor(30, 40);
  display.print(F("0 - 50V DC"));
  display.display();
  delay(1500);

  filtered = readVoltage();
}

void loop() {
  float v = readVoltage();
  // Adaptive smoothing: jump quickly on big changes, smooth when steady
  float diff = fabs(v - filtered);
  if (diff > 0.5)       filtered = v;                        // big change: instant
  else if (diff > 0.15) filtered = filtered * 0.5 + v * 0.5; // medium: fast
  else                  filtered = filtered * 0.9 + v * 0.1; // small: smooth

  // Deadband: only change the shown value if it moved enough
  static float shown = 0;
  if (fabs(filtered - shown) > 0.08) shown = filtered;

  float out = (shown < 0.05) ? 0.0 : shown;
  drawUI(out);
  delay(100);
}
