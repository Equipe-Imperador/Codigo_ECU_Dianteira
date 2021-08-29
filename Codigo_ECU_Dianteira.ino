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

// Módulo SD Card
#define CS_CardSD 4 // Pino de comunicação SPI
#include <SD.h>  // Biblioteca para salvar dados no SD
File MeuArquivo; // Cria objeto da classe File


void setup() 
{
  Serial.begin(9600); // LoRa611PRO
  // Definição dos pinos
  SERIAL_PORT_MONITOR.begin(9600);
  // Verifica se a Serial foi iniciada
  while(!Serial){};
  // Verifica se a CAN foi iniciada
  while (CAN_OK != CAN.begin(CAN_500KBPS)) 
  {             
      SERIAL_PORT_MONITOR.println("CAN Falhou, tentando novamente...");
      delay(100);
  }
  SERIAL_PORT_MONITOR.println("CAN Iniciada, Tudo OK!");
   
  // Verifica o cartão SD
  SERIAL_PORT_MONITOR.print("Iniciando Cartao SD...");
  while (!SD.begin(CS_CardSD)) 
  {
    SERIAL_PORT_MONITOR.println("Cartão SD falhou!");
  }
  SERIAL_PORT_MONITOR.println("Inicialização feita.");
  MeuArquivo = SD.open("Dados.txt", FILE_WRITE); // Abro um arquivo para escrita
}

void loop() 
{
  unsigned char Buf[8]={0};
  if (CAN_MSGAVAIL == CAN.checkReceive()) // Se possuir mensagem na rede CAN
  {        
    CAN.readMsgBuf(8, Buf); // Leio a mensagem e salvo num buffer
    // Mandando a mensagem pelo LoRa
    Serial.print("["); 
    for(int i = 0; i < 8; i++)
      Serial.print(Buf[i] + " ");
    Serial.println("]"); 
    // Escrevendo no cartão SD   
    // digitalWrite(SPI_CS, HIGH) testar CAN com o SD
    if(MeuArquivo) // Se meu arquivo foi aberto
    {
      MeuArquivo.print("[");
      for(int i = 0; i < 8; i++)
        MeuArquivo.print(Buf[i] + " ");
      Serial.println("]"); 
    }
    
  }
}
