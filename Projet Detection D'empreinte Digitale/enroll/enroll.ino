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
uint8_t nextID = 1; // Track the next available ID

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);  
  lcd.backlight();
  lcd.print("Initializing...");

  finger.begin(57600);
  delay(100);

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found!");
    lcd.clear();
    lcd.print("Sensor found!");
  } else {
    Serial.println("Sensor not found :(");
    lcd.clear();
    lcd.print("Sensor not found");
    while (1) { delay(1); }
  }
}

String readString() {
  while (!Serial.available());
  return Serial.readStringUntil('\n');
}

void loop() {
  Serial.println("Ready to enroll a fingerprint!");
  lcd.clear();
  lcd.print("Enroll fingerprint");

  if (nextID >= 128) {
    Serial.println("Storage full.");
    lcd.clear();
    lcd.print("Storage full");
    return;
  }

  Serial.print("Enter name for new entry: ");
  lcd.clear();
  lcd.print("Enter name:");
  String name = readString();
  
  Serial.print("Enrolling for Name: ");
  Serial.println(name);
  lcd.clear();
  lcd.print("Enrolling for ");
  lcd.print(name);

  if (getFingerprintEnroll(nextID)) {
    storeNameInEEPROM(nextID, name);  // Store name in EEPROM
    nextID++;  // Move to the next available ID only if enrollment is successful
  }
}

uint8_t getFingerprintEnroll(uint8_t id) {
  int p = -1;
  Serial.print("Waiting for finger for ");
  Serial.println(id);
  lcd.clear();
  lcd.print("Place finger");

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      lcd.clear();
      lcd.print("Image taken");
      break;
    } else if (p == FINGERPRINT_NOFINGER) {
      delay(100);
    } else {
      return p;
    }
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return p;
  
  Serial.println("Remove finger");
  lcd.clear();
  lcd.print("Remove finger");
  delay(2000);

  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  Serial.println("Place same finger again");
  lcd.clear();
  lcd.print("Place again");

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return p;

  p = finger.createModel();
  if (p != FINGERPRINT_OK) return p;

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.print("Stored! Name: ");
    Serial.println(id);
    lcd.clear();
    lcd.print("Stored: ");
    lcd.setCursor(0, 1);
    lcd.print(id);
  } else {
    Serial.println("Error storing fingerprint.");
    lcd.clear();
    lcd.print("Store error");
    return p;
  }

  return true;
}

void storeNameInEEPROM(uint8_t id, String name) {
  int address = id * 20; // Reserve 20 bytes per name in EEPROM
  for (int i = 0; i < 20; i++) {
    if (i < name.length()) {
      EEPROM.write(address + i, name[i]);
    } else {
      EEPROM.write(address + i, 0); // Pad remaining bytes with nulls
    }
  }
  Serial.print("Stored name in EEPROM: ");
  Serial.println(name);  // Debugging output
}
