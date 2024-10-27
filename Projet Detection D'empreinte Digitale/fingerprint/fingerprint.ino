#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
SoftwareSerial mySerial(2, 3); 
#else
#define mySerial Serial1
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
bool matchedDisplayed = false;
unsigned long displayTimeout = 0;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.print("Initializing...");
  delay(1000);

  finger.begin(57600);
  lcd.clear();

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
    lcd.print("Sensor found!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    lcd.print("Sensor not found");
    while (1) { delay(1); }
  }
  delay(2000);
}

void loop() {
  if (matchedDisplayed && millis() - displayTimeout >= 5000) {  // Reset after 5 seconds
    matchedDisplayed = false;
    lcd.clear();
    lcd.print("Place Finger");
  }
  
  if (!matchedDisplayed) {  // Only scan if no match is displayed
    getFingerprintID();
  }
  delay(100);
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();

  if (p == FINGERPRINT_NOFINGER) return p;

  if (p == FINGERPRINT_OK) {
    Serial.println("Image taken");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    lcd.clear();
    lcd.print("Comm error");
    return p;
  } else if (p == FINGERPRINT_IMAGEFAIL) {
    lcd.clear();
    lcd.print("Imaging error");
    return p;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Error");
    return p;
  }

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    uint8_t id = finger.fingerID;
    String name = getNameFromEEPROM(id);

    Serial.print("Found a print match! Name: ");
    Serial.println(name);  // Print the name to Serial Monitor

    lcd.clear();
    delay(200);  // Small delay to ensure LCD clears properly
    lcd.print("Match: ");
    lcd.setCursor(0, 1);
    lcd.print(name);  // Display the name on the second line

    matchedDisplayed = true;
    displayTimeout = millis();
  } else {
    lcd.clear();
    lcd.print("No match");
  }

  return p;
}

String getNameFromEEPROM(uint8_t id) {
  int address = id * 20;
  char name[21];  // Buffer to hold the name (20 bytes + null terminator)

  for (int i = 0; i < 20; i++) {
    name[i] = EEPROM.read(address + i);
    if (name[i] == 0) break;  // Stop if null character is found
  }
  name[20] = '\0';  // Ensure the string is null-terminated

  Serial.print("Read name from EEPROM: ");
  Serial.println(name);  // Debugging output

  return String(name);
}
