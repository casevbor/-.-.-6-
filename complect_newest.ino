#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Servo.h>
 
//DEFINE-s
#define adc_1 "Attach"
#define adc_2 "Card"

#define gates_rotations 3.3     // Кол-во оборотов сервака (по 360) для открытия ворот (float)
#define door_degrees 120      // Кол-во градусов для открытия двери

#define HC_trigPin A2         // Триггер пин для HC
#define HC_echoPin 4          // ECHO пин для HC

#define QUI_low_limit_HC 600    // Нижний предел расстояния покоя для HC
#define QUI_high_limit_HC 11000   // Верхний предел расстояния покоя для HC
int is_ride = 0;

//СЕРВОПРИВОДЫ
Servo gates;
Servo door;

// ДИСПЛЕЙ
LiquidCrystal_I2C lcd(0x27,16,2);

// RFID считыватель
#define RST_PIN 9
#define SS_PIN 10

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

uint32_t previousUid = 0;

String response = "";

// KEY MATRIX
const byte ROWS = 4; // 4 строки
const byte COLS = 3; // 3 столбца
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {0, 1, 2, 3};
byte colPins[COLS] = {5, 6, 7};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String currentCode = "";
String CODE = "1097";
char _slh = '#';
int is_open_gates = 0;

void setup() {
  is_open_gates = 0;
  // серваки
  gates.attach(A0);
  door.attach(A1);

  door.write(0);
  gates.write(90);
  
  //датчик растояния
  pinMode(HC_trigPin, OUTPUT);    // TRIGGER
  pinMode(HC_echoPin, INPUT);      // ECHO

  //дисплей
  lcd.init();                      // Инициализация дисплея  
  lcd.backlight();
  lcd.setCursor(0, 0);
  SPI.begin();

  //RFID
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);
  rfid.PCD_AntennaOff();
  rfid.PCD_AntennaOn();
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //остальное
  Serial.begin(9600);
  while (!Serial) {}
  addCardPrint();
}

void open_door() {
  for (int deg = 0; deg < door_degrees; deg++) {
    door.write(deg);
    delay(10);
  }
  //door.write(90);
  delay(2000);

  for (int deg = door_degrees; deg > 0; deg-=1) {
    door.write(deg);
    delay(10);
  }
  //door.write(0);
}

void close_gates() {
  gates.write(105);
  for (int i=0; i<265; i++) {
    delay(20);
    //lcd.clear();
    //lcd.write(get_dist());
    int rx = get_dist();
    if (QUI_low_limit_HC >= rx) {
      gates.write(90);
      rx = get_dist();
      while (QUI_low_limit_HC >= rx) {
        rx = get_dist();
        delay(20);
        //gates.write(90);
      }
      delay(100);
      gates.write(105);
      continue;
    }
  }
  //delay(6800);
  delay(300);
  gates.write(90);
}

void open_gates() {
  gates.write(80);
  delay(7000); //1835
  gates.write(90);
}

void addCardPrint() {
  lcd.clear();
  lcd.print(adc_1);
  lcd.setCursor(0, 1);
  lcd.print(adc_2);
  lcd.setCursor(0, 0);
}
// >>>

int get_dist(){
      int durationx;

      digitalWrite(HC_trigPin, LOW); // изначально датчик не посылает сигнал
      delayMicroseconds(2); // ставим задержку в 2 ммикросекунд

      digitalWrite(HC_trigPin, HIGH); // посылаем сигнал
      delayMicroseconds(10); // ставим задержку в 10 микросекунд
      digitalWrite(HC_trigPin, LOW); // выключаем сигнал

      durationx = pulseIn(HC_echoPin, HIGH);
      return durationx;
}

void loop() {

  static uint32_t rebootTimer = millis(); // Важный костыль против зависания модуля!
if (millis() - rebootTimer >= 1000) {   // Таймер с периодом 1000 мс
  rebootTimer = millis();               // Обновляем таймер
  digitalWrite(RST_PIN, HIGH);          // Сбрасываем модуль
  delayMicroseconds(2);                 // Ждем 2 мкс
  digitalWrite(RST_PIN, LOW);           // Отпускаем сброс
  rfid.PCD_Init();                      // Инициализируем заного
}
    // Обработка клавиатуры
    char key = keypad.getKey();
    if (key){
      if (key=='*') {
        currentCode = "";
         addCardPrint();

      } else {
          if (key==_slh) {
            if (currentCode==CODE) {
          lcd.clear();
          lcd.print("Welcome");
          open_door();
          addCardPrint();
          currentCode = "";

        } else {
          if (currentCode=="0000") {
            Serial.print("Rewrite: UID =");
            Serial.println(" ");

            String response = "";
            response = Serial.readStringUntil('\n');
            
            if (response == "TRewrite"){
              lcd.clear();
              lcd.print("Card updated");
              delay(1500);
              addCardPrint();
            }

          } else {
          currentCode="";
          lcd.clear();
          lcd.print("Incorrect");
          lcd.setCursor(0, 1);
          lcd.print("Code");
          lcd.setCursor(0, 0);
          delay(1000);
          addCardPrint();
        }

      }} else {
        currentCode += key;
        lcd.clear();
        lcd.print("Code: " + currentCode);
        }
      }
    }

    //Датчик HC
    


      int duration = get_dist();
      //lcd.clear();
      //lcd.print(duration);

      if (duration < QUI_low_limit_HC) {
        is_ride = 1;
        //lcd.clear();
        //lcd.print(duration);
        //delay(500);
      } else {

      if ((QUI_low_limit_HC <= duration) && is_open_gates && is_ride) {
        delay(2000);
        is_ride = 0;
        close_gates();
        is_open_gates = 0;
      } }
    

    // Принятие UID метки, отправка в Python скрипт и получение результата.
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        uint32_t currentUid = 0;
        for (uint8_t i = 0; i < 4; i++) {
            currentUid <<= 8;
            currentUid |= rfid.uid.uidByte[i];}

        if (currentUid != previousUid) {
            String uidString = "";
            for (uint8_t i = 0; i < 4; i++) {uidString += String(rfid.uid.uidByte[i]);}
            previousUid = currentUid;

            Serial.print("Запрос: UID = ");
            Serial.println(uidString);

            String response = "";
            response = Serial.readStringUntil('\n');

            lcd.clear();
            if (response=="True" && !is_open_gates) {
              is_open_gates = 1;
              lcd.print("Welcome!");
              open_gates();
              delay(2000);
              addCardPrint();
            } else {
              lcd.print("Access denied");
              delay(1000);
              addCardPrint();
              is_open_gates = 0;
            }
        }
    }
}