#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>

#include <Adafruit_SSD1306.h>
//#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

Adafruit_SSD1306 display(128,64,&Wire,D3);
 uint8_t amirali[]={0x79,0x6,0xF,0x1A};
PN532_SPI pn532spi(SPI, D4);
//PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532spi);
uint8_t numberOfCards;
uint8_t cards[128][4];
//const int capacity = 50 * JSON_OBJECT_SIZE(1);

HTTPClient http;
uint32_t uidAsInteger(uint8_t uid[]) {
  int result = 0;
  for (int i = 0 ; i < 4; i++) {
    result += (uid[4 - i - 1] * (1 << (4 * 2 * i)));
  }
  return result;
}

void saveToEEPROM(uint8_t uid[]) {
  for (uint8_t i = 0 ; i < 4; i++) {
    EEPROM.write((numberOfCards * 4) + i , uid[i]);
  }
  numberOfCards += 1;
  EEPROM.write(511 , numberOfCards);

}

bool isMember(uint8_t uid[]) {
  for (int i = 0 ; i < numberOfCards ; i++) {
    for (int j = 0; j < 4; j++) {
      if (cards[i][j] != uid[j]) {
        break;
      }
      if (j == 3) {
        return true;
      }
    }
  }
  return false;
}

//void getListFromServer(){
//  http.begin("http://arminmaz.pythonanywhere.com/core/update/");
//  int httpCode = http.GET();
//  if(httpCode < 0){
//    Serial.printf("Status :::: [HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
//  }
//  Stream& response = http.getStream();
//  DynamicJsonDocument doc(2048);
//
////  deserializerJson(doc, response);
//
//  JsonArray repos = doc["uid"];
//
//  for(JsonObject repo : repos){
//    Serial.print(" _ ");
//    Serial.print(repo["uid"].as<char* >());
////      repo =  std::stoi (repos,NULL,16);
//  }
//  String s = http.getString();
//
//  char json[50];
//  strcpy(json,s.c_str());
//  deserializeJson(doc, json);
//  JsonArray arr = doc.as<JsonArray>();
//

//
//void saveList(JsonArray repos) {
//  for (JsonObject repo : repos) {
//    int i = 0;
//    EEPROM.put(0 , repo);
//    EEPROM.commit();
//  }
//}

void checkForUpdate() {
  http.begin("http://arminmaz.pythonanywhere.com/core/update/");

}


void setup(void) {
  EEPROM.begin(513);
  Serial.begin(115200);
  Serial.println("Hello!");
  EEPROM.write(512, 0);
  // set up the LCD's number of columns and rows:
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Hello");
  display.display();

  nfc.begin();

  

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A card");
}

void loop(void) {
  boolean success;
  // int id = 0;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (success) {
    Serial.println("Found a card!");
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      Serial.print(" 0x"); Serial.print(uid[i], HEX);

    }
    for(int i = 0 ; i < 4 ; i++){
    if(uid[i] == amirali[i] ){
      if(i == 3){
       display.clearDisplay();
      display.setCursor(0 , 0);
      display.print("welcom amilaliiee");
      display.display();
      }
    }
      else if(amirali[i]!=uid[i]){
      display.clearDisplay();
      display.setCursor(0 , 0);
      display.print("Authentication failed!");
       display.display();
      }
    }
  
 
//    display.clearDisplay();
//    display.setCursor(0,0);
//    display.print(uidAsInteger(uid));
//    display.display();
//    Serial.println("id as int:");
//    //    Serial.println(uid);
//    Serial.println("");

}
    // wait until the card is taken away
    
    //send uid to server and get response and savedata
  
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }
  while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength)) {}
}
