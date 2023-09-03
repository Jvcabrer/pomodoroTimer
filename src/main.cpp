#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <cmath>
#include <EEPROM.h>
#include "apwifiesp32.h"
//#define BOTtoken "5885142699:AAH79kSUAwQyJ1aIkaehBrvOsmLQN6SppRQ" // Token del bot de Telegram DOMENICA
#define BOTtoken "5639158054:AAHOvj6mBd1XFZl4JZwrubx56F7FPoMQePo"
//#define CHAT_ID "1441825979" // ID de chat Domenica
#define CHAT_ID "1500591575" // ID del chat del Vicente
#define PRO_CPU 0
#define APP_CPU 1
#define NOAFF_CPU tskNO_AFFINITY
#define COLUMS 16 // LCD columns
#define ROWS 2
//const char *ssid = "ROBOTA"; // Datos de conexión WiFi
//const char *password = "R0b0ta2022";
String ssid_EE;
int dir_ssid_EE;
String pass_EE;
int dir_pass_EE;

int ciclosPomodoro = 1;
int ststart = 0;
int cursorMin = 5;
int cursorSeg = 8;
unsigned int time1;
unsigned int time2;
unsigned int time3;
unsigned int time4;
int timeout;

// control
int servoPin = 5; // servo
int btstart = 32;  // pulsador
//int btstop = 19;
int btstop = 26;
int stStop = 0;
int stop = 0;
int sendMessage = 0;
int buzzer = 19;

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

Servo servo;

LiquidCrystal_I2C LCD(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

void handleStop(int numNewMessages){

  for (int i = 0; i < numNewMessages; i++)
  {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID)
    {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    if (text == "/Autorizar")
    {
      stop = 1;
    }
    if (text == "/Rechazar")
    {
      stop = 0;
      stStop =0;
    }
  }
}

void lock(boolean a){
  if (a){
    servo.write(90);
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("SEGURO PUESTO");
    delay(2000);
    LCD.clear();
  }
  else{
    servo.write(0);
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print("SEGURO QUITADO");
  }
}

void sound(){
  delay(500);
  digitalWrite(buzzer, HIGH);
  delay(500);
  digitalWrite(buzzer, LOW);
}

void temp(int minute, int seg)
{
  if(stop)
    return; 
  //imprimir ciclo pomodoro
  LCD.setCursor(16,1);
  LCD.print(ciclosPomodoro);
  for (minute; minute > -1; minute--){
    if (stop){
      break;
    }
    LCD.setCursor(7, 1);
    LCD.print(":");
    if (minute < 9){
      cursorMin = 6;
    }
    LCD.setCursor(cursorMin, 1);
    LCD.print(minute);

    for (seg; seg > -1; seg--){
      time1 = millis();
      if (seg < 10)
      {
        LCD.setCursor(8, 1);
        LCD.print(0);
        cursorSeg = 9;
      }else{
        cursorSeg = 8;
      }
      LCD.setCursor(cursorSeg, 1);
      LCD.print(seg);
      if (stStop && (timeout<=4)){
        //Mensaje de autorizacion
        if (sendMessage){
          String mensaje = "Estudiante desea parar el sistema, escoja una opcion:\n";
          mensaje += "/Autorizar \n";
          mensaje += "/Rechazar \n";
          bot.sendMessage(CHAT_ID, mensaje, "");
          sendMessage = 0;
        }       
        //Espera respuestas
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        while (numNewMessages){
          handleStop(numNewMessages);
          numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        }
        if (stop){
          break;
        }
        timeout++;        
      }else{
        timeout=0;
        stStop=0;
      }
      time2 = millis();
      if ((time2 - time1) < 1000){       
        delay(1000 - (time2 - time1));        
      }
    }//seg
    seg = 59;
  } // min
  sound();
}

void IRAM_ATTR isr(){
  if(stStop == 0){
    stStop = 1;
    timeout = 0;
    sendMessage = 1;
  }
}

void setup(){
  // servo
  servo.attach(servoPin);
  servo.write(0);
  pinMode(btstart, INPUT_PULLUP);
  pinMode(btstop, INPUT_PULLDOWN);
  pinMode(buzzer, OUTPUT);
  Serial.begin(115200);
  EEPROM.begin(512);

  // LCD
  LCD.begin(COLUMS, ROWS, LCD_5x8DOTS);
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("CONFIGURANDO APP");
  // Se define la interrupcion
  attachInterrupt(digitalPinToInterrupt(btstop), isr, FALLING ); 
  //nombre de wifi a generarse y contrasena
  /*LCD.clear();
  LCD.setCursor(0,0);
  LCD.print("Buscando red");
  String ssidM     = readSTringEE(100);
  String passwordM = readSTringEE(200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidM, passwordM);
  delay(1000);  
  int contador = 0;
  while(WiFi.status() != WL_CONNECTED && contador<=3){
    delay(500);
    contador++;
  }*/
  if(WiFi.status() != WL_CONNECTED){
    LCD.clear();
    LCD.setCursor(0,0);
    LCD.print("Ingrese a la APP");
    initAP("Pomodoro","123456789");
    while (WiFi.status() != WL_CONNECTED){
      loopAP();
      Serial.print(".");
      delay(500);   
    }
  }

  // Bot de telegram
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  bot.sendMessage(CHAT_ID, "Bot iniciado", "");
  LCD.clear();
}

void loop()
{
  //Reinicio
  stStop = 0;
  stop = 0;
  //Menu principal
  LCD.setCursor(1, 0);
  LCD.print("POMODORO TIMER");
  LCD.setCursor(3, 1);
  LCD.print(" --Start--");
  // Presiona boton
  if (!digitalRead(btstart)){
    ststart = 1;
    while(!digitalRead(btstart)){}
    // solto
    delay(500); // Espera 0.5 segundos
    bot.sendMessage(CHAT_ID, "Pomodoro timer iniciado", "");
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    //Inicio de ciclos     
    lock(1);
    ciclosPomodoro = 1;
    while(ciclosPomodoro <= 4){
        if(stop)
          break;
        // TIME TO STUDY //
        LCD.setCursor(3, 0);
        LCD.print("STUDY HARD");
        temp(0, 5);
        if(stop)
          break;
        // BREAK TIME //
        LCD.clear();
        LCD.setCursor(3, 0);
        LCD.print("BREAK TIME");
        temp(0, 3);      

        ciclosPomodoro++;    
    }//while ciclos pomodoro    
    bot.sendMessage(CHAT_ID, "Pomodoro timer finalizó", "");
    lock(0);
    ststart = 0;
    LCD.clear();
  }
} // loop
