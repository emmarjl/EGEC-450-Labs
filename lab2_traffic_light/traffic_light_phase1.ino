#include <Arduino.h>

struct State {
  unsigned long int ledOutput;
  unsigned long time;
  byte nextState[4];
};

typedef const struct State STyp;

// Pin definitions
const int northGreen = 13;
const int northYellow = 15;
const int northRed = 4;
const int eastRed = 27;
const int eastYellow = 26;
const int eastGreen = 25;
const int northButton = 21;
const int eastButton = 20;

// State indices
#define NORTH_GREEN 0
#define NORTH_YELLOW 1
#define EAST_GREEN 2
#define EAST_YELLOW 3

// Input: 00 = none, 01 = East, 10 = North, 11 = both
// LEDs: bit 0 = NG, bit 1 = NY, bit 2 = NR
//       bit 3 = EG, bit 4 = EY, bit 5 = ER

STyp FSM[4] = {
  // North Green
  {0b100001,3000,
   {NORTH_GREEN,NORTH_YELLOW,NORTH_GREEN,NORTH_YELLOW}},

  // North Yellow
  {0b100010,1000,
   {EAST_GREEN,EAST_GREEN,EAST_GREEN,EAST_GREEN}},

  // East Green
  {0b001100,3000,
   {EAST_GREEN,EAST_GREEN,EAST_YELLOW,EAST_YELLOW}},

  // East Yellow
  {0b010100,1000,
   {NORTH_GREEN,NORTH_GREEN,NORTH_GREEN,NORTH_GREEN}}
};

byte currentState = NORTH_GREEN;

void setLights(unsigned long pattern) {
  digitalWrite(northGreen,(pattern & 0x01) ? HIGH : LOW);
  digitalWrite(northYellow,(pattern & 0x02) ? HIGH : LOW);
  digitalWrite(northRed,(pattern & 0x04) ? HIGH : LOW);
  digitalWrite(eastGreen,(pattern & 0x08) ? HIGH : LOW);
  digitalWrite(eastYellow,(pattern & 0x10) ? HIGH : LOW);
  digitalWrite(eastRed,(pattern & 0x20) ? HIGH : LOW);
}

void setup() {
  pinMode(northGreen,OUTPUT);
  pinMode(northYellow,OUTPUT);
  pinMode(northRed,OUTPUT);
  pinMode(eastRed,OUTPUT);
  pinMode(eastYellow,OUTPUT);
  pinMode(eastGreen,OUTPUT);
  pinMode(northButton,INPUT);
  pinMode(eastButton,INPUT);

  setLights(FSM[currentState].ledOutput);
}

void loop() {
  byte northPressed = digitalRead(northButton);
  byte eastPressed = digitalRead(eastButton);
  byte inputIndex = (northPressed << 1) | eastPressed;

  delay(FSM[currentState].time);

  currentState = FSM[currentState].nextState[inputIndex];

  setLights(FSM[currentState].ledOutput);
}
