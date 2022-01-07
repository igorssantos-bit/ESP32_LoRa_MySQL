/*******************************************************************************
   fw_master

   Codigo usado para envio dos dados de leitura de sensores de modulos escravos
   para um modulo mestre (master) usando a tecnologia LoRa ponto-a-ponto (P2P).

   O Hardware utilizado tanto para os modulos escravos quanto para o mestre e
   o IoT DevKit LoRaWAN. Especificacoes:
      Processador principal ESP-32 (WiFi & Bluetooth)
      Módulo LoRaWAN Bee V2 (Transceptor LoRa: SMW-SX1276M0)
      Sensores:   Temperatura e Humidade (DHT11)
                  Acelerometro (MMA8452Q)
                  LDR

   Escrito por Igor Santos (06/01/2022)
 ******************************************************************************/
#if !defined(ARDUINO_ESP32_DEV)
#error Use this example with the ESP32
#endif

/* Includes ------------------------------------------------------------------*/
#include "src/RoboCore_SMW_SX1276M0.h"
#include "HardwareSerial.h"
#include "ArduinoJson.h"

/* Defines -------------------------------------------------------------------*/
#define LORA_RXD2_PIN   16
#define LORA_TXD2_PIN   17

/* Variables -----------------------------------------------------------------*/
// Declaracao do GPIO
const int statusLED = 13;
const int sendLED = 2;

// Criamos as instancias que serao usadas no decorrer do codigo
HardwareSerial LoRaSerial(2);       // Pinos de Comunicacao LoRa
SMW_SX1276M0 lorawan(LoRaSerial);   // Objeto lora da biblioteca SMW_SX1276
CommandResponse resposta;

// Declaracao das variaveis que armazenam as informacoes da rede
const char DADDR[] = "00000001";
const char P2PDA[] = "00000002";
const char APPSKEY[] = "0000000000000000000000000000000c";
const char NWSKEY[] = "0000000000000000000000000000000b";

/* Prototypes  ----------------------------------------------------------------*/
void lora_network_init(void);
void event_handler(Event);

/* Functions  ----------------------------------------------------------------*/

/*!
   Funcao de inicializacao do kit de desenvolvimento
   @param None
   @return None
*/
void setup() {
   // Inicializacao Serial
   Serial.begin(9600);
   Serial.println(F("Lora_Master_v1.0"));

   // Inicializacao GPIO
   pinMode(statusLED, OUTPUT);
   pinMode(sendLED, OUTPUT);

   // Inicializacao Lora
   lorawan.setPinReset(5);                       // Define pino
   lorawan.reset();                              // Realiza um reset no modulo
   LoRaSerial.begin(115200, SERIAL_8N1, LORA_RXD2_PIN, LORA_TXD2_PIN);
   lorawan.event_listener = &event_handler;      // Declaracao do handler
   lora_network_init();
}

/*!
   Funcao loop que permite o programa realizar acoes sequenciais
   @param None
   @return None
*/
void loop() {
   // Habilita eventos vindos do modulo lora
   lorawan.listen();
}

/*!
   Funcao loop que permite o programa realizar acoes sequenciais
   @param None
   @return None
*/
void lora_network_init(void) {
   //Requisita e imprime o Device EUI do modulo
   char deveui[16];
   resposta = lorawan.get_DevEUI(deveui);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("DevEUI: "));
      Serial.write((uint8_t *)deveui, 16);
      Serial.println();
   } else {
      Serial.println(F("Erro ao obter o Device EUI"));
   }

   // set the Device Address
   resposta = lorawan.set_DevAddr(DADDR);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("Device Address set: "));
      Serial.write((uint8_t *)DADDR, 8);
      Serial.println();
   } else {
      Serial.println(F("Error setting the Device Address"));
   }

   // set P2P Device Address
   resposta = lorawan.set_P2PDevAddr(P2PDA);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("P2P Device Address set "));
      Serial.write((uint8_t *)P2PDA, 8);
      Serial.println();
   } else {
      Serial.println(F("Error setting the Device Address"));
   }

   //Configura o Application Session Key no modulo
   resposta = lorawan.set_AppSKey(APPSKEY);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("Application Session Key configurado "));
      Serial.write((uint8_t *)APPSKEY, 32);
      Serial.println();
   } else {
      Serial.println(F("Erro ao configurar o Application Session Key"));
   }

   //Configura o Network Session Key no modulo
   resposta = lorawan.set_NwkSKey(NWSKEY);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("Network Session Key configurado "));
      Serial.write((uint8_t *)NWSKEY, 32);
      Serial.println();
   } else {
      Serial.println(F("Erro ao configurar o Network Session Key"));
   }

   //Condfigura o metodo de conexao como P2P
   resposta = lorawan.set_JoinMode(SMW_SX1276M0_JOIN_MODE_P2P);
   if (resposta == CommandResponse::OK) {
      Serial.println(F("Metodo de Conexao Configurado como P2P"));
   } else {
      Serial.println(F("Erro ao configurar o metodo P2P"));
   }
}

/*!
   Funcao que trata os eventos do lora
   @param type Tipo de evento
   @return None
*/
void event_handler(Event type) {
   switch (type) {
      case (Event::JOINED):{
         Serial.println(F("Conectado!"));
      }
      break;
      
      case (Event::RECEIVED_X):{
         Serial.println(F("Mensagem Hexadecimal Recebida!"));

         //Declaracao da variavel "porta"
         uint8_t porta;

         //Declaracao da variavel "leitura_buffer"
         uint8_t leitura_buffer;

         //Declaracao do objeto "buffer"
         Buffer buffer;

         //Atrela a variavel "resposta" o que foi retornado pelo módulo
         resposta = lorawan.readX(porta, buffer);
         if (resposta == CommandResponse::OK) { //Se retornar "<OK>"
            Serial.print(F("Mensagem: "));
            for (uint8_t i = 0 ; i < buffer.available() ; i++) {
               leitura_buffer = buffer[i];
               Serial.write(leitura_buffer);
            }
            if (leitura_buffer == '0') {
               Serial.print(F(" (zero)"));
               int estado_LED = !digitalRead(statusLED);
               digitalWrite(statusLED, estado_LED);
            }
            Serial.print(F(" na porta "));
            Serial.println(porta);
         }
      }
      break;
      
      default:{}
      break;
   }
}
