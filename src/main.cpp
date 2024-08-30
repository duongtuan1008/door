// chân của keypad 14, 27, 26, 25 33, 32, 16, 17
//  chân của RFID | rs : 0 |MISO : 19 | MOSI :23| SCK :18| SDA : 5
//  SEVOR :2
//  lCD : SDA :21 |SCL :22
#include <Arduino.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include "WiFi.h"
#include <Wire.h>
#include <FirebaseESP32.h>
#define FIREBASE_AUTH "Z4smC62E7MrAC0uPS8NNSxlZJlBp9JRESuQ89f76"
#define FIREBASE_HOST "appi-2fe3d-default-rtdb.firebaseio.com"
const char *ssid = "tuanquang";
const char *passwordwifi = "88888888";
FirebaseData firebaseData;
String path = "/";
void initFirebase()
{
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  if (!Firebase.beginStream(firebaseData, path))
  {
    Serial.println("REASON: " + firebaseData.errorReason());
    Serial.println();
  }
  else
  {
    Serial.println("Kết nối Firebase thành công");
  }
}
#define PIN_SG90 2
unsigned char id = 0;
unsigned char id_rf = 0;
unsigned char index_t = 0;
unsigned char error_in = 0;
unsigned char in_num = 0, error_pass = 0, isMode = 0;

// init keypad
const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns

char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {14, 27, 26, 25}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {33, 32, 16, 17}; // connect to the column pinouts of the keypad

int addr = 0;
char password[6] = "11111";
char pass_def[6] = "12345";
char mode_changePass[6] = "*#01#";
char mode_resetPass[6] = "*#02#";
char mode_hardReset[6] = "*#03#";
char mode_addRFID[6] = "*101#";
char mode_delRFID[6] = "*102#";
char mode_delAllRFID[6] = "*103#";
char data_input[6];
char new_pass1[6];
char new_pass2[6];
#define SS_PIN 5  // ESP32 pin GIOP5
#define RST_PIN 4 // ESP32 pin GIOP27
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
byte nuidPICC[4];
typedef enum
{
  MODE_ID_RFID_ADD,
  MODE_ID_RFID_FIRST,
  MODE_ID_RFID_SECOND,
} MODE_ID_RFID_E;

// unsigned char MODE = MODE_ID_FINGER_ADD; // Mode = 3
unsigned char MODE_RFID = MODE_ID_RFID_ADD;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
Servo sg90;

void writeEpprom(char data[])
{
  for (unsigned char i = 0; i < 5; i++)
  {
    EEPROM.write(i, data[i]);
  }
  EEPROM.commit(); // Đảm bảo lưu dữ liệu
}

void readEpprom()
{
  for (unsigned char i = 0; i < 5; i++)
  {
    password[i] = EEPROM.read(i);
  }
}

void clear_data_input() // xoa gia tri nhap vao hien tai
{
  int i = 0;
  for (i = 0; i < 6; i++)
  {
    data_input[i] = '\0';
  }
}

unsigned char isBufferdata(char data[]) // Kiem tra buffer da co gia tri chua
{
  unsigned char i = 0;
  for (i = 0; i < 5; i++)
  {
    if (data[i] == '\0')
    {
      return 0;
    }
  }
  return 1;
}

bool compareData(char data1[], char data2[]) // Kiem tra 2 cai buffer co giong nhau hay khong
{
  unsigned char i = 0;
  for (i = 0; i < 5; i++)
  {
    if (data1[i] != data2[i])
    {
      return false;
    }
  }
  return true;
}

void insertData(char data1[], char data2[]) // Gan buffer 2 cho buffer 1
{
  unsigned char i = 0;
  for (i = 0; i < 5; i++)
  {
    data1[i] = data2[i];
  }
}

void getData() // Nhan buffer tu ban phim
{
  char key = keypad.getKey(); // Doc gia tri ban phim
  if (key)
  {
    delay(100);
    // Serial.println("key != 0");
    if (in_num == 0)
    {
      data_input[0] = key;
      lcd.setCursor(5, 1);
      lcd.print(data_input[0]);
      delay(100);
      lcd.setCursor(5, 1);
      lcd.print("*");
    }
    if (in_num == 1)
    {
      data_input[1] = key;
      lcd.setCursor(6, 1);
      lcd.print(data_input[1]);
      delay(100);
      lcd.setCursor(6, 1);
      lcd.print("*");
    }
    if (in_num == 2)
    {
      data_input[2] = key;
      lcd.setCursor(7, 1);
      lcd.print(data_input[2]);
      delay(100);
      lcd.setCursor(7, 1);
      lcd.print("*");
    }
    if (in_num == 3)
    {
      data_input[3] = key;
      lcd.setCursor(8, 1);
      lcd.print(data_input[3]);
      delay(100);
      lcd.setCursor(8, 1);
      lcd.print("*");
    }
    if (in_num == 4)
    {
      data_input[4] = key;
      lcd.setCursor(9, 1);
      lcd.print(data_input[4]);
      delay(100);
      lcd.setCursor(9, 1);
      lcd.print("*");
    }
    if (in_num == 4)
    {
      Serial.println(data_input);
      in_num = 0;
    }
    else
    {
      in_num++;
    }
  }
}
void checkPass() // kiem tra password
{
  getData();

  if (isBufferdata(data_input))
  {
    if (compareData(data_input, password)) // Dung pass
    {
      lcd.clear();
      clear_data_input();
      index_t = 3;
    }
    else if (compareData(data_input, mode_changePass))
    {
      // Serial.print("mode_changePass");
      lcd.clear();
      clear_data_input();
      index_t = 1;
    }
    else if (compareData(data_input, mode_resetPass))
    {
      // Serial.print("mode_resetPass");
      lcd.clear();
      clear_data_input();
      index_t = 2;
    }
    else if (compareData(data_input, mode_hardReset))
    {
      lcd.setCursor(0, 0);
      lcd.print("---HardReset---");
      writeEpprom(pass_def);
      insertData(password, pass_def);
      clear_data_input();
      delay(2000);
      lcd.clear();
      index_t = 0;
    }
    else if (compareData(data_input, mode_addRFID))
    {
      lcd.clear();
      clear_data_input();
      index_t = 8;
    }
    else if (compareData(data_input, mode_delRFID))
    {
      lcd.clear();
      clear_data_input();
      index_t = 9;
    }
    else if (compareData(data_input, mode_delAllRFID))
    {
      lcd.clear();
      clear_data_input();
      index_t = 10;
    }
    else
    {
      if (error_pass == 2)
      {
        clear_data_input();
        lcd.clear();
        index_t = 4;
      }
      Serial.print("Error");
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("WRONG PASSWORD");
      clear_data_input();
      error_pass++;
      delay(1000);
      lcd.clear();
    }
  }
}

void openDoor()
{
  // Serial.println("Open The Door");
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("---OPENDOOR---");
  unsigned char pos;
  delay(1000);
  sg90.write(180);
  delay(5000);
  sg90.write(0);
  lcd.clear();
  index_t = 0;
}

void error()
{
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("WRONG 3 TIME");
  delay(2000);
  lcd.setCursor(1, 1);
  lcd.print("Wait 1 minutes");
  unsigned char minute = 0;
  unsigned char i = 30;
  char buff[3];
  while (i > 0)
  {
    if (i == 1 && minute > 0)
    {
      minute--;
      i = 59;
    }
    if (i == 1 && minute == 0)
    {
      break;
    }
    sprintf(buff, "%.2d", i);
    i--;
    delay(200);
  }
  lcd.clear();
  index_t = 0;
}

void changePass() // Thay doi pass
{
  lcd.setCursor(0, 0);
  lcd.print("-- Change Pass --");
  delay(3000);
  lcd.setCursor(0, 0);
  lcd.print("--- New Pass ---");
  while (1)
  {
    getData();
    if (isBufferdata(data_input))
    {
      insertData(new_pass1, data_input);
      // Serial.println(new_pass1);
      clear_data_input();
      break;
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("---- AGAIN ----");
  while (1)
  {
    getData();

    if (isBufferdata(data_input))
    {
      insertData(new_pass2, data_input);
      // Serial.println(new_pass2);
      clear_data_input();
      break;
    }
  }
  delay(1000);
  if (compareData(new_pass1, new_pass2))
  {
    lcd.clear();
    // Serial.println("Success");
    lcd.setCursor(0, 0);
    lcd.print("--- Success ---");
    delay(1000);
    writeEpprom(new_pass2);
    insertData(password, new_pass2);
    if (Firebase.setString(firebaseData, "/PASSWORD", String(password)))
    {
      Serial.println("PASSWORD: " + String(password));
    }
    lcd.clear();
    index_t = 0;
  }
  else
  {
    lcd.clear();
    // Serial.println("miss");
    lcd.setCursor(0, 0);
    lcd.print("-- Mismatched --");
    delay(1000);
    lcd.clear();
    index_t = 0;
  }
}

void resetPass()
{
  unsigned char choise = 0;
  // Serial.println("Pass reset");
  lcd.setCursor(0, 0);
  lcd.print("---Reset Pass---");
  getData();
  if (isBufferdata(data_input))
  {
    if (compareData(data_input, password))
    {
      lcd.clear();
      clear_data_input();
      while (1)
      {
        lcd.setCursor(0, 0);
        lcd.print("---Reset Pass---");
        char key = keypad.getKey();
        if (choise == 0)
        {
          lcd.setCursor(0, 1);
          lcd.print(">");
          lcd.setCursor(2, 1);
          lcd.print("YES");
          lcd.setCursor(9, 1);
          lcd.print(" ");
          lcd.setCursor(11, 1);
          lcd.print("NO");
        }
        if (choise == 1)
        {
          lcd.setCursor(0, 1);
          lcd.print(" ");
          lcd.setCursor(2, 1);
          lcd.print("YES");
          lcd.setCursor(9, 1);
          lcd.print(">");
          lcd.setCursor(11, 1);
          lcd.print("NO");
        }
        if (key == '*')
        {
          if (choise == 1)
          {
            choise = 0;
          }
          else
          {
            choise++;
          }
        }
        if (key == '#' && choise == 0)
        {
          lcd.clear();
          delay(1000);
          writeEpprom(pass_def);
          insertData(password, pass_def);
          if (Firebase.setString(firebaseData, "/PASSWORD", String(password)))
          {
            Serial.println("PASSWORD: " + String(password));
          }
          lcd.setCursor(0, 0);
          lcd.print("---Reset ok---");
          delay(1000);
          lcd.clear();
          break;
        }
        if (key == '#' && choise == 1)
        {
          lcd.clear();
          break;
        }
      }
      index_t = 0;
    }
    else
    {
      index_t = 0;
      lcd.clear();
    }
  }
}
unsigned char numberInput()
{
  char number[5];
  char count_i = 0;
  while (count_i < 2)
  {
    char key = keypad.getKey();
    if (key && key != 'A' && key != 'B' && key != 'C' && key != 'D' && key != '*' && key != '#')
    {
      delay(100);
      lcd.setCursor(10 + count_i, 1);
      lcd.print(key);
      number[count_i] = key;
      count_i++;
    }
  }
  return (number[0] - '0') * 10 + (number[1] - '0');
}

bool isAllowedRFIDTag(byte tag[])
{
  int count = 0;
  for (int i = 10; i < 512; i += 4)
  {
    Serial.print("EEPROM: ");
    for (int j = 0; j < 4; j++)
    {
      Serial.print(EEPROM.read(i + j), HEX);
      if (tag[j] == EEPROM.read(i + j))
      {
        count++;
      }
    }
    Serial.println();
    if (count == 4)
    {
      return true;
    }
    count = 0;
  }
  return false;
}

void rfidCheck()
{
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
    byte rfidTag[4];
    Serial.print("RFID TAG: ");
    for (byte i = 0; i < rfid.uid.size; i++)
    {
      rfidTag[i] = rfid.uid.uidByte[i];
      Serial.print(rfidTag[i], HEX);
    }
    Serial.println();

    if (isAllowedRFIDTag(rfidTag))
    {
      lcd.clear();
      index_t = 3;
    }
    else
    {
      if (error_pass == 2)
      {
        lcd.clear();
        index_t = 4;
      }
      Serial.print("Error\n");
      lcd.clear();
      lcd.setCursor(3, 1);
      lcd.print("WRONG RFID");
      error_pass++;
      delay(1000);
      lcd.clear();
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

void addRFID()
{
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("ADD NEW RFID");
  switch (MODE_RFID)
  {
  case (MODE_ID_RFID_ADD):
  {
    Serial.print("ADD_IN");
    lcd.setCursor(0, 1);
    lcd.print("Input Id: ");
    id_rf = numberInput();
    Serial.println(id_rf);
    if (id_rf == 0)
    { // ID #0 not allowed, try again!
      lcd.clear();
      lcd.setCursor(3, 1);
      lcd.print("ID ERROR");
    }
    else
    {
      MODE_RFID = MODE_ID_RFID_FIRST;
    }
    delay(2000);
  }
  break;
  case (MODE_ID_RFID_FIRST):
  {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("   Put RFID    ");
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
    {
      byte rfidTag[4];
      Serial.print("RFID TAG: ");
      for (byte i = 0; i < rfid.uid.size; i++)
      {
        rfidTag[i] = rfid.uid.uidByte[i];
        Serial.print(rfidTag[i], HEX);
      }
      Serial.println();

      if (isAllowedRFIDTag(rfidTag))
      {
        lcd.clear();
        lcd.setCursor(1, 1);
        lcd.print("RFID ADDED BF");
        index_t = 0;
        delay(2000);
        lcd.clear();
        MODE_RFID = MODE_ID_RFID_ADD;
      }
      else
      {
        MODE_RFID = MODE_ID_RFID_SECOND;
      }
      FirebaseJson json;
      json.set("id", id_rf);
      String rfidString = String(rfidTag[0], HEX) + String(rfidTag[1], HEX) + String(rfidTag[2], HEX) + String(rfidTag[3], HEX);

      String path = "/RFIDTags/" + String(id_rf);
      if (Firebase.set(firebaseData, path.c_str(), json))
      {
        Serial.println("Thẻ RFID đã được lưu thành công!");
      }
      else
      {
        Serial.print("Lỗi: ");
        Serial.println(firebaseData.errorReason());
      }
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }
  break;
  case MODE_ID_RFID_SECOND:
  {
    lcd.setCursor(0, 1);
    lcd.print("   Put Again    ");
    delay(1000);
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
    {
      byte rfidTag[4];
      Serial.print("RFID TAG: ");
      for (byte i = 0; i < rfid.uid.size; i++)
      {
        rfidTag[i] = rfid.uid.uidByte[i];
        Serial.print(rfidTag[i], HEX);
      }
      for (int i = 0; i < 4; i++)
      {
        EEPROM.write(10 + (id_rf - 1) * 4 + i, rfidTag[i]);
        EEPROM.commit();
        Serial.println(EEPROM.read(10 + (id_rf - 1) * 4 + i), HEX);
      }
      Serial.print("OK");
      lcd.setCursor(0, 1);
      lcd.print("Add RFID Done");
      delay(2000);
      index_t = 0;
      Serial.print("ADD_OUT");
      lcd.clear();
      MODE_RFID = MODE_ID_RFID_ADD;
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  break;
  }
}

void delRFID()
{
  char buffDisp[20];
  lcd.setCursor(1, 0);
  lcd.print("  DELETE RFID   ");
  Serial.print("DEL_IN");
  lcd.setCursor(0, 1);
  lcd.print("Input ID: ");
  id_rf = numberInput();
  if (id_rf == 0)
  { // ID #0 not allowed, try again!
    lcd.clear();
    lcd.setCursor(3, 1);
    lcd.print("ID ERROR");
    delay(2000);
  }
  else
  {
    for (int i = 0; i < 4; i++)
    {
      EEPROM.write(10 + (id_rf - 1) * 4 + i, '\0');
      EEPROM.commit();
      Serial.println(EEPROM.read(10 + (id_rf - 1) * 4 + i), HEX);
    }
    sprintf(buffDisp, "Clear id:%d Done", id_rf);
    lcd.setCursor(0, 1);
    lcd.print(buffDisp);
    Serial.print("DEL_OUT");
    delay(2000);
    lcd.clear();
    index_t = 0;
  }
}

void delAllRFID()
{
  char key = keypad.getKey();
  lcd.setCursor(0, 0);
  lcd.print("CLEAR ALL RFID?");
  if (key == '*')
  {
    isMode = 0;
  }
  if (key == '#')
  {
    isMode = 1;
  }
  if (isMode == 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("> Yes      No  ");
  }
  if (isMode == 1)
  {
    lcd.setCursor(0, 1);
    lcd.print("  Yes    > No  ");
  }
  if (key == '0' && isMode == 0)
  {
    for (int i = 10; i < 512; i++)
    {
      EEPROM.write(i, '\0');
      EEPROM.commit();
      Serial.println(EEPROM.read(i), HEX);
    }
    lcd.setCursor(0, 1);
    lcd.print("  Clear done  ");
    delay(2000);
    index_t = 0;
    lcd.clear();
  }
  if (key == '0' && isMode == 1)
  {
    lcd.clear();
    index_t = 0;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, passwordwifi);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }
  
  initFirebase(); // Kết nối Firebase
  
  lcd.init();
  lcd.backlight();
  sg90.attach(PIN_SG90, 500, 2400);
  EEPROM.begin(512); // Initialize EEPROM with 512 bytes
  SPI.begin();
  rfid.PCD_Init();
  readEpprom(); // Đọc mật khẩu từ EEPROM

  // Nếu mật khẩu chứa ký tự lạ hoặc chưa được thiết lập, ghi lại mật khẩu mặc định
  if (password[0] == 0xFF) {
    writeEpprom(pass_def);
    insertData(password, pass_def);
    Serial.print("PASSWORD: ");
    Serial.println(password);
    if (Firebase.getString(firebaseData, "/PASSWORD") && firebaseData.dataType() == "string") {
      String passwordString = firebaseData.stringData();
      passwordString.toCharArray(password, sizeof(password) - 1);
      password[sizeof(password) - 1] = '\0'; // Đảm bảo chuỗi kết thúc bằng ký tự null
    }
  }
}

void loop() {
  lcd.setCursor(1, 0);
  lcd.print("Enter Password");
  // Đọc mật khẩu từ Firebase nếu có thay đổi
  if (Firebase.readStream(firebaseData)) {
    if (firebaseData.dataType() == "string") {
      String newPassword = firebaseData.stringData();
      newPassword.toCharArray(password, sizeof(password) - 1);
      password[sizeof(password) - 1] = '\0'; // Đảm bảo chuỗi kết thúc bằng ký tự null

      Serial.print("Updated Password: ");
      Serial.println(password);
    } else {
      Serial.print("Error: ");
      Serial.println(firebaseData.errorReason());
    }
  }
  
  checkPass(); // Kiểm tra mật khẩu nhập vào
  rfidCheck(); // Kiểm tra RFID
  
  // Các trạng thái khác của hệ thống
  while (index_t == 1) {
    changePass();
  }

  while (index_t == 2) {
    resetPass();
  }

  while (index_t == 3) {
    openDoor();
    error_pass = 0;
  }

  while (index_t == 4) {
    error();
    error_pass = 0;
  }

  while (index_t == 8) {
    addRFID();
  }

  while (index_t == 9) {
    delRFID();
  }

  while (index_t == 10) {
    delAllRFID();
  }
}
