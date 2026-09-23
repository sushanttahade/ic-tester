#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ========== PIN DEFINITIONS ==========

// IC Testing Pins
#define IC_INPUT_A 25
#define IC_INPUT_B 26
#define IC_OUTPUT 34

// Voltage Measurement Pin
#define VOLTAGE_PIN 35  // ADC pin for voltage measurement (ADC1_CH7)

// Resistance Measurement Pins
#define RES_GPIO1 27    // Controls R1 (100kΩ) path
#define RES_GPIO2 14    // Controls R2 (10kΩ) path
#define RES_GPIO3 12    // Controls R3 (1kΩ) path
#define RES_MEASURE 32  // ADC pin to measure voltage across unknown resistor (ADC1_CH4)

// Button Pins
#define MODE_BUTTON 15     // Cycles through IC types or measurement modes
#define TEST_BUTTON 4      // Starts test or measurement
#define SWITCH_BUTTON 5    // Switches between main modes (IC/Voltage/Resistance)

// ========== CONSTANTS ==========

// Voltage divider resistor values (adjust based on your circuit)
#define R1_VOLTAGE 33000.0  // 33kΩ
#define R2_VOLTAGE 7500.0   // 7.5kΩ

// Reference resistor values for resistance measurement
#define R1_REFERENCE 100000.0  // 100kΩ
#define R2_REFERENCE 10000.0   // 10kΩ
#define R3_REFERENCE 1000.0    // 1kΩ

#define VCC 3.3  // ESP32 operating voltage
#define ADC_RESOLUTION 4095.0  // 12-bit ADC

// ========== ENUMS ==========

enum MainMode {
  MODE_IC_TEST,
  MODE_VOLTAGE,
  MODE_RESISTANCE
};

enum ICType {
  IC_AND,
  IC_OR,
  IC_NOT,
  IC_NAND
};

// ========== GLOBAL VARIABLES ==========

MainMode currentMainMode = MODE_IC_TEST;
ICType currentIC = IC_AND;
String icNames[] = {"AND", "OR", "NOT", "NAND"};
String modeNames[] = {"IC Tester", "Voltmeter", "Ohmmeter"};

// ========== SETUP ==========

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 Multi-Purpose Tester ===");
  
  // Initialize IC testing pins
  pinMode(IC_INPUT_A, OUTPUT);
  pinMode(IC_INPUT_B, OUTPUT);
  pinMode(IC_OUTPUT, INPUT);
  digitalWrite(IC_INPUT_A, LOW);
  digitalWrite(IC_INPUT_B, LOW);
  
  // Initialize voltage measurement pin
  pinMode(VOLTAGE_PIN, INPUT);
  
  // Initialize resistance measurement pins
  pinMode(RES_GPIO1, OUTPUT);
  pinMode(RES_GPIO2, OUTPUT);
  pinMode(RES_GPIO3, OUTPUT);
  pinMode(RES_MEASURE, INPUT);
  digitalWrite(RES_GPIO1, LOW);
  digitalWrite(RES_GPIO2, LOW);
  digitalWrite(RES_GPIO3, LOW);
  
  // Initialize button pins
  pinMode(MODE_BUTTON, INPUT_PULLUP);
  pinMode(TEST_BUTTON, INPUT_PULLUP);
  pinMode(SWITCH_BUTTON, INPUT_PULLUP);
  
  Serial.println("All pins initialized");
  
  // Initialize I2C and display
  Wire.begin();
  scanI2C();
  
  bool displayFound = false;
  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    displayFound = true;
    Serial.println("Display found at 0x3C");
  } else if(display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    displayFound = true;
    Serial.println("Display found at 0x3D");
  }
  
  if(!displayFound) {
    Serial.println("ERROR: Display not found!");
    while(1) delay(1000);
  }
  
  // Welcome screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 10);
  display.print("MULTI-PURPOSE");
  display.setCursor(30, 25);
  display.print("TESTER");
  display.setCursor(10, 45);
  display.print("IC/Volt/Resist");
  display.display();
  
  Serial.println("Display initialized");
  delay(2000);
  
  displayMainModeSelection();
}

// ========== MAIN LOOP ==========

void loop() {
  // SWITCH button - changes main mode
  if(digitalRead(SWITCH_BUTTON) == LOW) {
    Serial.println("SWITCH button pressed");
    delay(50);
    currentMainMode = (MainMode)((currentMainMode + 1) % 3);
    Serial.print("Main mode changed to: ");
    Serial.println(modeNames[currentMainMode]);
    displayMainModeSelection();
    while(digitalRead(SWITCH_BUTTON) == LOW) delay(10);
    delay(50);
  }
  
  // MODE button - changes sub-mode or IC type
  if(digitalRead(MODE_BUTTON) == LOW) {
    Serial.println("MODE button pressed");
    delay(50);
    
    if(currentMainMode == MODE_IC_TEST) {
      currentIC = (ICType)((currentIC + 1) % 4);
      Serial.print("IC type changed to: ");
      Serial.println(icNames[currentIC]);
      displayICSelection();
    } else {
      // For voltage and resistance, MODE does nothing (continuous display)
    }
    
    while(digitalRead(MODE_BUTTON) == LOW) delay(10);
    delay(50);
  }
  
  // TEST button - starts test or measurement
  if(digitalRead(TEST_BUTTON) == LOW) {
    Serial.println("TEST button pressed");
    delay(50);
    
    switch(currentMainMode) {
      case MODE_IC_TEST:
        testIC();
        break;
      case MODE_VOLTAGE:
        measureVoltage();
        break;
      case MODE_RESISTANCE:
        measureResistance();
        break;
    }
    
    while(digitalRead(TEST_BUTTON) == LOW) delay(10);
    delay(50);
  }
  
  // Continuous measurement for voltage and resistance modes
  if(currentMainMode == MODE_VOLTAGE) {
    static unsigned long lastUpdate = 0;
    if(millis() - lastUpdate > 500) {
      displayVoltageLive();
      lastUpdate = millis();
    }
  } else if(currentMainMode == MODE_RESISTANCE) {
    static unsigned long lastUpdate = 0;
    if(millis() - lastUpdate > 500) {
      displayResistanceLive();
      lastUpdate = millis();
    }
  }
  
  delay(10);
}

// ========== I2C SCANNING ==========

void scanI2C() {
  Serial.println("Scanning I2C bus...");
  byte error, address;
  int devices = 0;
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      devices++;
    }
  }
  
  Serial.print("Found ");
  Serial.print(devices);
  Serial.println(" device(s)");
}

// ========== DISPLAY FUNCTIONS ==========

void displayMainModeSelection() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Main Mode:");
  
  display.setTextSize(2);
  display.setCursor(5, 20);
  
  if(currentMainMode == MODE_IC_TEST) {
    display.print("IC TEST");
  } else if(currentMainMode == MODE_VOLTAGE) {
    display.print("VOLTAGE");
  } else {
    display.print("RESIST");
  }
  
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("SWITCH:Change Mode");
  display.setCursor(0, 55);
  
  if(currentMainMode == MODE_IC_TEST) {
    display.print("MODE:IC TEST:Start");
  } else {
    display.print("TEST:Measure");
  }
  
  display.display();
  
  // Set appropriate sub-display
  delay(1000);
  if(currentMainMode == MODE_IC_TEST) {
    displayICSelection();
  }
}

void displayICSelection() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("IC Type:");
  
  display.setTextSize(3);
  display.setCursor(20, 20);
  display.print(icNames[currentIC]);
  
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.print("MODE:Next TEST:Run");
  
  display.display();
}

// ========== IC TESTING ==========

void testIC() {
  Serial.println("\n=============================");
  Serial.print("TESTING ");
  Serial.print(icNames[currentIC]);
  Serial.println(" GATE");
  Serial.println("=============================");
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.print("Testing...");
  display.setTextSize(2);
  display.setCursor(20, 30);
  display.print(icNames[currentIC]);
  display.display();
  
  delay(500);
  
  bool passed = false;
  
  switch(currentIC) {
    case IC_AND:
      passed = testAND();
      break;
    case IC_OR:
      passed = testOR();
      break;
    case IC_NOT:
      passed = testNOT();
      break;
    case IC_NAND:
      passed = testNAND();
      break;
  }
  
  Serial.println("=============================");
  Serial.print("RESULT: ");
  Serial.println(passed ? "PASS" : "FAIL");
  Serial.println("=============================\n");
  
  displayICResult(passed);
}

bool testAND() {
  Serial.println("A | B | Exp | Act | Status");
  bool t1 = testLogic(LOW, LOW, LOW);
  bool t2 = testLogic(LOW, HIGH, LOW);
  bool t3 = testLogic(HIGH, LOW, LOW);
  bool t4 = testLogic(HIGH, HIGH, HIGH);
  return t1 && t2 && t3 && t4;
}

bool testOR() {
  Serial.println("A | B | Exp | Act | Status");
  bool t1 = testLogic(LOW, LOW, LOW);
  bool t2 = testLogic(LOW, HIGH, HIGH);
  bool t3 = testLogic(HIGH, LOW, HIGH);
  bool t4 = testLogic(HIGH, HIGH, HIGH);
  return t1 && t2 && t3 && t4;
}

bool testNOT() {
  Serial.println("A | Exp | Act | Status");
  digitalWrite(IC_INPUT_B, LOW);
  bool t1 = testLogic(LOW, LOW, HIGH);
  bool t2 = testLogic(HIGH, LOW, LOW);
  return t1 && t2;
}

bool testNAND() {
  Serial.println("A | B | Exp | Act | Status");
  bool t1 = testLogic(LOW, LOW, HIGH);
  bool t2 = testLogic(LOW, HIGH, HIGH);
  bool t3 = testLogic(HIGH, LOW, HIGH);
  bool t4 = testLogic(HIGH, HIGH, LOW);
  return t1 && t2 && t3 && t4;
}

bool testLogic(bool inA, bool inB, bool expected) {
  digitalWrite(IC_INPUT_A, inA);
  digitalWrite(IC_INPUT_B, inB);
  delay(50);
  
  bool actual = digitalRead(IC_OUTPUT);
  bool pass = (actual == expected);
  
  Serial.print(inA);
  Serial.print(" | ");
  Serial.print(inB);
  Serial.print(" | ");
  Serial.print(expected);
  Serial.print("   | ");
  Serial.print(actual);
  Serial.print("   | ");
  Serial.println(pass ? "PASS" : "FAIL");
  
  return pass;
}

void displayICResult(bool passed) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("IC: ");
  display.print(icNames[currentIC]);
  
  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);
  
  display.setTextSize(3);
  display.setCursor(15, 25);
  display.print(passed ? "PASS" : "FAIL");
  
  display.setTextSize(1);
  display.setCursor(5, 55);
  display.print("Press any button");
  
  display.display();
  delay(5000);
  displayICSelection();
}

// ========== VOLTAGE MEASUREMENT ==========

void measureVoltage() {
  Serial.println("\n=== VOLTAGE MEASUREMENT ===");
  
  float voltage = readVoltage();
  
  Serial.print("Measured Voltage: ");
  Serial.print(voltage);
  Serial.println(" V");
  
  displayVoltageResult(voltage);
}

float readVoltage() {
  int adcValue = 0;
  // Average 10 readings for stability
  for(int i = 0; i < 10; i++) {
    adcValue += analogRead(VOLTAGE_PIN);
    delay(10);
  }
  adcValue /= 10;
  
  // Convert ADC to voltage at pin
  float vOut = (adcValue / ADC_RESOLUTION) * VCC;
  
  // Calculate actual voltage using voltage divider formula
  // Vin = Vout * (R1 + R2) / R2
  float vIn = vOut * (R1_VOLTAGE + R2_VOLTAGE) / R2_VOLTAGE;
  
  Serial.print("ADC: ");
  Serial.print(adcValue);
  Serial.print(" | Pin Voltage: ");
  Serial.print(vOut);
  Serial.print("V | Calculated Input: ");
  Serial.print(vIn);
  Serial.println("V");
  
  return vIn;
}

void displayVoltageResult(float voltage) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Voltage Measured:");
  
  display.setTextSize(2);
  display.setCursor(10, 25);
  display.print(voltage, 2);
  display.print(" V");
  
  display.setTextSize(1);
  display.setCursor(5, 55);
  display.print("Press to measure");
  
  display.display();
}

void displayVoltageLive() {
  float voltage = readVoltage();
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("VOLTMETER (Live)");
  
  display.setTextSize(3);
  display.setCursor(5, 25);
  display.print(voltage, 1);
  display.setTextSize(2);
  display.print("V");
  
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print("SWITCH:Mode TEST:Hold");
  
  display.display();
}

// ========== RESISTANCE MEASUREMENT ==========

void measureResistance() {
  Serial.println("\n=== RESISTANCE MEASUREMENT ===");
  
  float resistance = readResistance();
  
  Serial.print("Measured Resistance: ");
  
  if(resistance < 0) {
    Serial.println("ERROR or OPEN");
  } else if(resistance < 1000) {
    Serial.print(resistance);
    Serial.println(" Ω");
  } else if(resistance < 1000000) {
    Serial.print(resistance / 1000.0);
    Serial.println(" kΩ");
  } else {
    Serial.print(resistance / 1000000.0);
    Serial.println(" MΩ");
  }
  
  displayResistanceResult(resistance);
}

float readResistance() {
  float resistance = -1;
  float refResistor = 0;
  int selectedGPIO = 0;
  
  // Try R1 (100kΩ) first for high resistance
  digitalWrite(RES_GPIO1, HIGH);
  digitalWrite(RES_GPIO2, LOW);
  digitalWrite(RES_GPIO3, LOW);
  delay(100);
  
  int adc1 = 0;
  for(int i = 0; i < 10; i++) {
    adc1 += analogRead(RES_MEASURE);
    delay(10);
  }
  adc1 /= 10;
  float vOut1 = (adc1 / ADC_RESOLUTION) * VCC;
  
  // Try R2 (10kΩ) for medium resistance
  digitalWrite(RES_GPIO1, LOW);
  digitalWrite(RES_GPIO2, HIGH);
  digitalWrite(RES_GPIO3, LOW);
  delay(100);
  
  int adc2 = 0;
  for(int i = 0; i < 10; i++) {
    adc2 += analogRead(RES_MEASURE);
    delay(10);
  }
  adc2 /= 10;
  float vOut2 = (adc2 / ADC_RESOLUTION) * VCC;
  
  // Try R3 (1kΩ) for low resistance
  digitalWrite(RES_GPIO1, LOW);
  digitalWrite(RES_GPIO2, LOW);
  digitalWrite(RES_GPIO3, HIGH);
  delay(100);
  
  int adc3 = 0;
  for(int i = 0; i < 10; i++) {
    adc3 += analogRead(RES_MEASURE);
    delay(10);
  }
  adc3 /= 10;
  float vOut3 = (adc3 / ADC_RESOLUTION) * VCC;
  
  // Select best range (closest to VCC/2 for best accuracy)
  float diff1 = abs(vOut1 - VCC/2);
  float diff2 = abs(vOut2 - VCC/2);
  float diff3 = abs(vOut3 - VCC/2);
  
  float selectedVout = vOut1;
  refResistor = R1_REFERENCE;
  selectedGPIO = 1;
  
  if(diff2 < diff1 && diff2 < diff3) {
    selectedVout = vOut2;
    refResistor = R2_REFERENCE;
    selectedGPIO = 2;
  } else if(diff3 < diff1 && diff3 < diff2) {
    selectedVout = vOut3;
    refResistor = R3_REFERENCE;
    selectedGPIO = 3;
  }
  
  // Keep selected GPIO active
  digitalWrite(RES_GPIO1, selectedGPIO == 1 ? HIGH : LOW);
  digitalWrite(RES_GPIO2, selectedGPIO == 2 ? HIGH : LOW);
  digitalWrite(RES_GPIO3, selectedGPIO == 3 ? HIGH : LOW);
  
  Serial.print("Selected Range: R");
  Serial.print(selectedGPIO);
  Serial.print(" (");
  Serial.print(refResistor / 1000.0);
  Serial.print("kΩ) | Vout: ");
  Serial.print(selectedVout);
  Serial.println("V");
  
  // Calculate unknown resistance using voltage divider
  // Vout = VCC * Rx / (Rref + Rx)
  // Rx = (Vout * Rref) / (VCC - Vout)
  
  if(selectedVout < 0.1 || selectedVout > (VCC - 0.1)) {
    return -1; // Out of range
  }
  
  resistance = (selectedVout * refResistor) / (VCC - selectedVout);
  
  return resistance;
}

void displayResistanceResult(float resistance) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Resistance:");
  
  display.setTextSize(2);
  display.setCursor(5, 25);
  
  if(resistance < 0) {
    display.print("ERROR");
  } else if(resistance < 1000) {
    display.print(resistance, 1);
    display.print(" Ohm");
  } else if(resistance < 1000000) {
    display.print(resistance / 1000.0, 2);
    display.print(" k");
  } else {
    display.print(resistance / 1000000.0, 2);
    display.print(" M");
  }
  
  display.setTextSize(1);
  display.setCursor(5, 55);
  display.print("Press to measure");
  
  display.display();
}

void displayResistanceLive() {
  float resistance = readResistance();
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("OHMMETER (Live)");
  
  display.setTextSize(2);
  display.setCursor(5, 25);
  
  if(resistance < 0) {
    display.print("---");
  } else if(resistance < 1000) {
    display.print(resistance, 0);
    display.setTextSize(1);
    display.print(" Ohm");
  } else if(resistance < 1000000) {
    display.print(resistance / 1000.0, 1);
    display.setTextSize(1);
    display.print(" k");
  } else {
    display.print(resistance / 1000000.0, 1);
    display.setTextSize(1);
    display.print(" M");
  }
  
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print("SWITCH:Mode");
  
  display.display();
}