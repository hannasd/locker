
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
PN532_SPI pn532spi(SPI, D4);
PN532 nfc(pn532spi);
Adafruit_SSD1306 display(128, 64, &Wire, D3);
// uint8_t amirali[]={0x79,0x6,0xF,0x1A};
// uint8_t hanna[] = {0xCD,0x77,0x6A,0xA};
//PN532_I2C pn532i2c(Wire);
uint8_t eepromLoc = 0;
uint8_t numberOfCards;
uint8_t cards[30][4];
uint8_t temp[4];
//const int capacity = 50 * JSON_OBJECT_SIZE(1);
HTTPClient http;
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
      EEPROM.write(count++ , cards[i][j]);
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
  Serial.print("repos.size() : ");
  Serial.println(uidSize);
  int i = 0;
  for (JsonObject repo : repos) {
    Serial.println("Before toId.");
    Serial.println(repos.size());
    Serial.println(repo["uid"].as<char* >());
    toId(repo["uid"].as<char* >(), temp);
    Serial.println("After toId");
    // for(int j = 0; j < 4; j++){
    //   Serial.print("temp[");
    //   Serial.print(j);
    //   Serial.print("] : ");
    //   Serial.println(temp[j]);
    // }
    for (int j = 0; j < 4; j++) {
      Serial.print("temp[");
      Serial.print(j);
      Serial.print("] : ");
      Serial.println(temp[j]);
      cards[i][j] = temp[j];
    }
    i++;
    Serial.println("cards : ");
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 4; ++k)
      {
        Serial.print(cards[j][k]);
      }
      Serial.println();
    }

  }
  //   for(int i = 0; i < uidSize; i++){
  //     Serial.println("Before toId.");
  //     Serial.println(repos.size());
  //     Serial.println(repos[i]["uid"].as<char* >());
  //     toId(repos[i]["uid"].as<char* >(), temp);
  //     Serial.println("After toId");
  //     for(int j = 0; j < 4; j++){
  //       Serial.print("temp[");
  //       Serial.print(j);
  //       Serial.print("] : ");
  //       Serial.println(temp[j]);
  //     }
  //     for(int j = 0; j < 4; j++){
  //       Serial.print("temp[");
  //       Serial.print(j);
  //       Serial.print("] : ");
  //       Serial.println(temp[j]);
  //       // cards[i][j] = temp[j];
  //     }
  // //     Serial.println();
  // //   }
  // // //  for(int i = 0; i <uidSize;  i++)
  // //  {
  // //    Serial.print("cards[i][j] : ");
  // //    for(int j = 0; j < 4 ; j++)
  // //    {
  // //      cards[i][j] = uid[i][j];
  // //      Serial.print(cards[i][j]);
  // //    }
  // //    Serial.println();
  // //    //delete uid[i];
  //  }
  // saveToEEPROM(uid);
  // delete uid;
}

char* toString (uint8_t uid[]) {
  //    char* st = new char[8];
  //    for (int i = 0; i < 4; ++i) {
  //        int x = uid[i]/16;
  //        int y = uid[i]%16;
  //        char xx = x + '0'  , yy = y + '0';
  //        if (x > 9){
  //            xx = x -10 + 'A';
  //        }
  //        if (y > 9){
  //            yy = y -10 + 'A';
  //        }
  //        cout<<xx <<" "<<yy<<endl;
  //        st[i*2]=xx;
  //        st[i*2+1]=yy;
  //    }
  //    for (int j = 0; j < 8; ++j) {
  //        cout<<st[j];
  //    }
  //    cout<<endl;
  //    return st;
}
void POSTLog(uint8_t uid  , bool accepted ) {
  // char* suid = toString(uid);
  //  DynamicJsonDocument doc(1024);
  // //hextoString();
  // doc["uid"] = suid;
  // doc["authented"] = accepted;
  // serializeJson(doc, Serial);
  // JsonArray logs = doc["uid"]["authented"];
  // http.begin("");

  //  http.POST(logs[0]);
  //  const int capacity = JSON_OBJECT_SIZE(2);
  //  StaticJsonDocument<capacity> postDoc;
  //  postDoc["uid"].set(uid);
  //  postDoc["accepted"].set(accepted);
  //  JsonObject postObj = postDoc.to<JsonObject>();
  //  http.begin("http://172.20.10.4:8000/core/entry-log/");
  //

}
bool isMember(uint8_t uid[]) {
  int count = 0, j = 0;
  int columnSize = sizeof(cards[0]) / sizeof(cards[0][0]);

  for (int k = 0; k < 30 ; k++) {
    for (int i = 0; i < 4; i++) {
      Serial.print("cards[k][i] : ");
      Serial.println(cards[k][i]);
      Serial.print("uid[j] : ");
      Serial.println(uid[j]);
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
    Serial.println();
  }
  Serial.println("Match Not Found");
  return false;
}

void getListFromServer() {
  http.begin("http://172.20.10.4:8000/core/members-list/");
  int httpCode = http.GET();
  if (httpCode < 0) {
    Serial.printf("Status :::: [HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    //Should read the list from EEPROM
  }
  Stream& response = http.getStream();
  deserializeJson(doc, response);
  repos = doc.as<JsonArray>();
  for (JsonObject repo : repos) {
    // Serial.print(" _ ");
    Serial.println(repo["uid"].as<char* >());
    // repo =  stoi (repo["uid"],NULL,10);
    // repo = (int) strtol(repos["uid"], (char **)NULL, 10);

  }
  for (int i = 0; i < 10; i++)
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
  //should be removed later
  if (EEPROM.read(512) == 0xFF)
    return;
  for (size_t i = 0; i < EEPROM.read(512); i++)
  {
    for (size_t j = 0; j < 4 ; j++)
    {
      cards[i][j] = EEPROM.read(c++);
    }
  }
}
void setup(void) {
  EEPROM.begin(513);
  Serial.begin(115200);
  Serial.println("Hello!");
  //  // lcd
  //
  //  if( getListFromServer() == NULL){
  //      cards = readfromEEPROM();
  //  }else
  //  {
  //    JsonArray list = getListFromServer();
  //    jsontoarr(list);
  //
  //  }
  //
  //  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  //  display.clearDisplay();
  //  display.setTextSize(2);
  //  display.setTextColor(WHITE);
  //  display.setCursor(0,0);
  //  display.print("Hello");
  //  display.display();
  //
  //  nfc.begin();
  //
  //  timenow = millis();
  //  uint32_t versiondata = nfc.getFirmwareVersion();
  //  if (! versiondata) {
  //    Serial.print("Didn't find PN53x board");
  //    display.clearDisplay();
  //    display.setTextColor(WHITE);
  //    display.setCursor(1 , 1);
  //    display.print("board connection failed!");
  //    while (1); // halt
  //  }
  //
  //  // Got ok data, print it out!
  //  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  //  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  //  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);
  //
  //  // Set the max number of retry attempts to read from a card
  //  // This prevents us from waiting forever for a card, which is
  //  // the default behaviour of the PN532.
  //  nfc.setPassiveActivationRetries(0xFF);
  //
  //  // configure board to read RFID tags
  //  nfc.SAMConfig();
  //
  //  Serial.println("Waiting for an ISO14443A card");
  uint8_t tempuid[] = {0xFF , 0xA2, 0xFF , 0xFF};
  connectToWifi("M.Mahdi2414", "010684123");
  getListFromServer();
  jsontoarr();
  //isMember(tempuid);
  Serial.println("After reading from server");
  saveToEEPROM();
  Serial.println("After saving from EEPROM");
   for (int i = 0 ; i < 10 ; i++) {
    Serial.printf("cards[%d]: ", i);
    for (int j = 0 ; j < 4 ; j++) {

      Serial.print(cards[i][j]);
      cards[i][j] = 0;

    }
    Serial.println();
  }
  Serial.println("After null");
  for (int i = 0 ; i < 10 ; i++) {
    Serial.printf("cards[%d]: ", i);
    for (int j = 0 ; j < 4 ; j++) {

      Serial.print(cards[i][j]);

    }
    Serial.println();
  }
  readFromEEPROM();
  Serial.println("After reading from EEPROM");
  for (int i = 0 ; i < 10 ; i++) {
    Serial.printf("cards[%d]: ", i);
    for (int j = 0 ; j < 4 ; j++) {

      Serial.print(cards[i][j]);

    }
    Serial.println();
  }
}
void loop(void) {
  //  boolean success;
  //  boolean accepted = true;
  //  uint8_t uid[] = { 0, 0, 0, 0 };
  //  uint8_t uidLength;
  //
  //  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  //
  //  if (success) {
  //
  //    Serial.println("Found a card!");
  //    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
  //    Serial.print("UID Value: ");
  //    for (uint8_t i = 0; i < uidLength; i++)
  //    {
  //      Serial.print(" 0x"); Serial.print(uid[i], HEX);
  //
  //    }
  //    if(millis() - timenow > 30000 ){
  //      List =  getListFromServer();
  //      jsontoarr(List);
  //       timenow = millis();
  //    }
  //
  //    if(isMember(uid)){
  //      unlock();
  //       display.clearDisplay();
  //       display.setCursor(2 , 2);
  //       display.print("Welcome");
  //       display.display();
  //       POSTLog(uid , accepted);
  //    }
  //    else
  //      POSTLog(uid , !accepted);
  //
  //    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength)) {}
  //    }
  //
  //  else
  //  {
  //    // PN532 probably timed out waiting for a card
  //    Serial.println("Timed out waiting for a card");
  //  }
  //   display.clearDisplay();
  //  display.print("hello");
  //  display.display();
}
