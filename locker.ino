#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <time.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#define amirali 0
#define hanna 1
#define aghaye_asadi_rahmani 2
unsigned long timenow = 0;
PN532_SPI pn532spi(SPI, D4);
PN532 nfc(pn532spi);
Adafruit_SSD1306 display(128,64,&Wire,D3);
// uint8_t amirali[]={0x79,0x6,0xF,0x1A};
// uint8_t hanna[] = {0xCD,0x77,0x6A,0xA};
//PN532_I2C pn532i2c(Wire);
uint8_t eepromLoc = 0;
uint8_t numberOfCards;
uint8_t cards[128][4] = {{0x79,0x6,0xF,0x1A} , {0xCD,0x77,0x6A,0xA}};
//const int capacity = 50 * JSON_OBJECT_SIZE(1);
HTTPClient http;


unsigned int uidAsInteger(uint8_t uid[]) {
  int result = 0;
  for (int i = 0 ; i < 4; i++) {
    result += (uid[4 - i - 1] * (1 << (4 * 2 * i)));
  }
  return result;
}

void saveToEEPROM(JsonArray repos ) {
    numberOfCards = repos.size();
   for(int i = 0 ; i < repos.size() ; i++){
    EEPROM.write(i ,   uidAsInteger(repos[i]));
   }
  EEPROM.write(512 , numberOfCards);
  return;
}


void POSTLog(unsigned int uid  , bool accepted ){

   DynamicJsonDocument doc(1024);
   
  doc["uid"] = uid;
  doc["authented"] = accepted; 
  serializeJson(doc, Serial);
  JsonArray logs = doc["uid"]["authented"];
    http.begin("");
    
    http.POST(logs[0]);
  }

bool isMember(unsigned int uid)
{
  for(int i = 0 ; i < EEPROM.read(512) ; i++){
    if(EEPROM.read(i) == uid ){
     return true;
    }
  }
  return false;
}


 void getListFromServer(){
  http.begin("http://arminmaz.pythonanywhere.com/core/update/");
  int httpCode = http.GET();
  if(httpCode < 0){
    Serial.printf("Status :::: [HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  } 
  Stream& response = http.getStream();
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, response);
  JsonArray repos = doc["uid"];

  for(JsonObject repo : repos){
    Serial.print(" _ ");
    Serial.print(repo["uid"].as<char* >());
     // repo =  stoi (repo["uid"],NULL,10);
    // repo = (int) strtol(repos["uid"], (char **)NULL, 10);

  }
  String s = http.getString();

  char json[50];
  strcpy(json,s.c_str());
  deserializeJson(doc, json);
  JsonArray arr = doc.as<JsonArray>();
  saveToEEPROM(repos);
 }

void checkForUpdate() {
  http.begin("http://arminmaz.pythonanywhere.com/core/update/");

}


void setup(void) {
  EEPROM.begin(513);
  Serial.begin(115200);
  Serial.println("Hello!");
  getListFromServer();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Hello");
  display.display();

  nfc.begin();

  timenow = millis();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(1 , 1);
    display.print("board connection failed!");
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
  boolean accepted = false;
  int uidInt;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  
  uint8_t uidLength;                        
  
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  //harsi sanie yebar post kone list begire
  if (success) {
    Serial.println("Found a card!");
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      Serial.print(" 0x"); Serial.print(uid[i], HEX);

    }
    uidInt = uidAsInteger(uid);
    if(millis() - timenow > 30000 ){
      getListFromServer();
       timenow = millis();
    }
    //check she doroste ya na
    if(isMember(uidInt)){
      // unlock();
       //age dorost bud log befreste
       POSTLog(uidInt , !accepted);
    }
    else
      POSTLog(uidInt , accepted);
    
  
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength)) {}
    }

  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }
   display.clearDisplay();
  display.print("hello");
  display.display();
}
   
 
