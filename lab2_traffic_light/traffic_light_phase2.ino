#include <Arduino.h>

struct State {
  unsigned long ledOutput;   // 8-bit output
  unsigned long time;        // delay in ms
  byte nextState[8];         // 3 inputs → 8 transitions
};

typedef const struct State STyp;

const int northGreen  = 13;
const int northYellow = 15;
const int northRed    = 4;
const int eastGreen   = 25;
const int eastYellow  = 26;
const int eastRed     = 27;

// Don't have LEDs yet
const int walkLight   = 14;
const int dontWalk    = 12;

const int northButton = 21;
const int eastButton  = 20;
const int walkButton  = 19;

enum {
  NG, NY, EG, EY,          // Normal traffic
  NGH, NYH, EGH, EYH,      // Walk-hold
  NGR, NYR, EGR, EYR,      // Walk-request
  AR, WALK, FLASH1, FLASH2, FLASH3, FLASH4, DW  // Pedestrian sequence
};

// LED bit mapping
// bit0 = NG, bit1 = NY, bit2 = NR
// bit3 = EG, bit4 = EY, bit5 = ER
// bit6 = WALK, bit7 = DONT WALK

STyp FSM[] = {

  // Normal traffic

  [NG] = {0b10100001, 3000, {NG,NGH,NY,NGH,NG,NGH,NY,NGH}},
  [NY] = {0b10100010, 1000, {EG,NGH,EG,NGH,EG,NGH,EG,NGH}},
  [EG] = {0b10001100, 3000, {EG,EGH,EG,EGH,EY,EGH,EY,EGH}},
  [EY] = {0b10010100, 1000, {NG,EGH,NG,EGH,NG,EGH,NG,EGH}},

  // Walk-hold states

  [NGH] = {0b10100001, 2000, {NG,NGR,NG,NGR,NG,NGR,NG,NGR}},
  [NYH] = {0b10100010, 2000, {EG,NYR,EG,NYR,EG,NYR,EG,NYR}},
  [EGH] = {0b10001100, 2000, {EG,EGR,EG,EGR,EG,EGR,EG,EGR}},
  [EYH] = {0b10010100, 2000, {NG,EYR,NG,EYR,NG,EYR,NG,EYR}},

  // Walk Request states

  [NGR] = {0b10100001, 3000, {NYR,NYR,NYR,NYR,NYR,NYR,NYR,NYR}},
  [NYR] = {0b10100010, 1000, {AR,AR,AR,AR,AR,AR,AR,AR}},
  [EGR] = {0b10001100, 3000, {EYR,EYR,EYR,EYR,EYR,EYR,EYR,EYR}},
  [EYR] = {0b10010100, 1000, {AR,AR,AR,AR,AR,AR,AR,AR}},

  // Pedestrian sequence

  [AR]     = {0b10100100, 1000, {WALK,WALK,WALK,WALK,WALK,WALK,WALK,WALK}},
  [WALK]   = {0b01000100, 3000, {FLASH1,FLASH1,FLASH1,FLASH1,FLASH1,FLASH1,FLASH1,FLASH1}},
  [FLASH1] = {0b10100100, 500,  {FLASH2,FLASH2,FLASH2,FLASH2,FLASH2,FLASH2,FLASH2,FLASH2}},
  [FLASH2] = {0b00100100, 500,  {FLASH3,FLASH3,FLASH3,FLASH3,FLASH3,FLASH3,FLASH3,FLASH3}},
  [FLASH3] = {0b10100100, 500,  {FLASH4,FLASH4,FLASH4,FLASH4,FLASH4,FLASH4,FLASH4,FLASH4}},
  [FLASH4] = {0b00100100, 500,  {DW,DW,DW,DW,DW,DW,DW,DW}},
  [DW]     = {0b10100100, 1000, {NG,EG,NG,EG,NG,EG,NG,EG}}
};

byte currentState = NG;

void setLights(unsigned long pattern) {
  digitalWrite(northGreen,  (pattern & 0x01));
  digitalWrite(northYellow, (pattern & 0x02));
  digitalWrite(northRed,    (pattern & 0x04));
  digitalWrite(eastGreen,   (pattern & 0x08));
  digitalWrite(eastYellow,  (pattern & 0x10));
  digitalWrite(eastRed,     (pattern & 0x20));
  digitalWrite(walkLight,   (pattern & 0x40));
  digitalWrite(dontWalk,    (pattern & 0x80));
}

void setup() {
  Serial.begin(115200);

  pinMode(northGreen,OUTPUT);
  pinMode(northYellow,OUTPUT);
  pinMode(northRed,OUTPUT);
  pinMode(eastGreen,OUTPUT);
  pinMode(eastYellow,OUTPUT);
  pinMode(eastRed,OUTPUT);
  pinMode(walkLight,OUTPUT);
  pinMode(dontWalk,OUTPUT);

  pinMode(northButton,INPUT);
  pinMode(eastButton,INPUT);
  pinMode(walkButton,INPUT);

  setLights(FSM[currentState].ledOutput);
}

void loop() {
  byte n = digitalRead(northButton);
  byte e = digitalRead(eastButton);
  byte w = digitalRead(walkButton);

  byte inputIndex = (n << 2) | (e << 1) | w;

// Added for debug w/ serial monitor
  Serial.print("Current State: ");
  Serial.println(currentState);

  delay(FSM[currentState].time);

  currentState = FSM[currentState].nextState[inputIndex];

  setLights(FSM[currentState].ledOutput);
}
