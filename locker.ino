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
JsonArray List;
unsigned long timenow = 0;
PN532_SPI pn532spi(SPI, D4);
PN532 nfc(pn532spi);
Adafruit_SSD1306 display(128,64,&Wire,D3);
// uint8_t amirali[]={0x79,0x6,0xF,0x1A};
// uint8_t hanna[] = {0xCD,0x77,0x6A,0xA};
//PN532_I2C pn532i2c(Wire);
uint8_t eepromLoc = 0;
uint8_t numberOfCards;
uint8_t cards[50][4];
//const int capacity = 50 * JSON_OBJECT_SIZE(1);
HTTPClient http;
  void unlock(){

 }
void saveToEEPROM(uint8_t uid[][4]){
  int count = 0;
  for(size_t i = 0; i <sizeof(uid)/sizeof(uid[0]) ; i++)
  {
    for(size_t j = 0; j < sizeof(uid[0])/sizeof(uid[0][0]); j++)
    {
      EEPROM.write(count++ , uid[i][j]);
    }
  }
  EEPROM.write(512 , sizeof(uid)/sizeof(uid[0]));
}   

uint8_t* toId(char* st){
    uint8_t* uid = new uint8_t [4];
    for (int i = 0; i < 4; ++i) {
        char xx = st[i*2];
        char yy = st[i*2 + 1];
        uint8_t x = xx - '0';
        uint8_t y = (yy - '0');
        if (xx >= 'A' )
        {
            x = xx - 'A' + 10 ;
        }
        if (yy >= 'A' )
        {
            y = yy - 'A' +10;
        }
//        cout<<x<<" "<<y<<endl;
        uid[i] = x*16 + y;
    }
    return uid;
}
void jsontoarr(JsonArray arr){
  size_t size = sizeof(arr)/sizeof(arr[0]);
  uint8_t uid[size][4];
  //char delim = " "  , *ptr;
    for (JsonArray array :arr){
      int i = 0;
      uid[i++] =  toId(array);
          
    }
     for(size_t i = 0; i <size  i++)
     {
       for(size_t j = 0; j < 4 ; j++)
       {
         cards[i][j] = uid[i][j];
       } 
     }
    saveToEEPROM(uid);
}
 char* hextoString(uint8_t uid[]){
   char suid[] ;
   for(size_t i = 0; i < 4; i++)
   {  
       suid[i] = "0x" + itoa(uid , suid , 16 )
   }
   
     
 }
 char* toString (uint8_t uid[]){
    char* st = new char[8];
    for (int i = 0; i < 4; ++i) {
        int x = uid[i]/16;
        int y = uid[i]%16;
        char xx = x + '0'  , yy = y + '0';
        if (x > 9){
            xx = x -10 + 'A';
        }
        if (y > 9){
            yy = y -10 + 'A';
        }
        cout<<xx <<" "<<yy<<endl;
        st[i*2]=xx;
        st[i*2+1]=yy;
    }
    for (int j = 0; j < 8; ++j) {
        cout<<st[j];
    }
    cout<<endl;
    return st;
}
void POSTLog(uint8_t uid[]  , bool accepted ){
   char* suid = toString(uid);
    DynamicJsonDocument doc(1024);
   //hextoString();
   doc["uid"] = suid;
   doc["authented"] = accepted; 
   serializeJson(doc, Serial);
   JsonArray logs = doc["uid"]["authented"];
   http.begin("");
    
    http.POST(logs[0]);
  }

bool isMember(int uid[]) {
    int count = 0, j = 0;
    for (size_t k = 0; k <2; k++) {
        for (int i = 0; i < 4; i++) {
            if (j == 4)
                j = 0;
            if (cards[k][i] != uid[j])
                count = 0;
            else if (cards[k][i] == uid[j]) {
                count++;
                if (count == 4)
                    return true;
            }
            j++;
        }
    }
    return false;

}


JsonArray getListFromServer(){
  http.begin("http://arminmaz.pythonanywhere.com/core/update/");
  int httpCode = http.GET();
  if(httpCode < 0){
    Serial.printf("Status :::: [HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    return NULL;
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
  return repos; 
}
 
 void readfromEEPROM(){
   int c = 0;
   if(EEPROM.read(512) == 0xFF)
      return;
     for(size_t i = 0; i < EEPROM.read(512); i++)
     {
       for(size_t j = 0; j < 4 ; j++)
       {
         cards[i][j] = EEPROM.read(c++);
       } 
     }
  }
void setup(void) {
  EEPROM.begin(513);
  Serial.begin(115200);
  Serial.println("Hello!");
  // lcd

  if( getListFromServer() == NULL){
      cards = readfromEEPROM();
  }else
  {
    JsonArray list = getListFromServer();
    jsontoarr(list);
    
  }
   
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
  boolean accepted = true;
  uint8_t uid[] = { 0, 0, 0, 0 };  
  uint8_t uidLength;                        
  
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (success) {

    Serial.println("Found a card!");
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      Serial.print(" 0x"); Serial.print(uid[i], HEX);

    }
    if(millis() - timenow > 30000 ){
      List =  getListFromServer();
      jsontoarr(List);
       timenow = millis();
    }
    
    if(isMember(uid)){
      unlock();
       display.clearDisplay();
       display.setCursor(2 , 2);
       display.print("Welcome");
       display.display();
       POSTLog(uid , accepted);
    }
    else
      POSTLog(uid , !accepted);
    
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
   
 
