#include <Arduino.h>
#include "WEMOS_Motor.h"
#include "Oneshot125.h"
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <user_interface.h>
#include <Wire.h>

#define TimeOut 500
#define WIFI_CHANNEL 1

typedef struct struct_message
{
  int POWER = 0;
  int TURN = 0;
  uint8_t DESTRUCTION = 0;
  bool SAFE = false;
} struct_message;

struct_message myData;
Motor M1(0x30, _MOTOR_A, 1000); //Motor Direito  
Motor M2(0x30, _MOTOR_B, 1000); //Motor Esquerdo 
Oneshot motorArma;


double setTime = TimeOut;
unsigned long timer = millis();
uint8_t mac[] = {0x36, 0x36, 0x36, 0x36, 0x36, 0x36};

void Stap() //fail safe 1
{
  M1.setmotor(_SHORT_BRAKE);
  ets_delay_us(5);
  M2.setmotor(_SHORT_BRAKE);
  ets_delay_us(5);
  motorArma.writeMicroseconds(125);
  ets_delay_us(5);
}

void Set_Motor(int Speed, int rot) //Locomoção 
{
  Speed = map(Speed, 0, 255, 100, -100); //frente e trás
  rot = map(rot, 0, 255, 100, -100); //esquerda e direita
  if (Speed >= 3)// movimento pra frente
  {

    uint8_t speed1 = 0;
    uint8_t speed2 = 0;
    if ((Speed + rot) < 0)//contém os valores em 100 e 0
      speed1 = 0;
    else if ((Speed + rot) > 100)
      speed1 = 100;
    else
      speed1 = Speed + rot;

    if ((Speed - rot) < 0)
      speed2 = 0;
    else if ((Speed - rot) > 100)
      speed2 = 100;
    else
      speed2 = Speed - rot;

    speed1 = constrain(speed1, 0, 40);//redundância na contenção do valor do motor
    speed2 = constrain(speed2, 0, 40);
    M1.setmotor(2, speed1);//passa valores de potencia (1 CCW e 2 CW)
    ets_delay_us(50);
    M2.setmotor(1, speed2);
    ets_delay_us(50);
  }
  else if (Speed <= -3)//movimentação trás
  {
    Speed = -Speed;
    uint8_t speed1 = 0;
    uint8_t speed2 = 0;
    if ((Speed + rot) < 0)
      speed1 = 0;
    else if ((Speed + rot) > 100)
      speed1 = 100;
    else
      speed1 = Speed + rot;

    if ((Speed - rot) < 0)
      speed2 = 0;
    else if ((Speed - rot) > 100)
      speed2 = 100;
    else
      speed2 = Speed - rot;

    speed1 = constrain(speed1, 0, 40);
    speed2 = constrain(speed2, 0, 40);
    M1.setmotor(1, speed1);
    ets_delay_us(50);
    M2.setmotor(2, speed2);
    ets_delay_us(50);
  }
  else
  {
    if (rot >= 3)//rotação no próprio eixo
    {
      uint8_t speed1 = rot;
      uint8_t speed2 = rot;
      speed1 = constrain(speed1, 0, 40);
      speed2 = constrain(speed2, 0, 40);
      M1.setmotor(1, speed1);
      ets_delay_us(50);
      M2.setmotor(1, speed2);
      ets_delay_us(50);
    }
    else if (rot <= -3)
    {
      uint8_t speed1 = -rot;
      uint8_t speed2 = -rot;
      speed1 = constrain(speed1, 0, 40);
      speed2 = constrain(speed2, 0, 40);
      M1.setmotor(2, speed1);
      ets_delay_us(50);
      M2.setmotor(2, speed2);
      ets_delay_us(50);
    }
    else//senão joystick no meio ficar parado
    {

      M1.setmotor(1, 0);
      ets_delay_us(50);
      M2.setmotor(1, 0);
      ets_delay_us(50);
    }
  }
}

void Recv(uint8_t *mac, uint8_t *incomingData, uint8_t len)//call back de recepção do esp now
{
  memcpy(&myData, incomingData, sizeof(myData));//transforma incomingData nos dados da struct
  setTime = millis() + TimeOut; //adia a ativação do fail safe em timeout milisegundos 
  if(myData.SAFE)//fail safe do controle (aperte X para ligar)
  {
    Set_Motor(myData.POWER, myData.TURN);//passa o valor do joystick
    motorArma.writeMicroseconds(myData.DESTRUCTION);//passa o valor da arma
  }
  else {
    Stap(); 
  }
  ets_delay_us(50);
}
void setup()
{
  Serial.begin(9600); //inicia serial
  WiFi.mode(WIFI_STA); //liga o wifi
  wifi_set_macaddr(STATION_IF, mac); //set do mac do carrinho
  Wire.begin(D2, D1); //ativa o i2c
  esp_now_init(); //ativa o esp now 
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);// seta o modo de operação
  

  motorArma.attach(D6); //atribui um pino para o motor da arma
  delay(10);
  motorArma.writeMicroseconds(125);//ativa o motor
  delay(10);
  esp_now_register_recv_cb(Recv);//seta o call back do esp now
}

void loop()
{
  timer = millis();
  if (timer / setTime >= 1)//se o timer > que o valor ele para
  {
    Stap(); //fail safe
  }
}