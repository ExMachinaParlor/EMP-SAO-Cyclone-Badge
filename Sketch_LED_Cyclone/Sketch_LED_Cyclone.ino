#include <Adafruit_NeoPixel.h>

/***********************************************************
   Pin & LED Configuration
 ***********************************************************/
#define DATA_PIN            2   // Data pin for both NeoPixel rings
#define SMALL_RING_NUM      16  // Number of LEDs in the small (inner) ring
#define LARGE_RING_NUM      24  // Number of LEDs in the large (outer) ring
#define TOTAL_PIXELS        (SMALL_RING_NUM + LARGE_RING_NUM)
#define BUTTON_PIN          4   // Push button input pin

/***********************************************************
   NeoPixel Object
 ***********************************************************/
Adafruit_NeoPixel strip(TOTAL_PIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);

// Offsets into the single strip
const uint16_t smallOffset = 0;
const uint16_t largeOffset = SMALL_RING_NUM;

/***********************************************************
   Game State Definitions
 ***********************************************************/
enum GameState {
  ATTRACT_MODE,
  START_GAME,
  PLAY_GAME,
  RESULT
};
GameState currentState = ATTRACT_MODE;

/***********************************************************
   Game Variables
 ***********************************************************/
unsigned long chaseInterval = 80;     // Delay (ms) between chase steps
unsigned long lastChaseUpdate = 0;    // Time of last chase step
int chaseIndex = 0;                   // Current chase position (0..LARGE_RING_NUM-1)
int jackpotIndex = 0;                 // Winning position on large ring
bool winner = false;                  // Outcome flag

// Button debounce
bool lastButtonState = HIGH;
bool buttonPressed = false;

/***********************************************************
   Setup
 ***********************************************************/
void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all off
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Cyclone-Style Game Starting...");
}

/***********************************************************
   Main Loop
 ***********************************************************/
void loop() {
  readButton();
  switch (currentState) {
    case ATTRACT_MODE: attractMode(); break;
    case START_GAME:   startGame();   break;
    case PLAY_GAME:    playGame();    break;
    case RESULT:       showResult();  break;
  }
}

/***********************************************************
   Attract Mode
 ***********************************************************/
void attractMode() {
  static unsigned long lastUpdate = 0;
  unsigned long t = millis();
  if (t - lastUpdate > 100) {
    lastUpdate = t;
    static int attractIdx = 0;
    strip.clear();

    // Rotate one purple pixel on outer (large) ring
    strip.setPixelColor(largeOffset + attractIdx, strip.Color(50, 0, 50));

    // Blink inner (small) ring dim green every other step
    if ((attractIdx % 4) < 2) {
      for (uint16_t i = 0; i < SMALL_RING_NUM; i++) {
        strip.setPixelColor(smallOffset + i, strip.Color(0, 30, 0));
      }
    }

    strip.show();
    attractIdx = (attractIdx + 1) % LARGE_RING_NUM;
  }
  if (buttonPressed) { buttonPressed = false; currentState = START_GAME; }
}

/***********************************************************
   Start Game
 ***********************************************************/
void startGame() {
  jackpotIndex = random(0, LARGE_RING_NUM);
  Serial.print("New Game - Jackpot Index (outer ring): ");
  Serial.println(jackpotIndex);
  chaseIndex = 0;
  lastChaseUpdate = millis();
  winner = false;

  // Clear all
  strip.clear();
  strip.show();

  // Show jackpot on outer ring (blue) for 2s
  strip.setPixelColor(largeOffset + jackpotIndex, strip.Color(0, 0, 255));
  strip.show();
  delay(2000);

  strip.clear();
  strip.show();
  currentState = PLAY_GAME;
}

/***********************************************************
   Play Game
 ***********************************************************/
void playGame() {
  unsigned long t = millis();
  if (t - lastChaseUpdate >= chaseInterval) {
    lastChaseUpdate = t;
    chaseIndex = (chaseIndex + 1) % LARGE_RING_NUM;
    strip.clear();
    strip.setPixelColor(largeOffset + chaseIndex, strip.Color(255, 0, 0));
    strip.show();
  }
  if (buttonPressed) {
    buttonPressed = false;
    if (chaseIndex == jackpotIndex) {
      winner = true;
      Serial.println("Jackpot HIT!");
    } else {
      winner = false;
      Serial.print("Jackpot MISS! Stopped at ");
      Serial.print(chaseIndex);
      Serial.print(", jackpot is ");
      Serial.println(jackpotIndex);
    }
    currentState = RESULT;
  }
}

/***********************************************************
   Result Animation
 ***********************************************************/
void showResult() {
  if (winner) {
    for (int i = 0; i < 3; i++) {
      // Flash both rings green
      for (uint16_t p = 0; p < TOTAL_PIXELS; p++) strip.setPixelColor(p, strip.Color(0, 255, 0));
      strip.show(); delay(300);
      strip.clear(); strip.show(); delay(300);
    }
  } else {
    for (int i = 0; i < 3; i++) {
      strip.clear();
      // Show jackpot (yellow) and stopped (red) on outer ring
      strip.setPixelColor(largeOffset + jackpotIndex, strip.Color(255, 255, 0));
      strip.setPixelColor(largeOffset + chaseIndex,   strip.Color(255, 0, 0));
      // Flash inner ring red
      for (uint16_t p = 0; p < SMALL_RING_NUM; p++) strip.setPixelColor(smallOffset + p, strip.Color(255, 0, 0));
      strip.show(); delay(300);
      strip.clear(); strip.show(); delay(300);
    }
  }
  currentState = ATTRACT_MODE;
}

/***********************************************************
   readButton(): Debounce & detect press
 ***********************************************************/
void readButton() {
  bool currState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && currState == LOW) {
    Serial.println("Button pressed!");
    buttonPressed = true;
  }
  lastButtonState = currState;
}
