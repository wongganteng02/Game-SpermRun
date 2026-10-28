#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// ---------- OLED ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- ESP8266 PINS ----------
#define BUTTON_PIN D6   // GPIO12
#define BUZZER_PIN D5   // GPIO14

// ---------- SPERM SETTINGS ----------
int birdX = 32;
float birdY = 32;
float velocity = 0;
float gravity = 0.26;
float flapStrength = -3.4;
float tailOffset = 0;

// ---------- GAME STATE ----------
int score = 0;
bool gameOver = false;
bool introDone = false;

int pipeX = 128;
int pipeGapY = 20;
int pipeGapSize = 34;

// ---------- FUNCTIONS ----------
void beep(int freq, int duration) {
  tone(BUZZER_PIN, freq, duration);
}

void drawSperm(int x, int y, float offset) {
  display.fillCircle(x, y, 3, SSD1306_WHITE);
  display.drawPixel(x + 1, y - 1, SSD1306_BLACK);

  for (int i = 0; i < 10; i++) {
    int xPos = x - 3 - i;
    int yPos = y + sin((offset + i) * 0.6) * 3;
    display.drawPixel(xPos, yPos, SSD1306_WHITE);
  }
}

void kissAnimation() {
  int posA = -10;
  int posB = 138;

  for (int i = 0; i < 50; i++) {
    display.clearDisplay();
    if (posA < 54) posA += 2;
    if (posB > 74) posB -= 2;

    display.fillCircle(posA, 40, 6, SSD1306_WHITE);
    display.drawPixel(posA + 2, 38, SSD1306_BLACK);
    if (i > 30) display.drawFastHLine(posA - 2, 42, 3, SSD1306_WHITE);

    display.fillCircle(posB, 40, 6, SSD1306_WHITE);
    display.drawPixel(posB - 2, 38, SSD1306_BLACK);
    if (i > 30) display.drawFastHLine(posB - 1, 42, 3, SSD1306_WHITE);

    display.display();
    delay(20);
  }

  beep(800, 200);

  for (int h = 0; h < 6; h++) {
    display.clearDisplay();
    display.fillCircle(59, 40, 6, SSD1306_WHITE);
    display.fillCircle(69, 40, 6, SSD1306_WHITE);

    int pulse = (h % 2 == 0) ? 5 : 3;
    display.fillCircle(64 - pulse, 25, pulse, SSD1306_WHITE);
    display.fillCircle(64 + pulse, 25, pulse, SSD1306_WHITE);
    display.fillTriangle(64 - pulse * 2, 26, 64 + pulse * 2, 26, 64, 26 + pulse * 2, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(15, 8);
    display.print("You Have Failed");

    display.setCursor(35, 52);
    display.print("Try again!");

    display.display();
    delay(300);
  }
}

void introAnimation() {
  int ix = -20;

  while (ix < 140) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.print("SPERM RUN");

    ix += 4;
    drawSperm(ix, 50 + sin(ix * 0.1) * 5, ix);

    display.display();
    delay(20);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 30);
  display.print("Press Button");
  display.display();

  while (digitalRead(BUTTON_PIN) == HIGH) {
    yield();  // beri waktu ke sistem ESP8266 agar watchdog tidak reset
  }
  introDone = true;
}

void resetGame() {
  birdY = 32;
  velocity = 0;
  pipeX = 128;
  pipeGapY = random(5, 25);
  score = 0;
  gameOver = false;
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  randomSeed(analogRead(A0));

  Wire.begin(D2, D1); // SDA, SCL

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true) {
      yield();  // OLED tidak terdeteksi, cegah watchdog reset
    }
  }

  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  if (!introDone) {
    introAnimation();
    resetGame();
    return;
  }

  if (!gameOver) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      velocity = flapStrength;
      beep(1500, 20);
    }

    velocity += gravity;
    birdY += velocity;
    tailOffset += 4;

    pipeX -= 3;
    if (pipeX < -12) {
      pipeX = 128;
      pipeGapY = random(5, 25);
      score++;
      beep(2000, 50);
    }

    if (birdY < 2 || birdY > 62) gameOver = true;

    if (birdX + 3 > pipeX && birdX - 3 < pipeX + 10) {
      if (birdY - 3 < pipeGapY || birdY + 3 > pipeGapY + pipeGapSize)
        gameOver = true;
    }

    display.clearDisplay();
    drawSperm(birdX, birdY, tailOffset);
    display.fillRect(pipeX, 0, 10, pipeGapY, SSD1306_WHITE);
    display.fillRect(pipeX, pipeGapY + pipeGapSize, 10, 64, SSD1306_WHITE);

    display.setCursor(0, 0);
    display.print("SCORE: ");
    display.print(score);

    display.display();
    delay(20);
  } else {
    beep(200, 500);

    display.clearDisplay();
    display.setCursor(35, 20);
    display.print("CRASHED!");
    display.setCursor(35, 40);
    display.print("SCORE: ");
    display.print(score);
    display.display();

    delay(2000);
    kissAnimation();
    introDone = false;
  }
}