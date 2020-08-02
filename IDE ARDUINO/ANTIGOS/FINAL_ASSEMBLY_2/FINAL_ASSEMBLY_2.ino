#include <SD.h>
#include <SPI.h>
#include "RTClib.h"
#include "EmonLib.h"
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_ADS1015.h>
#include "HX711.h"                    // Biblioteca HX711 

//define programa do SD CARD
File myFile;

//Módulo RTC DS3231 ligado as portas 20 e 21 do Arduino
RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//VALOR DE CALIBRAÇÃO PARA SENSOR DE TENSÃO (DEVE SER AJUSTADO EM PARALELO COM UM MULTÍMETRO)
#define VOLT_CAL 400

//VALOR DE AMOSTRAS PARA MÉDIA MÓVEL LEITOR DE TENSÃO 12V
#define AMOSTRAS 10

//Define o pino do sinal digital do DS18B20.
#define ONE_WIRE_BUS 4

#define DOUT_1  7                      // HX711 DATA OUT = pino A0 do Arduino 
#define CLK_1  6                       // HX711 SCK IN = pino A1 do Arduino 

#define DOUT_0  12                      // HX711 DATA OUT = pino A0 do Arduino 
#define CLK_0  11                       // HX711 SCK IN = pino A1 do Arduino 

//Cria uma instância para SCT013
EnergyMonitor SCT013;

 //CRIA UMA INSTÂNCIA para ZMPT101b (substituir emon1 por zmpt)
EnergyMonitor emon1;

//Módulo ADS1115 com 16-bits
Adafruit_ADS1115 ads;

HX711 balanca_0;                        // define instancia balança HX711
HX711 balanca_1;                        // define instancia balança HX711
 

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

// Addresses of 4 DS18B20s
uint8_t sensor1[8] = { 0x28, 0xAA, 0xB8, 0x5A, 0x4D, 0x14, 0x01, 0x97 };
uint8_t sensor2[8] = { 0x28, 0xAA, 0x74, 0x9E, 0x4A, 0x14, 0x01, 0xD9 };
uint8_t sensor3[8] = { 0x28, 0xAA, 0x19, 0xF2, 0x54, 0x14, 0x01, 0x18 };
uint8_t sensor4[8] = { 0x28, 0xAA, 0x27, 0x04, 0x4D, 0x14, 0x01, 0x82 };

//Variáveis do sensor DS18B20
int deviceCount = 0;
float tempC;
float b1temp;
float b2temp;
float b3temp;
float b4temp;

//Variáveis do LED RGB da placa principal
const int RED_PIN = 24;
const int GREEN_PIN = 23;
const int BLUE_PIN = 22;

//Variáveis relacionadas com o sensor de ZMPT101b e SCT013
int pinSCT = A3;   //Pino analógico conectado ao SCT-013
double tensao;
double potencia;
double Irms;

//Variáveis relacionadas com o sensor de rotação indutivo
int pino_D0 = 3;
int rpm;
volatile byte pulsos;
unsigned long timeold;

//Define o número de sinais por volta
unsigned int pulsos_por_volta = 1;

//Pino CS do módulo SD CARD. Pin 53 para Mega / Pin 10 para UNO
int pinoSS = 53;

//Variáveis relacionadas a interface do serial
int contador_serial = 0;
int escolha = 0;

//Variável relacionada com o sensor de corrente SHUNT
float correnteLV;

float calibration_factor = 235533.00;     // fator de calibração aferido na Calibraçao 

float massa_combustivel;

//Variáveis relacionadas ao medidor de tensão 12V das baterias
float b1tensao;
float b2tensao;
float b3tensao;
float b4tensao;

float aRef=5.00; //tensão de referência


//FUNÇÃO LEITURA DE TENSÃO 12V
float lePorta(uint8_t portaAnalogica) {
  float total=0;  
  for (int i=0; i<AMOSTRAS; i++) {
    total += 1.0 * analogRead(portaAnalogica);
   // delay(5);
  }
  return total / (float)AMOSTRAS;
}  
 

void contador()
{
  //Incrementa contador do sensor de rotação
  pulsos++;
}

void LEDiniciar (){
  
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);
  delay(100);
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, LOW);
    delay(100);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
    delay(1000);
  
}

void LEDerrorON (){
      digitalWrite(RED_PIN, HIGH);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
}

void LEDerrorOFF (){
        digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
}

void serialTask () {
  //=================== SERIAL ===========================
  // Função que solicita o usuário qual forma de apresentação dos dados ele deseja.
  //Caso nenhuma forma de apresentação dos dados seja escolhida em até 20s, o tipo gravação será escolhido automaticamente.

  if (contador_serial < 20) {

    LEDiniciar ();

    contador_serial++;

  

    if (Serial.available()) {
      escolha = Serial.read();                       // le carcter da serial
      if (escolha == '1' || escolha == '2') {
        contador_serial = 30;
      }
      else{
        Serial.println("Escolha inválida, tente novamente.");
      }
    }

        if (Serial1.available()) {
      escolha = Serial1.read();                       // le carcter da serial
      if (escolha == '1' || escolha == '2') {
        contador_serial = 30;
      }
      else{
        Serial1.println("Escolha inválida, tente novamente.");
      }
    }
  }
    
  
  if (contador_serial == 20) {
    escolha = '1';
    contador_serial = 30;
  }
  if (escolha == '1')                  // se pressionar t ou T
  {

  }
  if (escolha == '2')                  // se pressionar t ou T
  {
    serial1(); 
    serialblue();            // imprime no monitor serial
  }
}

void serial1() {
  //COLOCAR AQUI OS SERIAIS DO MODO 1 DE APRESENTACAO DOS DADOS
//============================================ RTC SERIAL====================================

    DateTime now = rtc.now();

    Serial.print(now.day(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.year(), DEC);
    //Serial.print(" (");
    //Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(", ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.print(" || ");

//============================================ ROTAÇÃO SERIAL ====================================

    //Mostra o valor de RPM no serial monitor
    Serial.print("RPM = ");
    Serial.print(rpm, DEC);
        Serial.print(" || ");

//============================================ CORRENTE LV SERIAL ================================

  Serial.print("Amps: "); 
  Serial.print(correnteLV); 
      Serial.print(" || ");

//============================================= CORRENTE, TENSÃO E POTENCIA HV ===================

    Serial.print("Corrente = ");
    Serial.print(Irms);
    Serial.print(" A");
        Serial.print(" || ");
    
    Serial.print("Potencia = ");
    Serial.print(potencia);
    Serial.print(" W");
        Serial.print(" || ");

    Serial.print("Tensão = ");
    Serial.print(tensao);
    Serial.print(" V");
        Serial.print(" || ");

//================================================ MASSA COMBUSTIVEL =============================================

      Serial.print(" || Peso total: ");                            // imprime no monitor serial
  Serial.print(massa_combustivel, 2);              // imprime peso na balança com 3 casas decimais 
  Serial.println(" kg");  

  //=============================================== Tensão 12v ====================

  Serial.print("Tensao B1: ");
  Serial.print(b1tensao * 3.01, 2);
  Serial.print ("V");
           Serial.print(" || ");

             Serial.print("Tensao B2: ");
  Serial.print(b2tensao * 3.01, 2);
  Serial.print ("V");
           Serial.print(" || ");

             Serial.print("Tensao B3: ");
  Serial.print(b3tensao * 3.01, 2);
  Serial.print ("V");
           Serial.print(" || ");

             Serial.print("Tensao B4: ");
  Serial.print(b4tensao * 3.01, 2);
  Serial.print ("V");
           Serial.print(" || ");

  
  //delay(100);
}

void serialblue() {
  //COLOCAR AQUI OS SERIAIS DO MODO 1 DE APRESENTACAO DOS DADOS
//============================================ RTC SERIAL====================================

    DateTime now = rtc.now();

    Serial1.print(now.day(), DEC);
    Serial1.print('/');
    Serial1.print(now.month(), DEC);
    Serial1.print('/');
    Serial1.print(now.year(), DEC);
    //Serial.print(" (");
    //Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial1.print(", ");
    Serial1.print(now.hour(), DEC);
    Serial1.print(':');
    Serial1.print(now.minute(), DEC);
    Serial1.print(':');
    Serial1.print(now.second(), DEC);
    Serial1.println();

//============================================ ROTAÇÃO SERIAL ====================================

    //Mostra o valor de RPM no serial monitor
    Serial1.print("RPM = ");
    Serial1.println(rpm, DEC);
    Serial1.print("RPM = ");

//============================================ CORRENTE LV SERIAL ================================

  Serial1.print("Amps: "); 
  Serial1.println(correnteLV); 

//============================================= CORRENTE, TENSÃO E POTENCIA HV ===================

    Serial1.print("Corrente = ");
    Serial1.print(Irms);
    Serial1.println(" A");
    
    Serial1.print("Potencia = ");
    Serial1.print(potencia);
    Serial1.println(" W");

    Serial1.print("Tensão = ");
    Serial1.print(tensao);
    Serial1.println(" V");

//============================================= DS18B20 ===================

     Serial.print("Sensor 1: ");
     Serial.print(b1temp);
     Serial.print("C");
         Serial.print(" || ");

  Serial.print("Sensor 2: ");
     Serial.print(b2temp);
     Serial.print("C");
         Serial.println();

  Serial.print("Sensor 3: ");
     Serial.print(b3temp);
     Serial.print("C");
         Serial.print(" || ");

    Serial.print("Sensor 4: ");
     Serial.print(b4temp);
     Serial.print("C");
         Serial.print(" || ");

//=============================================== Tensão 12v ====================

  Serial.print("Tensao B1: ");
  Serial.print(b1tensao * 3.01, 2);
  Serial.print ("V");
           Serial.print(" || ");

             Serial.print("Tensao B2: ");
  Serial.print(b2tensao * 3.01, 2);
  Serial.print ("V");
           Serial.print(" || ");

             Serial.print("Tensao B3: ");
  Serial.print(b3tensao * 3.01, 2);
  Serial.print ("V");
           Serial.print(" || ");

             Serial.print("Tensao B4: ");
  Serial.print(b4tensao * 3.01, 2);
  Serial.print ("V");
           Serial.print(" || ");
  
  //delay(100);
}

void gravacaoTask() {
  //=================== SD CARD ===========================
  myFile = SD.open("datalog2.txt", FILE_WRITE); // Cria / Abre arquivo .txt

  if (myFile) { // Se o Arquivo abrir imprime:
        if (contador_serial == 30){

       digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, HIGH);
      digitalWrite(BLUE_PIN, LOW);

        if (escolha == '2')                  // se pressionar t ou T
  {
    Serial.println("Escrevendo no Arquivo .txt"); // Imprime na tela
  }
        }

        DateTime now = rtc.now();

    myFile.print(now.day(), DEC);
    myFile.print('/');
    myFile.print(now.month(), DEC);
    myFile.print('/');
    myFile.print(now.year(), DEC);
    myFile.print(", ");
    myFile.print(now.hour(), DEC);
    myFile.print(':');
    myFile.print(now.minute(), DEC);
    myFile.print(':');
    myFile.print(now.second(), DEC);
    myFile.print(","); // Escreve no Arquivo
    
    myFile.print("Rotação"); // Escreve no Arquivo
    myFile.print(","); // Escreve no Arquivo
    myFile.print(rpm); // Escreve no Arquivo
    myFile.println(","); // Escreve no Arquivo

                            digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
    
    myFile.close(); // Fecha o Arquivo após escrever
            if (contador_serial == 30){

                if (escolha == '2')                  // se pressionar t ou T
  {

    Serial.println("Terminado."); // Imprime na tela
    Serial.println(" ");

  }

            digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, HIGH);
      digitalWrite(BLUE_PIN, LOW);
    
            }
  }

  else {// Se o Arquivo não abrir
            if (contador_serial == 30){
              
              LEDerrorON ();

                if (escolha == '2')                  // se pressionar t ou T
  {

    Serial.println("Erro ao Abrir Arquivo .txt"); // Imprime na tela

  }
   
    LEDerrorOFF ();

            }
  }

  /*myFile = SD.open("datalog.txt"); // Abre o Arquivo

    if (myFile) {
    Serial.println("Conteúdo do Arquivo:"); // Imprime na tela

    while (myFile.available()) { // Exibe o conteúdo do Arquivo
    Serial.write(myFile.read());
    }

    myFile.close(); // Fecha o Arquivo após ler
    }

    else {
    Serial.println("Erro ao Abrir Arquivo .txt"); // Imprime na tela
    }
  */
}

void rotacaoTask(){
  //Atualiza contador a cada segundo
  if (millis() - timeold >= 500)
  {
    //Desabilita interrupcao durante o calculo
    detachInterrupt(1);
    rpm = (60 * 1000 / pulsos_por_volta ) / (millis() - timeold) * pulsos;
    timeold = millis();
    pulsos = 0;
    //Habilita interrupcao
    attachInterrupt(1, contador, FALLING);
  }
}

void shuntTask(){
  int16_t results;
  
  results = ads.readADC_Differential_0_1();  

  
  correnteLV = ((float)results * 256.0) / 32768.0;//100mv shunt
  correnteLV = correnteLV * 1.333; //uncomment for 75mv shunt *JOGAR ESSES VALORES PARA O TASK DE CALCULO POSTERIORMENTE
  //amps = amps * 2; //uncomment for 50mv shunt

  //delay(5000);
}

void potencia220Task(){
    Irms = SCT013.calcIrms(1480);   // Calcula o valor da Corrente

    emon1.calcVI(10,100); //FUNÇÃO DE CÁLCULO (20 SEMICICLOS, TEMPO LIMITE PARA FAZER A MEDIÇÃO)    
  
    tensao   = emon1.Vrms; //VARIÁVEL RECEBE O VALOR DE TENSÃO RMS OBTIDO
    
    potencia = Irms * tensao;          // Calcula o valor da Potencia Instantanea    

}

void temperaturabateriaTask(){
  
    sensors.requestTemperatures();

    b1temp = sensors.getTempC(sensor1);
    b2temp = sensors.getTempC(sensor2);
    b3temp = sensors.getTempC(sensor3);
    b4temp = sensors.getTempC(sensor4);
}

void tensaobateriaTASK(){
    b1tensao = (lePorta(A8) * aRef) / 1024.0;
        b2tensao = (lePorta(A9) * aRef) / 1024.0;
            b3tensao = (lePorta(A10) * aRef) / 1024.0;
                b4tensao = (lePorta(A11) * aRef) / 1024.0;
}

 
void celulacargaTASK()
{

              if (contador_serial == 30){
  //Serial.print("Peso 0: ");                            // imprime no monitor serial
  //Serial.print(balanca_0.get_units(), 2);              // imprime peso na balança com 3 casas decimais 
  //Serial.print(" kg");                             // imprime no monitor serial 
  //delay(500) ;                                       // atraso de 0,5 segundos 

  //  Serial.print(" || Peso 1: ");                            // imprime no monitor serial
  //Serial.print(balanca_1.get_units(), 2);              // imprime peso na balança com 3 casas decimais 
  //Serial.print(" kg");                             // imprime no monitor serial 
  //delay(500) ;                                       // atraso de 0,5 segundos 


  massa_combustivel = balanca_0.get_units() + balanca_1.get_units();   
}
}
 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(9600);

  // Declara pinoSS do SD CARD como saída
  pinMode(pinoSS, OUTPUT);

  // Declara o pino do sensor indutivo como entrada
  pinMode(pino_D0, INPUT);
  //Interrupcao 0 - pino digital 2
  //Aciona o contador a cada pulso
  attachInterrupt(1, contador, FALLING);
  pulsos = 0;
  rpm = 0;
  timeold = 0;

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Define o ganho do ADS1115 / 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  ads.setGain(GAIN_SIXTEEN);
  // Inicia o módulo.
  ads.begin();

      sensors.setResolution(sensor1, 9);
      sensors.setResolution(sensor2, 9);
        sensors.setResolution(sensor3, 9);
          sensors.setResolution(sensor4, 9);

    emon1.voltage(A2, VOLT_CAL, 1.7); //PASSA PARA A FUNÇÃO OS PARÂMETROS (PINO ANALÓGIO / VALOR DE CALIBRAÇÃO / MUDANÇA DE FASE)
    SCT013.current(pinSCT, 52); // 2000 espiras / resistor (valor stock de 6.0606)

        // Initialize library with data output pin, clock input pin and gain factor.
  // Channel selection is made by passing the appropriate gain:
  // - With a gain factor of 64 or 128, channel A is selected
  // - With a gain factor of 32, channel B is selected
  // By omitting the gain factor parameter, the library
  // default "128" (Channel A) is used here.
  
  balanca_0.begin(DOUT_0, CLK_0, 128);                          // inicializa a balança
  balanca_1.begin(DOUT_1, CLK_1, 128);                          // inicializa a balança

    Serial.println("Iniciando o sistema..."); // Imprime na tela

      delay(3000); // wait for console opening *DS3231*



    if (! rtc.begin()) {
            digitalWrite(RED_PIN, HIGH);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
    Serial.println("Couldn't find RTC");
          digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

    DateTime now = rtc.now();

    Serial.print(now.day(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.year(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    // Inicializa o SD Card
  if (rtc.begin()) {
    Serial.println("RTC pronto para uso."); // Imprime na tela
  }

  // Inicializa o SD Card
  if (SD.begin()) {
    Serial.println("SD Card pronto para uso."); // Imprime na tela
  }

  else {
    Serial.println("Falha na inicialização do SD Card.");
    return;
  }
  
  balanca_0.set_scale(calibration_factor);             // ajusta fator de calibração
  balanca_0.tare();                                    // zera a Balança
  balanca_1.set_scale(calibration_factor);             // ajusta fator de calibração
  balanca_1.tare();                                    // zera a Balança  

    Serial.println("Caso deseje visualizar os dados adquiridos via Serial, digite '1' a qualquer momento."); // Imprime na tela


    Serial1.println("Caso deseje visualizar os dados adquiridos via Serial, digite '1' a qualquer momento."); // Imprime na tela
}

void loop() {

  serialTask();
  gravacaoTask();
  rotacaoTask();
  shuntTask();
  potencia220Task();
  temperaturabateriaTask();
  celulacargaTASK();
  tensaobateriaTASK();

}
