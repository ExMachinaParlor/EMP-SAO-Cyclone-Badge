#include <Adafruit_NeoPixel.h>

/***********************************************************
   Pin & LED Configuration
 ***********************************************************/
#define RING1_PIN       6  // Data pin for the first NeoPixel ring
#define RING2_PIN       9  // Data pin for the second NeoPixel ring
#define BUTTON_PIN      3  // Push button input pin

// *** MAKE SURE these match the actual number of LEDs on each ring! ***
#define RING1_NUMPIXELS 24 // <-- If your outer ring has 24, change this to 24
#define RING2_NUMPIXELS 16 // <-- Same check for the second ring

/***********************************************************
   NeoPixel Objects
 ***********************************************************/
Adafruit_NeoPixel ring1 = Adafruit_NeoPixel(RING1_NUMPIXELS, RING1_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ring2 = Adafruit_NeoPixel(RING2_NUMPIXELS, RING2_PIN, NEO_GRB + NEO_KHZ800);

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
// Timing & Animation
unsigned long chaseInterval = 80;   // Delay (ms) between increments of the chase
unsigned long lastChaseUpdate = 0;  // Tracks when we last moved the chase light

// LED indices
int chaseIndex = 0;      // Current chasing light position on ring1
int jackpotIndex = 0;    // The "winning" LED index on ring1

// Button Debounce
bool lastButtonState = HIGH;
bool buttonPressed = false;

// Result / Animations
bool winner = false;     // Set true if the player hits the jackpot

/***********************************************************
   Setup
 ***********************************************************/
void setup() {
  // Initialize Serial (for debugging)
  Serial.begin(9600);

  // Initialize NeoPixel rings
  ring1.begin();
  ring2.begin();
  ring1.show();  // Turn off all LEDs initially
  ring2.show();

  // Initialize button pin (internal pull-up)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Print a startup message
  Serial.println("Cyclone-Style Game Starting...");
}

/***********************************************************
   Main Loop
 ***********************************************************/
void loop() {
  // Always check the button state
  readButton();

  // State Machine
  switch (currentState) {
    case ATTRACT_MODE:
      attractMode();
      break;

    case START_GAME:
      startGame();
      break;

    case PLAY_GAME:
      playGame();
      break;

    case RESULT:
      showResult();
      break;
  }
}

/***********************************************************
   State: Attract Mode
   Simple animation to draw attention
 ***********************************************************/
void attractMode() {
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  // Example: rotate a single pixel around ring1, 
  // and maybe blink ring2 in a different color
  if (currentTime - lastUpdate > 100) {
    lastUpdate = currentTime;

    // Simple rotation on ring1
    static int attractIndex = 0;
    ring1.clear();
    ring1.setPixelColor(attractIndex, ring1.Color(50, 0, 50)); // Purple
    ring1.show();

    // Blink ring2 every few steps
    ring2.clear();
    if (attractIndex % 4 < 2) {
      for(int i = 0; i < RING2_NUMPIXELS; i++) {
        ring2.setPixelColor(i, ring2.Color(0, 30, 0)); // Dim green
      }
    }
    ring2.show();

    attractIndex = (attractIndex + 1) % RING1_NUMPIXELS;
  }

  // If button pressed, move to START_GAME
  if (buttonPressed) {
    buttonPressed = false;
    currentState = START_GAME;
  }
}

/***********************************************************
   State: Start Game
   Initialize variables for a new round
 ***********************************************************/
void startGame() {
  // Pick a random jackpot index or use a fixed one
  jackpotIndex = random(0, RING1_NUMPIXELS);
  Serial.print("New Game - Jackpot Index: ");
  Serial.println(jackpotIndex);

  // Reset the chase
  chaseIndex = 0;
  lastChaseUpdate = millis();
  winner = false;

  // Clear both rings
  ring1.clear();
  ring1.show();
  ring2.clear();
  ring2.show();

  // Transition to PLAY_GAME
  currentState = PLAY_GAME;
}

/***********************************************************
   State: Play Game
   Move the chasing light around ring1.
   If button pressed, capture result.
 ***********************************************************/
void playGame() {
  unsigned long currentTime = millis();

  // Update the chasing light based on chaseInterval
  if (currentTime - lastChaseUpdate >= chaseInterval) {
    lastChaseUpdate = currentTime;

    // Move to next LED
    chaseIndex = (chaseIndex + 1) % RING1_NUMPIXELS;

    // Light up ring1
    ring1.clear();
    ring1.setPixelColor(chaseIndex, ring1.Color(255, 0, 0)); // Red chasing light
    ring1.show();
  }

  // Check for button press to "stop" the chase
  if (buttonPressed) {
    buttonPressed = false;

    // Check if chaseIndex == jackpotIndex
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
   State: Result
   Show win/lose animation on both rings
 ***********************************************************/
void showResult() {
  // Quick animation demonstration
  if (winner) {
    // Win animation: flash ring1 a few times, ring2 all green
    for (int i = 0; i < 3; i++) {
      ring1.fill(ring1.Color(0, 255, 0)); // Green flash
      ring1.show();
      ring2.fill(ring2.Color(0, 255, 0)); // All green
      ring2.show();
      delay(300);

      ring1.clear(); 
      ring1.show();
      ring2.clear(); 
      ring2.show();
      delay(300);
    }
  } else {
    // Lose animation: highlight the jackpot in one color, 
    // the player's stop in another
    for (int i = 0; i < 3; i++) {
      ring1.clear();
      // Jackpot LED in yellow
      ring1.setPixelColor(jackpotIndex, ring1.Color(255, 255, 0)); 
      // Stopped LED in red
      ring1.setPixelColor(chaseIndex, ring1.Color(255, 0, 0));
      ring1.show();

      // Ring2 in red
      ring2.fill(ring2.Color(255, 0, 0));
      ring2.show();
      delay(300);

      ring1.clear();
      ring1.show();
      ring2.clear();
      ring2.show();
      delay(300);
    }
  }

  // After animation, go back to attract mode
  currentState = ATTRACT_MODE;
}

/***********************************************************
   readButton()
   - Reads button with basic debounce
   - Sets a global 'buttonPressed' flag on the falling edge
   - Now includes a debug print for the press
 ***********************************************************/
void readButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Check for falling edge: HIGH -> LOW
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    Serial.println("Button pressed!");
    buttonPressed = true;
  }

  lastButtonState = currentButtonState;
}
