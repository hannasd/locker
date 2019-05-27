
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <string.h>
#include "config.h"

DynamicJsonDocument doc(2048);
JsonArray repos;
unsigned long timenow = 0;
PN532_SPI pn532spi(SPI, D2);
PN532 nfc(pn532spi);
uint8_t cards[30][4];
uint8_t temp[4];
HTTPClient http;

void buzzer(bool accepted) {
  if (accepted) {
    tone(BUZZER_PIN , BUZZER_FREQ);
    delay(250);
    noTone(BUZZER_PIN);
  } else {
    tone(BUZZER_PIN , BUZZER_FREQ);
    delay(1000);
    noTone(BUZZER_PIN);
  }
}

void unlock() {
  digitalWrite(LOCK_PIN , HIGH);
  delay(5000);
  digitalWrite(LOCK_PIN , LOW);
}

void connectToWifi(String networkName, String networkPassword) {
  int counter = 0;
  WiFi.begin(networkName, networkPassword);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (counter++ > WIFI_ATTEMPTS)
    {
      Serial.println("\nConnection Failed");
      return;
    }
  }
  Serial.print("\nConnected, IP address: ");
  Serial.println(WiFi.localIP());
}

void saveToEEPROM() {
  int count = 8; //First 8 bytes are reserved
  for (size_t i = 0; i < 30 ; i++)
  {
    for (size_t j = 0; j < 4; j++)
    {
      EEPROM.write(count , cards[i][j]);
      count++;
    }
  }
  EEPROM.write(512 , 30);
  EEPROM.commit();
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

  for (int i = 0; i < 8; i++) {
    output += uid[i];
  }
  output += "\",\"approved\":";
  if (accepted) {
    output += "true";
  } else {
    output += "false";
  }
  output += "}";
  http.begin(SERVER_ROOT LOG_URL);
  http.addHeader("Content-Type", "application/json");
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

int getListFromServer() {
  http.begin(SERVER_ROOT MEMBERSLIST_URL);
  int httpCode = http.GET();
  if (httpCode < 0) {
    return httpCode;
  }
  Stream& response = http.getStream();
  doc.clear();
  deserializeJson(doc, response);
  repos = doc.as<JsonArray>();
  return httpCode;
}

void readFromEEPROM() {
  //EEPROM first and second rows are reserved
  int c = 8;
  for (size_t i = 0; i < 30; i++)
  {
    for (size_t j = 0; j < 4 ; j++)
    {
      cards[i][j] = EEPROM.read(c++);
    }
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

bool isTimePassed(long long duration) {
  if (millis() - timenow > duration) {
    timenow = millis();
    return true;
  }
  return false;
}

void updateMembersList() {
  int result = getListFromServer();
  if (result == 200)
  {
    jsontoarr();
    saveToEEPROM();
  } else if (EEPROM.read(512) != 0xFF)
  {
    readFromEEPROM();
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
  EEPROM.begin(520);
  Serial.begin(115200);
  Serial.println("Hello!");
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LOCK_PIN , OUTPUT);
  digitalWrite(LOCK_PIN , LOW);
  digitalWrite(BUZZER_PIN , LOW);
  delay(1000);
  connectToWifi(WIFI_SSID, WIFI_PASS);
  updateMembersList();
  timenow = millis();
  nfc.begin();
  nfcSetup();
}

void loop(void) {
  boolean success;
  boolean accepted;
  uint8_t uid[] = { 0, 0, 0, 0 , 0 , 0 , 0  };
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (isTimePassed(UPDATE_INTERVAL)) {
    updateMembersList();
  }

  if (success) {
    accepted = isMember(uid);
    buzzer(accepted);
    if (accepted) {
      unlock();
    }
    POSTLog(toString(uid) , accepted);
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength)) {}
  }
}
