#include <SPI.h>

// Pins
#define PIN_SCK 14   // 74HC595 SRCLK
#define PIN_MOSI 13   // 74HC595 SER
#define PIN_LATCH 33   // 74HC595 RCLK

const uint8_t DIGIT_PINS[4] = {25, 26, 27, 32};  // digit 1 (left) .. digit 4 (right)

#define DIGIT_ON HIGH
#define DIGIT_OFF LOW

#define BTN_START 19
#define BTN_STOP 21
#define BTN_INC 22
#define BTN_RESET 4

// Constants
#define DISPLAY_PERIOD_US 2500   // 4 digits x 2.5 ms = 100 Hz refresh
#define MS_PERIOD_US 1000   // stopwatch resolution: 1 ms
#define DEBOUNCE_MS 40

// Lit-segment patterns
const uint8_t SEG[10] = {
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F  // 9
};
const uint16_t DIVISOR[4] = {1000, 100, 10, 1};

// Globals shared with ISRs
volatile uint16_t displayValue = 0; // 0..9999 -> S.mmm
volatile uint8_t  currentDigit = 0; // digit being driven (0..3)
volatile bool     running = false;
volatile uint32_t lastStartMs = 0; // per-button debounce timestamps
volatile uint32_t lastStopMs = 0;
volatile uint32_t lastIncMs = 0;
volatile uint32_t lastResetMs = 0;

// Display timer ISR (time-multiplexing)
void IRAM_ATTR onDisplayTimer() {
  // Blank digits prevent ghosting
  for (uint8_t i = 0; i < 4; i++) digitalWrite(DIGIT_PINS[i], DIGIT_OFF);

  uint8_t d   = (displayValue / DIVISOR[currentDigit]) % 10;
  uint8_t out = ~SEG[d];
  if (currentDigit == 0) out &= ~0x80; // decimal point after the left-most digit

  // Shift out over SPI, then pulse RCLK to latch
  SPI.transfer(out);
  digitalWrite(PIN_LATCH, HIGH);
  digitalWrite(PIN_LATCH, LOW);

  // Turn on this digit, then move to the next one
  digitalWrite(DIGIT_PINS[currentDigit], DIGIT_ON);
  currentDigit = (currentDigit + 1) & 0x03;
}

 // Stopwatch timer ISR (1 ms)
void IRAM_ATTR onMsTimer() {
  if (running) {
    displayValue++;
    if (displayValue > 9999) displayValue = 0; // overflow after 10 s
  }
}

// Button ISRs (debounced with timestamps)
void IRAM_ATTR onStart() {
  uint32_t now = millis();
  if (now - lastStartMs > DEBOUNCE_MS) {
    lastStartMs = now;
    running = true;
  }
}

void IRAM_ATTR onStop() {
  uint32_t now = millis();
  if (now - lastStopMs > DEBOUNCE_MS) {
    lastStopMs = now;
    running = false;
  }
}

void IRAM_ATTR onIncrement() {
  uint32_t now = millis();
  if (now - lastIncMs > DEBOUNCE_MS) {
    lastIncMs = now;
    if (!running) {
      displayValue++;
      if (displayValue > 9999) displayValue = 0;
    }
  }
}

void IRAM_ATTR onReset() {
  uint32_t now = millis();
  if (now - lastResetMs > DEBOUNCE_MS) {
    lastResetMs = now;
    running = false;
    displayValue = 0;
  }
}

// Timer setup
static void startTimer(uint8_t id, uint32_t periodUs, void (*isr)()) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)id;
  hw_timer_t *t = timerBegin(1000000); // 1 MHz tick = 1 us
  timerAttachInterrupt(t, isr);
  timerAlarm(t, periodUs, true, 0); // auto-reload
#else
  hw_timer_t *t = timerBegin(id, 80, true); // 80 MHz / 80 = 1 MHz
  timerAttachInterrupt(t, isr, true);
  timerAlarmWrite(t, periodUs, true);
  timerAlarmEnable(t);
#endif
}

void setup() {
  // Latch and digit pins
  pinMode(PIN_LATCH, OUTPUT);
  digitalWrite(PIN_LATCH, LOW);
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(DIGIT_PINS[i], OUTPUT);
    digitalWrite(DIGIT_PINS[i], DIGIT_OFF); // all digits off
  }

  // SPI to the 74HC595 (SCK=14, MISO unused, MOSI=13, SS unused)
  SPI.begin(PIN_SCK, -1, PIN_MOSI, -1);

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  // Buttons: internal pull-ups, interrupt on the falling edge (press = LOW)
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);
  pinMode(BTN_INC, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_START), onStart, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_STOP), onStop, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_INC), onIncrement, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_RESET), onReset, FALLING);

  // Timers
  startTimer(0, DISPLAY_PERIOD_US, onDisplayTimer);
  startTimer(1, MS_PERIOD_US, onMsTimer);
}

void loop() {
}
