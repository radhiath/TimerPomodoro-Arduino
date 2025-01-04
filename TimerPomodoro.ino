#include <Arduino.h>
#include <TM1637Display.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Definisi pin I/O
#define CLK 2
#define DIO 3
#define BUTTON_ADD_MINUTE 4
#define BUTTON_ADD_TEN_SECONDS 5
#define BUTTON_ADD_ONE_SECOND 6
#define BUTTON_START_STOP 7
#define BUTTON_PMDR_TOGGLE 8
#define BUZZER 9
#define LED_INDICATOR 10

// Segment buat nampilin "dOnE"
const uint8_t SEG_DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
};

// Segment buat nampilin "----"
const uint8_t SEG_DASHDASH[] = { SEG_G, SEG_G, SEG_G, SEG_G};

// Inisialisasi state awal button
bool addMinuteButtonState = false;
bool addTenSecondsButtonState = false;
bool addOneSecondButtonState = false;
bool startStopButtonState = false;
bool pmdrButtonState = false;

// Inisialisasi waktu debounce button
unsigned long lastDebounceTimeAddMinute = 0;
unsigned long lastDebounceTimeAddTenSeconds = 0;
unsigned long lastDebounceTimeAddOneSecond = 0;
unsigned long lastDebounceTimeStartStop = 0;
unsigned long lastDebounceTimePmdr = 0;

unsigned long debounceDelay = 50; // Waktu debounce button (ms)

unsigned long lastUpdateTime = 0;
unsigned long updateDelay = 1000; // Interval update (ms)

unsigned long timerSeconds = 0; // Total waktu (s)

// Inisialisasi mode default
bool pmdrMode = false;
byte pmdrWork = 0;
bool timerRunning = false;
bool timerDone = false;

// Inisialisasi 7 Segment dan LCD
TM1637Display display(CLK, DIO);
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  // Inisialisasi pin I/O
  pinMode(BUTTON_ADD_MINUTE, INPUT_PULLUP);
  pinMode(BUTTON_ADD_TEN_SECONDS, INPUT_PULLUP);
  pinMode(BUTTON_ADD_ONE_SECOND, INPUT_PULLUP);
  pinMode(BUTTON_START_STOP, INPUT_PULLUP);
  pinMode(BUTTON_PMDR_TOGGLE, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_INDICATOR, OUTPUT);

  display.setBrightness(9);
  beepAndDisplay(SEG_DASHDASH, 1000, 100);

  lcd.begin(20, 4);
  lcd.backlight();
  lcd.clear();
}

void loop() {
  // Periksa state tombol dan debounce
  if (debounceButton(BUTTON_PMDR_TOGGLE, pmdrButtonState, lastDebounceTimePmdr) && !timerRunning) {
    justBeep();
    pmdrMode = !pmdrMode;
    lcd.clear();
    if (pmdrMode) {
      lcd.setCursor(0, 0);
      lcd.print("== Mode Pomodoro ===");
      lcd.setCursor(0, 3);
      lcd.print(" Tekan tombol start");
      pmdrWork = 0;
      pmdrCycleTimer();
    } else {
      timerSeconds = 0;
    }
  }

  if (!pmdrMode) {
    lcd.setCursor(0, 0);
    lcd.print("==== Mode Timer ====");
    lcd.setCursor(0, 2);
    lcd.print("  M    K    H    B");
    lcd.setCursor(0, 3);
    lcd.print(" +1M +10d  +1d Start");
    
    if (debounceButton(BUTTON_ADD_MINUTE, addMinuteButtonState, lastDebounceTimeAddMinute) && !timerRunning) {
      justBeep();
      if (timerSeconds <= 5939) { // Pastikan tidak melebihi 5999
        timerSeconds += 60; // Tambah waktu 1 menit
      }
    }

    if (debounceButton(BUTTON_ADD_TEN_SECONDS, addTenSecondsButtonState, lastDebounceTimeAddTenSeconds) && !timerRunning) {
      justBeep();
      if (timerSeconds <= 5989) { // Pastikan tidak melebihi 5999
        timerSeconds += 10; // Tambah waktu 10 detik
      }
    }

    if (debounceButton(BUTTON_ADD_ONE_SECOND, addOneSecondButtonState, lastDebounceTimeAddOneSecond) && !timerRunning) {
      justBeep();
      if (timerSeconds < 5999) { // Pastikan tidak melebihi 5999
        timerSeconds++; // Tambah waktu 1 detik
      }
    }

    if (debounceButton(BUTTON_START_STOP, startStopButtonState, lastDebounceTimeStartStop) && timerSeconds > 0) {
      justBeep();
      timerRunning = !timerRunning; // Mulai/stop timer
    }

  } else {
    if (debounceButton(BUTTON_START_STOP, startStopButtonState, lastDebounceTimeStartStop) && timerSeconds > 0) {
      justBeep();
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      if (pmdrWork % 8 == 0) {
        lcd.setCursor(0, 2);
        lcd.print("Waktunya Istirahat");
        lcd.setCursor(0, 3);
        lcd.print("Ngopi dulu ga sih   ");
      } else if (pmdrWork % 2 == 0) {
        lcd.setCursor(0, 2);
        lcd.print("Waktunya Istirahat");
        lcd.setCursor(0, 3);
        lcd.print("Santai duluuu       ");
      } else {
        lcd.setCursor(0, 2);
        lcd.print("Waktunya Kerja");
        lcd.setCursor(0, 3);
        lcd.print("Semangat!!          ");
      }
      timerRunning = !timerRunning; // Mulai/stop timer
    }
  }
  
  // Indikator LED
  digitalWrite(LED_INDICATOR, timerRunning ? HIGH : LOW);

  // Update timer
  if (timerRunning && millis() - lastUpdateTime >= updateDelay) {
    lastUpdateTime = millis();
    if (timerSeconds > 0) {
      timerSeconds--; // Countdown timer
    } else {
      timerRunning = !timerRunning;;
      timerDone = !timerDone;
      if (pmdrMode) {
        beepAndDisplay(SEG_DONE, 700, 500);
        beepAndDisplay(SEG_DONE, 700, 500);
        beepAndDisplay(SEG_DONE, 700, 500);
        pmdrCycleTimer();
      } else {
        beepAndDisplay(SEG_DONE, 700, 500);
        beepAndDisplay(SEG_DONE, 700, 500);
        beepAndDisplay(SEG_DONE, 700, 500);
      }
    }
  }

  // Tampilan LCD di Mode Pomodoro
  if (timerDone && pmdrMode) {
    lcd.setCursor(0, 1);
    lcd.print("                   ");
    lcd.setCursor(0, 2);
    lcd.print("                   ");
    lcd.setCursor(0, 3);
    lcd.print(" Tekan tombol start");
    delay(1500);
    timerDone = !timerDone;
  } else {
    // Format tampilan MM:SS
    unsigned int minutes = timerSeconds / 60;
    unsigned int seconds = timerSeconds % 60;
    int displayValue = minutes * 100 + seconds;
    display.showNumberDecEx(displayValue, 0b01000000, true);
  }
}

// Fungsi untuk debounce tombol
bool debounceButton(int buttonPin, bool &buttonState, unsigned long &lastDebounceTime) {
  bool reading = digitalRead(buttonPin);
  if (reading != buttonState && millis() - lastDebounceTime > debounceDelay) {
    lastDebounceTime = millis();
    buttonState = reading;
    if (buttonState == LOW) {
      return true;
    }
  }
  return false;
}

// Fungsi untuk siklus Pomodoro
void pmdrCycleTimer() {
  pmdrWork++;
  if (pmdrWork % 8 == 0) {
    timerSeconds = 900; // Istirahat panjang (15 menit)
    pmdrWork = 0;
  } else if (pmdrWork % 2 == 0) {
    timerSeconds = 300; // Istirahat singkat (5 menit)
  } else {
    timerSeconds = 1500; // Kerja (25 menit)
  }
}

// Fungsi untuk bunyi beep dan menampilkan segment
void beepAndDisplay(const uint8_t segments[], unsigned long timeOn, unsigned long timeOff) {
  digitalWrite(BUZZER, HIGH);
  display.setSegments(segments);
  delay(timeOn);
  digitalWrite(BUZZER, LOW);
  display.clear();
  delay(timeOff);
}

// Fungsi untuk bunyi beep
void justBeep() {
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
}
