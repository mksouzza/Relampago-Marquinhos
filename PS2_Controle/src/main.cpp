/*
PRETO = GND
VERMELHO = VCC
DATA = MARROM 21
COMANDO = ORANGE 19
SS = AMARELO 18
CLK = AZUL 05
*/

#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <PS2X_lib.h>

#define PS2_DAT 21
#define PS2_CMD 19
#define PS2_SEL 18
#define PS2_CLK 05
#define pressures false
#define rumble false
#define WIFI_CHANNEL 1
#define CHANNEL 1

typedef struct struct_message
{
  int POWER = 0;
  int TURN = 0;
  uint8_t DESTRUCTION = 0;
  bool SAFE = false;
} struct_message;
struct_message myData;

PS2X ps2x;

int error = -1;
byte type = 0;
byte vibrate = 0;
int tryNum = 1;
esp_now_peer_info_t peer;
uint8_t mac[] = {0x36, 0x36, 0x36, 0x36, 0x36, 0x33};
uint8_t peerMacAddress[] = {0x36, 0x36, 0x36, 0x36, 0x36, 0x36};

void addPeer(uint8_t *peerMacAddress) //adiciona o endereço do receptor
{
  peer.channel = CHANNEL;
  peer.encrypt = 0;
  memcpy(peer.peer_addr, peerMacAddress, 6);
  esp_now_add_peer(&peer);
}

void InitESPNow() //inicia o esp now
{
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESPNow Init Success");
  }
  else
  {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)// call back func de envio
{
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup()
{
  Serial.begin(115200);//ativa serial
  while (error != 0)
  {
    delay(1000);
    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);//ativa o controle
    tryNum++;
  }

  WiFi.mode(WIFI_STA);// ativa o wifi
  InitESPNow();//inicia o esp now
  addPeer(peerMacAddress);//adiciona endereço
  esp_now_register_send_cb(OnDataSent);//call back de envio
}

void loop()
{

  ps2x.read_gamepad(false, vibrate);//atualiza os valores e mantem o controle ligado
  myData.POWER = ps2x.Analog(PSS_RY);//frente e trás
  myData.TURN = ps2x.Analog(PSS_RX);//esquerda e direita
  if (ps2x.ButtonPressed(PSB_L1))//L1 liga a arma e L2 desliga a arma
    myData.DESTRUCTION = 150;
  else if (ps2x.ButtonPressed(PSB_L2))
    myData.DESTRUCTION = 125;

  Serial.printf("P: %d, T: %d \n", myData.POWER, myData.TURN);
  if (ps2x.ButtonPressed(PSB_CROSS))//fail safe 2 (ativa/desativa os motores)
  {
    vTaskDelay(10);
    myData.SAFE = !myData.SAFE;
  }
  esp_now_send(peerMacAddress, (uint8_t *)&myData, sizeof(myData));
  vTaskDelay(100);
}
