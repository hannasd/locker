
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
#include <string.h>
// JsonArray List;
DynamicJsonDocument doc(2048);
JsonArray repos ;
unsigned long timenow = 0;
PN532_SPI pn532spi(SPI, D2);
PN532 nfc(pn532spi);
Adafruit_SSD1306 display(128, 64, &Wire, D3);
//PN532_I2C pn532i2c(Wire);
uint8_t numberOfCards;
uint8_t cards[30][4];
uint8_t temp[4];
//const int capacity = 50 * JSON_OBJECT_SIZE(1);
int httpCodeGet = 0;
HTTPClient http;
const int capacity = JSON_OBJECT_SIZE(2);
StaticJsonDocument<capacity> postDoc;
void unlock() {

}

void connectToWifi(String networkName, String networkPassword) {
  WiFi.begin(networkName, networkPassword);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void saveToEEPROM() {
  int count = 9;
  for (size_t i = 0; i < 30 ; i++)
  {
    for (size_t j = 0; j < 4; j++)
    {
      EEPROM.write(count , cards[i][j]);
      count++;
    }
  }
  EEPROM.write(512 , 30);
}

void toId(const char st[8] , uint8_t t[4]) {
  // uint8_t* uid = new uint8_t [4];
  for (int i = 0; i < 4; ++i) {
    char xx = st[i * 2];
    char yy = st[i * 2 + 1];
    uint8_t x = xx - '0';
    uint8_t y = (yy - '0');
    if (xx >= 'A' )
    {
      x = xx - 'A' + 10 ;
    }
    if (yy >= 'A' )
    {
      y = yy - 'A' + 10;
    }

    t[i] = x * 16 + y;
  }
}

//zero-mode in arduinoJson library:
//input shouldn't be const or should hold the dyanamic doc alive because arduinoJson is changing the input
//or the programm may crash after several runs
void jsontoarr() {
  size_t uidSize = repos.size();
  int i = 0;
  for (JsonObject repo : repos) {
    toId(repo["uid"].as<char* >(), temp);
    for (int j = 0; j < 4; j++) {
      cards[i][j] = temp[j];
    }
    i++;
  }
}

char* toString (uint8_t uid[]) {
  char* st = new char[8];
  for (int i = 0; i < 4; ++i) {
    int x = uid[i] / 16;
    int y = uid[i] % 16;
    char xx = x + '0'  , yy = y + '0';
    if (x > 9) {
      xx = x - 10 + 'A';
    }
    if (y > 9) {
      yy = y - 10 + 'A';
    }
    st[i * 2] = xx;
    st[i * 2 + 1] = yy;
  }
  return st;
}

void POSTLog(const char* uid  , bool accepted ) {
  String output;
  output = " ";
  output += "{\"uid\":\"";
  
  for(int i = 0; i < 8; i++){
    output += uid[i];
  }
  output += "\",\"approved\":";
  if(accepted){
    output += "true";
  }else{
    output += "false";
  }
  output+= "}";
  Serial.println("output : ");
  Serial.println(output);
  http.begin("http://172.20.10.3:8000/core/entry-log/");
  http.addHeader("Content-Type", "application/json");
//{"uid":"123456789","approved":true}
  int httpCode = http.POST(output);
  if (httpCode < 0) {
    Serial.printf("Status :::: [HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
bool isMember(uint8_t uid[]) {
  int count = 0, j = 0;
  int columnSize = sizeof(cards[0]) / sizeof(cards[0][0]);

  for (int k = 0; k < 30 ; k++) {
    for (int i = 0; i < 4; i++) {
      if (cards[k][i] != uid[j]) {
        count = 0;
        j = 0;
        break;
      }
      else if (cards[k][i] == uid[j]) {
        count++;
        if (count == 4) {
          Serial.println("Match Found");
          return true;
        }
        j++;
      }

    }
  }
  Serial.println("Match Not Found");
  return false;
}

void getListFromServer() {
  http.begin("http://172.20.10.3:8000/core/members-list/");
  httpCodeGet = http.GET();
  if (httpCodeGet < 0) {
    Serial.printf("Status :::: [HTTP] GET... failed, error: %s\n", http.errorToString(httpCodeGet).c_str());
    return;
  }
  Stream& response = http.getStream();
  deserializeJson(doc, response);
  repos = doc.as<JsonArray>();
  Serial.println("data from server: ");
  for (JsonObject repo : repos) {
    // Serial.print(" _ ");
    Serial.println(repo["uid"].as<char* >());
    // repo =  stoi (repo["uid"],NULL,10);
    // repo = (int) strtol(repos["uid"], (char **)NULL, 10);

  }
  Serial.println("END");
  // return repos;
}

void readFromEEPROM() {

  //EEPROM first and second rows are reserved
  int c = 9;
//  if (EEPROM.read(512) == 0xFF)
//    return;
Serial.println("EEPROM data : ");
  for (size_t i = 0; i <30; i++)
  {
    for (size_t j = 0; j < 4 ; j++)
    {
      Serial.print(EEPROM.read(c));
      cards[i][j] = EEPROM.read(c++);
    }
    Serial.println();
  }
}

void nfcSetup() {
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
void checkTime() {
  if (millis() - timenow > 30000 ) {
    getListFromServer();
    Serial.println("30 seconds past");
    timenow = millis();

  }
}
void printUidValue(uint8_t uid[] , uint8_t uidLength) {

  Serial.println("Found a card!");
  Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
  Serial.print("UID Value: ");
  for (uint8_t i = 0; i < uidLength; i++)
  {
    Serial.print(" 0x"); Serial.print(uid[i], HEX);

  }
}
void setup(void) {
  EEPROM.begin(513);
  Serial.begin(115200);
  Serial.println("Hello!");
  connectToWifi("iPhone", "12345678987");
  //  // lcd
  getListFromServer();
  if (httpCodeGet < 0) {
    if (EEPROM.read(512) != 0xFF)
      readFromEEPROM();
  } else {
    jsontoarr();
    saveToEEPROM();
  }
  timenow = millis();
  Serial.println("EEPROM data : ");
  int c = 0;
  for (size_t i = 0; i <30; i++)
  {
    for (size_t j = 0; j < 4 ; j++)
    {
      Serial.print(EEPROM.read(c));
      cards[i][j] = EEPROM.read(c++);
    }
    Serial.println();
  }
  //lcdBegin();
  nfc.begin();
  nfcSetup();
  uint8_t tempuid[] = {0xFF , 0xA2, 0xFF , 0xFF};
}
void loop(void) {
  boolean success;
  boolean accepted = true;
  uint8_t uid[] = { 0, 0, 0, 0 , 0 , 0 , 0  };
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  checkTime();
  if (httpCodeGet < 0) {

  } else {
    jsontoarr();
    saveToEEPROM();
  }
  if (success) {
    printUidValue(uid , uidLength);
    Serial.println("data in cards");
    for (int i = 0 ; i < 4 ; i++)
      Serial.println(cards[0][i]);
    if (isMember(uid)) {
      POSTLog(toString(uid) , true);
    } else {
    POSTLog(toString(uid) , false);
    }
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength)) {}
  }

  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }
  

}
