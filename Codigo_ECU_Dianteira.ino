/* 
   EQUIPE IMPERADOR DE BAJA-SAE UTFPR
   AUTOR: Juliana Moreira e Matheus Henrique Orsini da Silva
   31/05/2021
   Codigo ECU Dianteira/Data Logger
   INPUTS: MsgCAN_ECU_Traseira e MsgCAN_ECU_Central  
   OUTPUTS: Telemetria e Painel
   Método de envio: Utilização de módulo CAN MCP2515 e Lora
*/

// Include de bibliotecas
#include "mcp2515_can.h" // Biblioteca módulo CAN
#include <SPI.h> // Biblioteca de comunicação do módulo CAN
#include <Wire.h> // I2C com o painel
#include <SD.h>  // Biblioteca para salvar dados no SD

// Funções
void ComparaVetor(unsigned char*, unsigned char*, int); // Comparar mudanças nos dados CAN
void I2C(String, int); // Envio de I2C
void Lora(String); // Envio para telemetria
int LedRPM(int); // Calcular o numero de leds do RPM
int LedComb(int); // Calcular o numero de leds do combustível
void TransfereVetor(unsigned char*, unsigned char*, int); // Transfere dados de um vetor para outro

// Módulo CAN
#define SPI_CS 10
uint32_t CAN_ID = 0x100;
mcp2515_can CAN(SPI_CS); // Cria classe da CAN
unsigned char Buf[8]={0}; // Buffer CAN
unsigned char Traseira[8] = {0}, TraseiraPas[8] = {0}, Media[8] = {0}, MediaPas[8] = {0}; // Vetores para comparar mudança
unsigned long ID_MSG = 0; // ID da mensagem

// Módulo SD Card
#define CS_CardSD 4 // Pino de comunicação SPI
File MeuArquivo; // Cria objeto da classe File

void setup() 
{
  Wire.begin(); // Começa a I2C 
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
  if (CAN_MSGAVAIL == CAN.checkReceive()) // Se possuir mensagem na rede CAN
  {        
    CAN.readMsgBuf(8, Buf); // Leio a mensagem e salvo num buffer
    ID_MSG = CAN.getCanId(); // Pego endereço da mensagem
    
    // Transferindo vetores e comparando mudanças
    switch (ID_MSG)
    {
      case 1:
        TransfereVetor(Media, MediaPas, 8);
        TransfereVetor(Buf, Media, 8);
        ComparaVetor(Media, MediaPas, 8);
        break;
      case 10:
        TransfereVetor(Traseira, TraseiraPas, 8);
        TransfereVetor(Buf, Traseira, 8);
        ComparaVetor(Traseira, TraseiraPas, 8);
        break;
    }
    
   
    // Escrevendo no cartão SD   
    // digitalWrite(SPI_CS, HIGH) testar CAN com o SD
    if(MeuArquivo) // Se meu arquivo foi aberto
    {
      MeuArquivo.print("[");
      for(int i = 0; i < 8; i++)
        MeuArquivo.print(Buf[i]), MeuArquivo.print(";");
      MeuArquivo.print(ID_MSG, HEX), MeuArquivo.println("]"); 
    }
  }
}

/* 
    Função para comparar a mudança dos dados.
    Essa função irá comparar a mudança das mensagens da CAN enviando somente o alterado para o painel.
    Parâmetros: Mensagem atual da CAN, Mensagem passada da CAN.
    Return: VOID.
 */
void ComparaVetor(unsigned char* atual, unsigned char* passado, int len)
{
  /*
      Irei passar por todo meu vetor novo e ver se houve alguma mudança, se tiver analiso qual o ID da mensagem CAN
      e com base na indexação definida em cada ECU realizo o envio do dado para o painel no formato:
                                    CHAR + VALOR
      sendo meu char uma sigla representativa da variável e o valor o valor observado pelo sensor.
   */
  for(int i = 0; i < len; i++)
  {
    if(atual[i] != passado[i])
    {
      switch (ID_MSG)
      {
        case 1: // ECU Media
          switch(i) // Com base na indexação de cada ECU
          {
            case 2: // Temperatura
              if(atual[i] == 1)
                I2C("L",11); // Envio 11 pois significa que o LED 1 tem valor de 1(HIGH)
              else if(atual[i] == 0)
                I2C("L",10);
              break;
            case 3: // Freio
              if(atual[i] == 1)
                I2C("L",21);
              else if(atual[i] == 0)
                I2C("L",20);
              break;
            case 6: // Bateria
              if(atual[i] == 1)
                I2C("L",31);
              else if(atual[i] == 0)
                I2C("L",30);
              break;
          }
          break;
        case 10: // ECU Traseira
          switch(i)
          {
            case 0:
              I2C("V",atual[i]);  
              break;
            case 1:
              I2C("R",LedRPM(atual[1])); // Chamo as funções LedRPM e LedComb para calcular quantos LEDs devo acender
              break;
            case 6:
              I2C("C",LedComb(atual[i]));
              if(LedComb(atual[i]) <= 2)
                I2C("L",41);
              else if(LedComb(atual[i]) > 2)
                I2C("L",40);   
              break;
          }
          break;
      }
    }
  }
}

/* 
    Função para comunicação I2C com painel.
    Parâmetros: Identificação da variável, Valor.
    Return: VOID.
 */

void I2C(String ID, int Valor)
{
  String str = "", lora = "";
  str += ID, lora += ID;
  str += Valor, lora += Valor;
  str += ";;", lora += ";";
  Wire.beginTransmission(8); // Inicia transmissão com o endereço 0x8 (Painel)
  Wire.write(str.c_str());
  Wire.endTransmission();
  delay(20);
  Lora(str); //Envio do padrão para telemetria
}

/* 
    Função para calcular numero de LEDs acessos no painel.
    Todos os valores foram definidos com base numa divisão de 500RPM a cada led
    Parâmetros: Valor do RPM na casa dos milhares.
    Return: Número de LEDs para acender.
 */
int LedRPM(int Mil)
{
  if(Mil >= 60)
    return 12;
  else if(Mil >= 55)
    return 11;
  else if(Mil >= 50)
    return 10;
  else if(Mil >= 45)
    return 9;
  else if(Mil >= 40)
    return 8;
  else if(Mil >= 35)
    return 7;
  else if(Mil >= 30)
    return 6;
  else if(Mil >= 25)
    return 5;
  else if(Mil >= 20)
    return 4;
  else if(Mil >= 15)
    return 3;
  else if(Mil >= 10)
    return 2;
  else if(Mil >= 5)
    return 1;
}

/* 
    Função para calcular numero de LEDs acessos no painel.
    Todos os valores foram definidos com base numa divisão de 430mL a cada led
    Parâmetros: Valor do tanque em litros.
    Return: Número de LEDs para acender.
 */
int LedComb(int L)
{
  if(L >= 52)
    return 12;
  else if(L >= 47)
    return 11;
  else if(L >= 43)
    return 10;
  else if(L >= 39)
    return 9;
  else if(L >= 34)
    return 8;
  else if(L >= 30)
    return 7;
  else if(L >= 26)
    return 6;
  else if(L >= 21)
    return 5;
  else if(L >= 17)
    return 4;
  else if(L >= 13)
    return 3;
  else if(L >= 9)
    return 2;
  else if(L >= 4)
    return 1;
}

/* 
    Função para envio de uma string por Serial.
    Parâmetros: String.
    Return: VOID.
 */
void Lora(String str)
{
  Serial.println(str);
}

/* 
    Função para transferir dados de um vetor para outro de tamanho igual.
    Parâmetros: Vetor de origem, Vetor de saída.
    Return: VOID.
 */
void TransfereVetor(unsigned char* entrada, unsigned char* saida, int len)
{
    for(int i = 0; i < len; i++)
    {
      saida[i] = entrada[i];
    }
}
