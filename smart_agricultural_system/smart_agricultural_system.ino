/*****************
**    Kết nối   **
*****************/

// DHT       | Arduino
//----------------
// VCC(1)    |  5V
// DATA(2)   |  2
// NC(3)     |  --
// GND(4)    |  GND
// ----------------

//Cảm biến độ ẩm | Arduino
//------------------------
// VCC(1)      |  5V
// GND(2)      |  GND
// D0(3)       |  --
// A0(4)       |  A1
//------------------------

// Cảm biến ánh sáng nối chân A0
// Relay điều khiến máy bơm nối chân 3
// LED 13 báo hiệu chế độ tự động có được chạy
// chân A3 điều khiển hệ thống đèn
// chân A2 điều khiển hệ thống sưởi ấm


//--độ ẩm đất
// Ẩm ướt : trên 60%
// Ổn định: 30% – 60%
// Khô: Dưới 30% (cần phải tưới nước)

//--Nhiệt độ
// Lạnh: Dưới 15oC
// Trung bình: 20oC – 25oC
// Nóng: 25oC trở lên
//
//--Ánh sáng
// Tối: dưới 40% (không bơm nước tưới)
// Sáng: trên 40%


#include <DHT.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F,16,2);

SoftwareSerial Serial1(10, 11); // RX, TX

#define IP "184.106.153.149"      // thingspeak.com ip

#define DHTPIN 2                  // Chân DATA cua DHT21 nối với chân 2
#define LDR_PIN A0                // Chân A0 nối với chân OUT cảm biến as
#define SOIL_MOIST_1_PIN A1       // Chân A1 nối với cảm biến độ ẩm
#define PUMPW_PIN 3               // chân 3 điều khiển relay máy bơm
#define BUTTON_PUMP 9             // Nút điều khiển máy bơm
#define BUTTON_MODE 8             // Nút điều khiển mode
#define Mode_Led 13               // báo hiệu chế độ Auto
#define Lighter A2                // điều khiển đèn chiếu sáng
#define Warmer A3                 // điều khiển hệ thống sưởi
#define RAIN_PIN 6                // cảm biến mưa  
#define IN1 5
#define IN2 4
#define LED_BUTTON 12             // báo hiệu có thể dùng button

int humDHT;                       // độ ẩm
int tempDHT;                      // nhiệt độ
int lumen;                        // ánh sáng
int soilMoist;                    // độ ẩm đất

int HOT_TEMP = 35;                // nhiệt độ định mức trên
int COLD_TEMP = 15;               // nhiệt độ định mức dưới
int DARK_LIGHT = 40;              // Độ sáng định mức
int DRY_SOIL = 30;                // độ ẩm định mức dưới 
int WET_SOIL = 60;                // độ ẩm định mức trên

int pumpWaterStatus = 0;          // trạng thái máy bơm
int lightStatus = 0;              // trạng thái chiếu sáng
int warmStatus = 0;               // trạng thái sưởi
int autoControlMode = 1;          // trạng thái chế độ tự động
int roofStatus = 0;               // trạng thái mái che
int rainStatus = 0;               // trạng thái mưa
int mode = 1;                 
bool checkFirstPress = true;   

unsigned long sampleTimingSeconds = 30000;       // Thời gian đọc cảm biến (30s)
unsigned long startTiming = 0;                             
unsigned long timePumpOn = 10;                   // thời gian bơm nước
unsigned long startTimingShowLCD = 0;        

char msg[] = "GET /update?key=RXDOI9H7JJ2D6P9F";    // Write Key API của thingspeak
char cmd[100];
char aux_str[100];
int legth;

// Khởi tạo cảm biến DHT21
DHT dht(DHTPIN, DHT21);

void setup() {
  
  Serial.begin(115200);
  Serial1.begin(115200);
  
  pinMode(PUMPW_PIN, OUTPUT);
  pinMode(Mode_Led, OUTPUT);
  pinMode(Lighter, OUTPUT);
  pinMode(Warmer, OUTPUT);
  pinMode(RAIN_PIN,INPUT); 
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(LED_BUTTON, OUTPUT);
  
  dht.begin();
  
  lcd.init();                               // Khởi động màn hình LCD
  lcd.backlight();                          // Bật đèn nền
  lcd.begin(16, 2);
  lcd.print("  DO AN LTVDK");
  lcd.setCursor(0, 1);
  lcd.print("-HT NONG NGHIEP-");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DANG KHOI DONG");
  lcd.setCursor(0, 1);
  lcd.print("Vui long cho ...");
  connectWiFi();
  lcd.setCursor(0, 0);
  lcd.print("Da ket noi wifi ");
  delay(1000);
  
  digitalWrite(Mode_Led, HIGH);             // bật đèn báo đang ở chế độ tự động
  readToSensors();                          // Khởi tạo đọc cảm biến
  printData();                              // in dữ liệu ra Serial 
  showDataLCD();                            // hiển thị dữ liệu ra LCD
  digitalWrite(LED_BUTTON, HIGH);           // bật đèn báo có thể dùng button
  startTiming = millis();                   // Bắt đầu đếm thời gian cho việc đọc dữ liệu từ cảm biến
  
}

void loop() {
    
    readToButton(); 
    if ((unsigned long)(millis() - startTiming) > sampleTimingSeconds) {
      mode = 1;
      digitalWrite(Mode_Led, autoControlMode);              // chuyển về mode1 và bật đèn báo Mode1
      checkFirstPress = true;
      digitalWrite(PUMPW_PIN, LOW);
      pumpWaterStatus = 0;
      Serial.println("Ket thuc thoi gian nhan Button");
      digitalWrite(LED_BUTTON, LOW);                        // tắt đèn báo k thể dùng button
      readToSensors();                                      // đọc dữ liệu
      printData();                                          // in dữ liệu ra Serial 
      showDataLCD();                                        // hiển thị dữ liệu ra LCD
      if(autoControlMode) {
        autoControlPlantation();                            // chế độ tự động bơm nước
      }
      updateDataThingSpeak();                               // update dữ liệu lên thingspeak
      startTiming = millis();                               // cập nhập lại startTime
      Serial.println("Co the dung button");
      digitalWrite(LED_BUTTON, HIGH);                       // bật đèn báo có thể dùng button
    }
}


/******************************************
**     Xử lý đọc và hiển thi dữ liệu     **
******************************************/

//==== Hàm lấy giá trị cường độ ánh sáng ===
int getLumen(int anaPin) {
  int anaValue = 0;
  for (int i = 0; i < 30; i++)                              // Đọc giá trị cảm biến 30 lần và lấy giá trị trung bình
  {
    anaValue += analogRead(anaPin);
    delay(50);
  }

  anaValue = anaValue / 30;
  anaValue = map(anaValue, 0, 1023, 0, 100);               // Tối: 0%  ==> Sáng 100%
  return anaValue;
}

//==== Hàm lấy giá trị độ ẩm đất ====
int getSoilMoist() {
  int anaValue = 0;
  for (int i = 0; i < 30; i++)
  {
    anaValue += analogRead(SOIL_MOIST_1_PIN);              // Đọc giá trị cảm biến độ ẩm đất
    delay(50);   // Đợi đọc giá trị ADC
  }
  anaValue = anaValue / 30;
  anaValue = map(anaValue, 1023, 0, 0, 100);              // Ít nước:  0%  ==> Nhiều nước 100%
  return anaValue;
}

//=== Hàm lấy dữ liệu từ các cảm biến ===
void readToSensors() {
  tempDHT = dht.readTemperature();                        // Đọc nhiệt độ DHT21
  humDHT = dht.readHumidity();                            // Đọc độ ẩm DHT21
  lumen = getLumen(LDR_PIN);                              // Đọc ánh sáng
  soilMoist = getSoilMoist();                             // Đọc cảm biến độ ẩm đất
  if(digitalRead(RAIN_PIN))                               // Kiểm tra có mưa hay không
    rainStatus = 0;
  else 
    rainStatus = 1;
}

//=== Hàm in dữ liệu ra Serial ===
void printData() {
  
  // IN thông tin thu được ra Serial
  Serial.print("Do am: ");
  Serial.print(humDHT);
  Serial.print(" %\t");
  Serial.print("Nhiet do: ");
  Serial.print(tempDHT);
  Serial.print(" *C\t");
  Serial.print("Anh sang: ");
  Serial.print(lumen);
  Serial.print(" %\t");
  Serial.print("Do am dat: ");
  Serial.print(soilMoist);
  Serial.println(" %");
}

//=== Hàm hiểu thị ra LCD ===
void showDataLCD() {
  lcd.clear();           
  lcd.setCursor(0, 0);
  lcd.print("DO AM    = ");
  lcd.print(humDHT);
  lcd.println("%   " );

  lcd.setCursor(0, 1);
  lcd.print("NHIET DO = ");
  lcd.print(tempDHT);
  lcd.println("oC  ");
  
  delay(5000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DO AM DAT = ");
  lcd.print(soilMoist);
  lcd.println("%  " );

  lcd.setCursor(0, 1);
  lcd.print("ANH SANG  = ");
  lcd.print(lumen);
  lcd.println("%  ");

  delay(5000);
}


/****************************************************
**     Điều khiển các thiết bị dựa vào dữ liệu     **
****************************************************/

//=== Hàm điều khiển máy bơm ===
void controlPumper()
{
  if (pumpWaterStatus == 1)
    digitalWrite(PUMPW_PIN, HIGH);
  if (pumpWaterStatus == 0) 
    digitalWrite(PUMPW_PIN, LOW);
}

//=== Hàm điều khiển hệ thống chiếu sáng ===
void controlLighter()
{
  if (lightStatus == 1) digitalWrite(Lighter, HIGH);
  else if (lightStatus == 0) digitalWrite(Lighter, LOW);
}

//=== Hàm điều khiển hệ thống sưởi ấm ===
void controlWarmer()
{
  if (warmStatus == 1) digitalWrite(Warmer, HIGH);
  else if (warmStatus == 0) digitalWrite(Warmer, LOW);
}

//=== Hàm điều khiển mái che ===
void controlRoof()
{
  if (roofStatus == 1){
    rotateOut(255);
    delay(1000);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    return;
    }
  else if (roofStatus == 0){
    rotateIn(255);
    delay(1000);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    return;
  }
}

//=== Hàm quay mái che ra ===
void rotateOut(int speed) {    
  digitalWrite(IN1, HIGH);
  analogWrite(IN2, 255 - speed);
}

//=== Hàm thu mái che vào ===
void rotateIn(int speed) {
  digitalWrite(IN1, LOW);
  analogWrite(IN2, speed);
}

//=== Hàm bật máy bơm trong chế độ tự động ===
void turnPumpOn() {
  digitalWrite(PUMPW_PIN, LOW);
  pumpWaterStatus = 1;
  updateControllerThingSpeak();
  lcd.clear();      
  lcd.setCursor(0, 0);
  lcd.print(" DANG BOM NUOC       ");
  lcd.setCursor(0, 1);
  lcd.print("    TU DONG        ");
  digitalWrite(PUMPW_PIN, HIGH);
  delay (timePumpOn * 1000);
  digitalWrite(PUMPW_PIN, LOW);
  pumpWaterStatus = 0;
  updateControllerThingSpeak();
  showDataLCD();
}

//=== Hàm điều khiển tự động ===
void autoControlPlantation() {
 
  // nếu trời tối thì bật hê thống đèn chiếu sáng
  if (lumen < 15)
  {
    lightStatus = 1;
    Serial.println("Light on");
    controlLighter();
  } else {
    Serial.println("Light off");
    lightStatus = 0;
    controlLighter();
  }

  // nếu nhiệt độ < 15oC thì bật hệ thống sưởi ấm
  if (tempDHT < COLD_TEMP)
  {
    Serial.println("Warm on");
    warmStatus = 1;
    controlWarmer();
  } else {
    Serial.println("Warm off");
    warmStatus = 0;
    controlWarmer();
  }

  // nếu trời mưa và mái che đang tắt thì bật mái che
  if ((rainStatus || (tempDHT >= HOT_TEMP && lumen >= 85)) && !roofStatus){
    Serial.println("Roof on");
    roofStatus = 1;
    controlRoof();
  }

  // nếu trời ko mưa, trời không nóng và mái che đang bật thì tắt mái che
  if(!rainStatus && (tempDHT < HOT_TEMP || lumen < 85) && roofStatus){
    Serial.println("Roof off");
    roofStatus = 0;
    controlRoof();
  }
  
  //  Nếu khô và trời sáng -> bơm nước (ánh sáng > 40% và độ ẩm đất < 30%)
  //  Nếu khô và trời tối -> không bơm nước
  if (soilMoist < DRY_SOIL && lumen > DARK_LIGHT)
  {
    turnPumpOn();
  }
}

/******************************************
**       Chế độ điều khiển bằng tay      **
******************************************/

//=== Hàm kiểm tra trạng thái nút bấm ===
boolean checkButton(int pin)
{
  boolean state;
  boolean previousState;
  const int checkButtonDelay = 60;

  previousState = digitalRead(pin);
  for (int counter = 0; counter < checkButtonDelay; counter++)
  {
    delay(1);
    state = digitalRead(pin);
    if (state != previousState)
    {
      counter = 0;
      previousState = state;
    }
  }
  return state;
}

//=== Hàm đọc và xử lý nút bấm ===
void readToButton()
{
  int digiValue = checkButton(BUTTON_PUMP);
  
  if (!digiValue && !autoControlMode)
  {
    checkFirstPress = false;
    if(mode == 5)
      mode = 1;
    else
      mode++;
    switch(mode){
      case 1:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MODE 1.");
        lcd.setCursor(0, 1);
        lcd.print("CD TU DONG: ");
        lcd.print(autoControlMode);
        digitalWrite(Mode_Led, autoControlMode);
        break;
      case 2:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MODE 2.        ");
        lcd.setCursor(0, 1);
        lcd.print("MAY BOM: ");
        lcd.print(pumpWaterStatus);
        digitalWrite(Mode_Led, pumpWaterStatus);
        break;
      case 3:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MODE 3.");
        lcd.setCursor(0, 1);
        lcd.print("CHIEU SANG: ");
        lcd.print(lightStatus);
        digitalWrite(Mode_Led, lightStatus);
        break;
      case 4:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MODE 4.");
        lcd.setCursor(0, 1);
        lcd.print("SUOI AM: ");
        lcd.print(warmStatus);
        digitalWrite(Mode_Led, warmStatus);
        break;
      case 5:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MODE 5.");
        lcd.setCursor(0, 1);
        lcd.print("MAI CHE: ");
        lcd.print(roofStatus);
        digitalWrite(Mode_Led, roofStatus);
        break;
    }
    startTiming = millis();                       // sau mỗi lần bấm nút thì reset startTiming ở mức 15s(nghĩa là sau 15s nữa sẽ tiến hành các tác vụ đọc và update dữ liệu)
  } else if (!digiValue){
      checkFirstPress = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MODE 1.");
      lcd.setCursor(0, 1);
      lcd.print("CD TU DONG: ");
      lcd.print(autoControlMode);
      digitalWrite(Mode_Led, autoControlMode);
      startTiming = millis();                     // sau mỗi lần bấm nút thì reset startTiming ở mức 15s(nghĩa là sau 15s nữa sẽ tiến hành các tác vụ đọc và update dữ liệu)
  }
  
  digiValue = checkButton(BUTTON_MODE);
  if (!digiValue)
  {
    if(checkFirstPress){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MODE 1.");
        lcd.setCursor(0, 1);
        lcd.print("CD TU DONG: ");
        lcd.print(autoControlMode);
        digitalWrite(Mode_Led, autoControlMode);
        checkFirstPress = false;
        return;
    }
    switch(mode){
      case 1:
        autoControlMode = !autoControlMode;     
        digitalWrite(Mode_Led, autoControlMode);
        lcd.setCursor(0, 1);
        lcd.print("CD TU DONG: ");
        lcd.print(autoControlMode);
        break;
      case 2: 
        pumpWaterStatus = !pumpWaterStatus;
        digitalWrite(Mode_Led, pumpWaterStatus);
        controlPumper();
        lcd.setCursor(0, 1);
        lcd.print("MAY BOM: ");
        lcd.print(pumpWaterStatus);
        break;
      case 3: 
        lightStatus = !lightStatus;
        digitalWrite(Mode_Led, lightStatus);
        controlLighter();
        lcd.setCursor(0, 1);
        lcd.print("CHIEU SANG: ");
        lcd.print(lightStatus);
        break;
      case 4: 
        warmStatus = !warmStatus;
        digitalWrite(Mode_Led, warmStatus);
        controlWarmer();
        lcd.setCursor(0, 1);
        lcd.print("SUOI AM: ");
        lcd.print(warmStatus);
        break;
      case 5: 
        roofStatus = !roofStatus;
        digitalWrite(Mode_Led, roofStatus);
        controlRoof();
        lcd.setCursor(0, 1);
        lcd.print("MAI CHE: ");
        lcd.print(roofStatus);
        break;
    }
    startTiming = millis();
  }
}


/************************************************
**     Xử lý update dữ liệu lên thingspeak     **
************************************************/

//=== Hàm gửi lệnh AT 1 ===
int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout) {

  uint8_t x = 0,  answer = 0;
  char response[100];
  unsigned long previous;
  memset(response, '\0', 100);    // xóa buffer
  delay(100);
  while ( Serial1.available() > 0) Serial1.read();   // đọc input
  Serial1.println(ATcommand);    // Gửi lệnh AT
  x = 0;
  previous = millis();

  // Chờ phản hồi
  do {
    if (Serial1.available() != 0) {
      // Nếu có dữ liệu trong buffer UART, đọc và kiểm tra nó với expected_answer
      response[x] = Serial1.read();
      x++;
      // Nếu đúng thì trả kết quả answer = 1, thoát hàm
      if (strstr(response, expected_answer) != NULL)
      {
        answer = 1;
      }
    }
  } while ((answer == 0) && ((millis() - previous) < timeout));       // Nếu sai thì tiếp tục thử lại cho tới hết thời gian timeout
  Serial.println(response);                                           // In giá trị nhận được để debug
  return answer;
}

//=== Hàm gửi lệnh AT 2 ===
int8_t sendATcommand2(char* ATcommand, char* expected_answer1,
                      char* expected_answer2, unsigned int timeout) {

  uint8_t x = 0,  answer = 0;
  char response[100];
  unsigned long previous;
  memset(response, '\0', 100);    // Khởi tạo lại chuỗi về 0
  delay(100);
  while ( Serial1.available() > 0) Serial1.read();   // Xóa buffer
  Serial1.println(ATcommand);    // Gửi lệnh AT
  x = 0;
  previous = millis();
  // Chờ phản hồi
  do {
    // Nếu có dữ liệu từ UART thì đọc và kiểm tra
    if (Serial1.available() != 0) {
      response[x] = Serial1.read();
      x++;
      // Trả về giá trị 1 nếu nhận được expected_answer1
      if (strstr(response, expected_answer1) != NULL)
      {
        answer = 1;
      }
      // Trả về giá trị 2 nếu nhận được expected_answer2
      else if (strstr(response, expected_answer2) != NULL)
      {
        answer = 2;
      }
    }
  }
  // Đợi time out
  while ((answer == 0) && ((millis() - previous) < timeout));
  Serial.println(response);   // In giá trị nhận được để debug
  return answer;
}

//=== Hàm kết nối Wifi ===
void connectWiFi(void)
{
  sendATcommand("AT", "OK", 5000);                                    // Kiểm tra kết nối
  sendATcommand("AT+CWMODE=1", "OK", 5000);                           // Cấu hình chế độ station
  sendATcommand("AT+CWJAP=\"king\",\"10010110\"", "OK", 5000);        // Ket noi wifi 
  sendATcommand("AT+CIPMUX=1", "OK", 5000);                           // Bật chế độ đa kết nối
  sendATcommand("AT+CIFSR", "OK", 5000);                              // Hiển thị ip
  Serial.println("ESP8266 Connected");
}

//=== Hàm kết nối với thingspeak ===
void connectThingSpeak(void)
{
  memset(aux_str, '\0', 100);
  snprintf(aux_str, sizeof(aux_str), "AT+CIPSTART=1,\"TCP\",\"%s\",80", IP);
  if (sendATcommand2(aux_str, "OK",  "ERROR", 20000) == 1)
  {
    Serial.println("OK Connected Thingspeak");
  }
}

//=== Hàm gửi dữ liệu bằng lệnh AT ===
void sendThingSpeakCmd(void)
{
  memset(aux_str, '\0', 100);   // aux_str = null
  sprintf(aux_str, "AT+CIPSEND=1,%d", legth);
  if (sendATcommand2(aux_str, ">", "ERROR", 20000) == 1)
  {
    Serial.println(cmd);
    sendATcommand2(cmd, "SEND OK", "ERROR", 30000);
  }
}

//=== Hàm gửi toàn bộ dữ liệu lên thingspeak ===
void updateDataThingSpeak(void)
{
  connectThingSpeak();
  sprintf(cmd, "%s&field1=%d&field2=%d&field3=%d&field4=%d&field5=%d&field6=%d&field7=%d&field8=%d", msg, tempDHT, humDHT, lumen, soilMoist, pumpWaterStatus, lightStatus, warmStatus, roofStatus);
  legth = strlen(cmd) + 2;
  sendThingSpeakCmd();
  sendATcommand("AT+CIPCLOSE=1", "OK", 5000);
}

//=== Hàm update trạng thái các hệ thống điều khiển lên thingspeak ===
void updateControllerThingSpeak(void)
  for (int i = 0; i < 1; i++)     // Thực hiện 2 lần cho chắc ăn
  {
    connectThingSpeak ();
    // Cập nhật trạng thái bơm
    sprintf(cmd, "%s&field5=%d&field6=%d&field7=%d&field8=%d", msg, pumpWaterStatus, lightStatus, warmStatus, roofStatus);
    legth = strlen(cmd) + 2;
    sendThingSpeakCmd();
    sendATcommand("AT+CIPCLOSE=1", "OK", 5000);
  }
}
