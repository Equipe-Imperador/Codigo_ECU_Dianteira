/* 
   EQUIPE IMPERADOR DE BAJA-SAE UTFPR
   AUTOR: Juliana Moreira e Matheus Henrique Orsini da Silva
   31/05/2021
   Codigo ECU Dianteira/Data Logger
   INPUTS: MsgCAN_ECU_Traseira e MsgCAN_ECU_Central  
   OUTPUTS: Telemetria
   Método de envio: Utilização de módulo CAN MCP2515 e Lora
*/

// Include de bibliotecas
#include "mcp2515_can.h" // Biblioteca módulo CAN
#include <SPI.h> // Biblioteca de comunicação do módulo CAN

// Módulo CAN
#define CAN_ID 0x100
#define SPI_CS 10
mcp2515_can CAN(SPI_CS); // Cria classe da CAN



void setup() 
{
  Serial.begin(115200); // LoRa611PRO
  // Definição dos pinos
  SERIAL_PORT_MONITOR.begin(115200);
  // Verifica se a Serial foi iniciada
  while(!Serial){};
  // Verifica se a CAN foi iniciada
  while (CAN_OK != CAN.begin(CAN_500KBPS)) 
  {             
      SERIAL_PORT_MONITOR.println("CAN Falhou, tentando novamente...");
      delay(100);
  }
  SERIAL_PORT_MONITOR.println("CAN Iniciada, Tudo OK!"); 
}

void loop() 
{
  unsigned char Buf[8]={0};
  if (CAN_MSGAVAIL == CAN.checkReceive()) // Se possuir mensagem na rede CAN
  {        
    CAN.readMsgBuf(8, buf); // Leio a mensagem e salvo num buffer
    // Mandando a mensagem pelo LoRa
    Serial.print("["); 
    for(int i = 0; i < 8; i++)
      Serial.print(Buf[i] + " ");
    Serial.println("]"); 
    // Escrevendo no cartão SD     
  }
}
