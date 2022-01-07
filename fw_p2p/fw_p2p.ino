/*******************************************************************************
   IoT DevKit - Envio e Recebimento de Mensagens pelo LoRaWAN Bee (v1.0)

   Codigo usado para enviar e receber mensagens dos IoT DevKits usando
   comunicacao P2P LoRaWAN.

   Escrito por Igor Santos (04/01/2022)
*******************************************************************************/
#if !defined(ARDUINO_ESP32_DEV) // ESP32
#error Use this example with the ESP32
#endif

/* Includes ------------------------------------------------------------------*/
#include "src/RoboCore_SMW_SX1276M0.h"
#include "HardwareSerial.h"
#include "ArduinoJson.h"

/* Defines -------------------------------------------------------------------*/
//Declaracao dos pinos de comunicacao serial do Kit
HardwareSerial LoRaSerial(2);
#define RXD2 16
#define TXD2 17

//Criacao do objeto lorawan para a biblioteca SMW_SX1276M0
SMW_SX1276M0 lorawan(LoRaSerial);

//Declaracao da variavel que armazena as respostas do modulo
CommandResponse resposta;

/* Variables ---------------------------------------------------------*/
//Declaracao das variaveis que armazenam as informacoes da rede
const char DEVADDR[] = "00000000";
const char NWSKEY[] = "00000000000000000000000000000000";
const char APPSKEY[] = "00000000000000000000000000000000";
//As variaveis acima devem ser alteradas de acordo com os codigos gerados na plataforma PROIoT

//Declaracao da variavel que armazena o "ALIAS" da variavel da PROIoT
const char ALIAS[] = "Luminosidade"; //Altere essa variavel de acordo com o ALIAS configurado na plataforma

//Declaracao das variaveis de intervalo de tempo
const unsigned long TEMPO_ESPERA = 300000; //5 minutos
unsigned long intervalo;

//Declaracao das variaveis para o LDR
const int PINO_LDR = 15;
int leitura_LDR = 0;

//Declaracao das variaveis para o LED
const int PINO_LED = 13;
int estado = LOW;

/* Functions  ----------------------------------------------------------------*/
//Declaracao da funcao que verifica a conexao do modulo e o recebimento de mensagens
void event_handler(Event);

void setup() {

   //Inicia o monitor serial e imprime o cabecalho
   Serial.begin(115200);
   Serial.println(F("Envio e Recebimento de Mensagens da Plataforma PROIoT"));

   //Definicao do pino de reset do modulo
   lorawan.setPinReset(5);
   lorawan.reset(); //Realiza um reset no modulo

   //Inicia a comunicacao serial com o modulo
   LoRaSerial.begin(115200, SERIAL_8N1, RXD2, TXD2);

   //Define o pino conectado ao sensor como uma entrada
   pinMode(PINO_LDR, INPUT);

   //Define o pino conectado ao LED como uma saida
   pinMode(PINO_LED, OUTPUT);

   //Associa a funcao que verifica a conexao do modulo ao objeto "lorawan"
   lorawan.event_listener = &event_handler;
   Serial.println(F("Evento configurado."));

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
   resposta = lorawan.set_DevAddr(DEVADDR);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("Device Address set ("));
      Serial.write((uint8_t *)DEVADDR, 8);
      Serial.println(')');
   } else {
      Serial.println(F("Error setting the Device Address"));
   }

   //Configura o Application Season Key no modulo
   resposta = lorawan.set_AppSKey(APPSKEY);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("Application Season Key configurado ("));
      Serial.write((uint8_t *)APPSKEY, 32);
      Serial.println(')');
   } else {
      Serial.println(F("Erro ao configurar o Application Season Key"));
   }

   //Configura o Network Season Key no modulo
   resposta = lorawan.set_NwkSKey(NWSKEY);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("Network Season Key configurado ("));
      Serial.write((uint8_t *)NWSKEY, 32);
      Serial.println(')');
   } else {
      Serial.println(F("Erro ao configurar o Network Season Key"));
   }

   //Condfigura o metodo de conexao como P2P
   resposta = lorawan.set_JoinMode(SMW_SX1276M0_JOIN_MODE_P2P);
   if (resposta == CommandResponse::OK) {
      Serial.println(F("Metodo de Conexao Configurado como P2P"));
   } else {
      Serial.println(F("Erro ao configurar o metodo P2P"));
   }
}

//------------------------------------------------------------------------
void loop() {

   //Recebe informacoes do modulo
   lorawan.listen();

   if (intervalo < millis()) {

      //Realiza a leitura do LDR e mapeia a leitura entre 0 e 100
      leitura_LDR = analogRead(PINO_LDR);
      leitura_LDR = map(leitura_LDR, 0, 4095, 100, 0);

      //Cria o objeto dinamico "json" com tamanho "1" para a biblioteca
      DynamicJsonDocument json(JSON_OBJECT_SIZE(1));

      //Atrela ao objeto "json" a leitura do LDR com o nome do Alias definido
      json[ALIAS] = leitura_LDR;

      //Mede o tamanho da mensagem "json" e atrela o valor somado em uma unidade ao objeto "tamanho_mensagem"
      size_t tamanho_mensagem = measureJson(json) + 1;

      //Cria a string "mensagem" de acordo com o tamanho do objeto "tamanho_mensagem"
      char mensagem[tamanho_mensagem];

      //Copia o objeto "json" para a variavel "mensagem" e com o "tamanho_mensagem"
      serializeJson(json, mensagem, tamanho_mensagem);

      //Imprime a mensagem e envia a mensagem como texto pelo modulo
      Serial.print(F("Mensagem: "));
      Serial.println(mensagem);
      resposta = lorawan.sendT(1, mensagem); //Envio como texto

      intervalo = millis() + TEMPO_ESPERA; //Atualiza a contagem de tempo

   }

}

//------------------------------------------------------------------------

void event_handler(Event type) {

   //Verifica se o modulo esta conectado e atualiza essa informacao
   if (type == Event::JOINED) {
      Serial.println(F("Conectado!"));
   }

   //Verifica se o modulo recebeu alguma mensagem
   else if (type == Event::RECEIVED_X) {
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
            estado = ! estado; //Inverte a variavel "estado"
            digitalWrite(PINO_LED, estado); //Aciona o LED de acordo com a variavel "estado
         }
         Serial.print(F(" na porta "));
         Serial.println(porta);
      }
   }

}
