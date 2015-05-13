// MIDI Accordion

#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

#define COLUMN_PINS_COUNT 3
#define ROW_PINS_COUNT 16

#define PITCHBEND_ZERO 200
#define PITCHBEND_TH 5
#define MODULATION_TH 5

#define VELOCITY_ADC_PIN 0
#define PITCHBEND_ADC_PIN 1
#define MODULATION_ADC_PIN 2

#define CHANNEL1_SW_PIN 23
#define CHANNEL2_SW_PIN 24
#define SUSTAIN_SW_PIN 25
#define PANIC_SW_PIN 26

#define PITCHBENDZERO_LED_PIN 27
#define CHANNEL1_LED_PIN 28
#define CHANNEL2_LED_PIN 29
#define SUSTAIN_LED_PIN 30
#define PANIC_SW_PIN 31

int g_midiChannel;

int g_velocity;
int g_pitchBend;
int g_sustain;

int g_rawModLast;
int g_rawPitchLast;

int g_prev[COLUMN_PINS_COUNT][ROW_PINS_COUNT] = {0};

int cPins[] = {48, 49, 50};
int rPins[] = {32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};

int notes[COLUMN_PINS_COUNT][ROW_PINS_COUNT] = {
    {81, 78, 75, 72, 69, 66, 63, 60, 57, 54, 51, 48, 45, 42, 39, 36},
    {82, 79, 76, 73, 70, 67, 64, 61, 58, 55, 52, 49, 46, 43, 40, 37},
    {83, 80, 77, 74, 71, 68, 65, 62, 59, 56, 53, 50, 47, 44, 41, 38}};


void setup() {
  MIDI.begin(5);
  g_midiChannel = 1;

  pinMode(PITCHBENDZERO_LED_PIN, OUTPUT);
  pinMode(CHANNEL1_LED_PIN, OUTPUT);
  pinMode(CHANNEL2_LED_PIN, OUTPUT);
  pinMode(SUSTAIN_LED_PIN, OUTPUT);
  pinMode(PANIC_SW_PIN, OUTPUT);

  pinMode(CHANNEL1_SW_PIN, INPUT);
  pinMode(CHANNEL2_SW_PIN, INPUT);
  pinMode(SUSTAIN_SW_PIN, INPUT);
  pinMode(PANIC_SW_PIN, INPUT);
  digitalWrite(CHANNEL1_SW_PIN, HIGH);
  digitalWrite(CHANNEL2_SW_PIN, HIGH);
  digitalWrite(SUSTAIN_SW_PIN, HIGH);
  digitalWrite(PANIC_SW_PIN, HIGH);

  for (int cPin = 0; cPin < COLUMN_PINS_COUNT; cPin++) {
    pinMode(cPins[cPin], OUTPUT);
    digitalWrite(cPins[cPin], HIGH);
  }

  for (int rPin = 0; rPin < ROW_PINS_COUNT; rPin++) {
    pinMode(rPins[rPin], INPUT);
    digitalWrite(rPins[rPin], HIGH);
  }
}


void loop() {
  read_midi_channel();
  read_panic();
  read_sustain();
  read_velocity();
  read_pitch_bend();
  read_modulation();
  read_notes();
}


void read_midi_channel() {
  if (digitalRead(CHANNEL1_SW_PIN) == HIGH) {
    if (digitalRead(CHANNEL2_SW_PIN) == HIGH) {
      if (g_midiChannel != 1) {
        send_all_off();
        g_midiChannel = 1;
        digitalWrite(CHANNEL1_LED_PIN, LOW);
        digitalWrite(CHANNEL2_LED_PIN, LOW);
      }
    } else {
      if (g_midiChannel != 3) {
        send_all_off();
        g_midiChannel = 3;
        digitalWrite(CHANNEL1_LED_PIN, LOW);
        digitalWrite(CHANNEL2_LED_PIN, HIGH);
      }
    }
  } else {
    if (digitalRead(CHANNEL2_SW_PIN) == HIGH) {
      if (g_midiChannel != 2) {
        send_all_off();
        g_midiChannel = 2;
        digitalWrite(CHANNEL1_LED_PIN, HIGH);
        digitalWrite(CHANNEL2_LED_PIN, LOW);
      }
    } else {
      if (g_midiChannel != 4) {
        send_all_off();
        g_midiChannel = 4;
        digitalWrite(CHANNEL1_LED_PIN, HIGH);
        digitalWrite(CHANNEL2_LED_PIN, HIGH);
      }
    }
  }
}


void read_panic() {
  if (digitalRead(PANIC_SW_PIN) == LOW) {
    send_all_off();
    delay(20);
  }
}


void read_sustain() {
  if (digitalRead(SUSTAIN_SW_PIN) == LOW) {
    if (g_sustain == 0) {
      digitalWrite(SUSTAIN_LED_PIN, HIGH);
      MIDI.sendControlChange(64, 1, g_midiChannel);
      g_sustain = 1;
    }
  } else {
    if (g_sustain == 1) {
      digitalWrite(SUSTAIN_LED_PIN, LOW);
      MIDI.sendControlChange(64, 0, g_midiChannel);
      g_sustain = 0;
    }
  }
}


void read_velocity() {
  int vel = analogRead(VELOCITY_ADC_PIN);
  g_velocity = vel / 8;
}


void read_pitch_bend() {
  int pb = analogRead(PITCHBEND_ADC_PIN);
  if (pb > (g_rawPitchLast + PITCHBEND_TH) ||
      pb < (g_rawPitchLast - PITCHBEND_TH)) {
    g_pitchBend = (pb - 512) * 8;
    if (-PITCHBEND_ZERO < g_pitchBend && g_pitchBend < PITCHBEND_ZERO) {
      g_pitchBend = 0;
    }
    MIDI.sendPitchBend(g_pitchBend, g_midiChannel);
    g_rawPitchLast = pb;
  }
  if (g_pitchBend == 0) {
    digitalWrite(PITCHBENDZERO_LED_PIN, HIGH);
  } else {
    digitalWrite(PITCHBENDZERO_LED_PIN, LOW);
  }
}


void read_modulation() {
  int mod = analogRead(MODULATION_ADC_PIN);
  if (mod > (g_rawModLast + MODULATION_TH) ||
      mod < (g_rawModLast - MODULATION_TH)) {
    int modulation = mod / 8;
    MIDI.sendControlChange(1, modulation, g_midiChannel);
    g_rawModLast = mod;
  }
}


void read_notes() {
  for (int cPin = 0; cPin < COLUMN_PINS_COUNT; cPin++) {
    digitalWrite(cPins[cPin], LOW);
    for (int rPin = 0; rPin < ROW_PINS_COUNT; rPin++) {
      if (digitalRead(rPins[rPin]) == LOW) {
        if (g_prev[cPin][rPin] == 0) {
          MIDI.sendNoteOn(notes[cPin][rPin], g_velocity, g_midiChannel);
          g_prev[cPin][rPin] = 1;
        }
      } else {
        if (g_prev[cPin][rPin] == 1) {
          MIDI.sendNoteOff(notes[cPin][rPin], 0, g_midiChannel);
          g_prev[cPin][rPin] = 0;
        }
      }
    }
    digitalWrite(cPins[cPin], HIGH);
  }
}


void send_all_off() {
  digitalWrite(PANIC_SW_PIN, HIGH);
  for (int cPin = 0; cPin < COLUMN_PINS_COUNT; cPin++) {
    for (int rPin = 0; rPin < ROW_PINS_COUNT; rPin++) {
      MIDI.sendNoteOff(notes[cPin][rPin], 0, g_midiChannel);
      g_prev[cPin][rPin] = 0;
    }
  }
  digitalWrite(PANIC_SW_PIN, LOW);
}
