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

#define DOUT_0  25                      // HX711 DATA OUT = pino A0 do Arduino 
#define CLK_0  26                       // HX711 SCK IN = pino A1 do Arduino 

//Variáveis do LED RGB da placa principal
#define RED_PIN 24
#define GREEN_PIN 23
#define BLUE_PIN 22

//Variáveis relacionadas com o sensor de rotação indutivo
#define pino_D0 3

//Buzzer!

#define NOTE_G4  392
#define NOTE_C5  523

#define REST      0

//Variáveis Buzzer

// change this to make the song slower or faster
int tempo = 85;

// change this to whichever pin you want to use
int buzzer = 8;

int melody[] = {NOTE_C5,8, NOTE_G4,8};

//Mais funções do Buzzer!

int notes = sizeof(melody) / sizeof(melody[0]) / 2;

// this calculates the duration of a whole note in ms
int wholenote = (60000 * 4) / tempo;

int divider = 0, noteDuration = 0;


//Pino analógico conectado ao SCT-013
int pinSCT = A3;

//Cria uma instância para SCT013
EnergyMonitor SCT013;

//CRIA UMA INSTÂNCIA para ZMPT101b (substituir ZMPT por zmpt)
EnergyMonitor ZMPT;

//Módulo ADS1115 com 16-bits
Adafruit_ADS1115 ads;

HX711 balanca_0;                        // define instancia balança HX711
HX711 balanca_1;                        // define instancia balança HX711


// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

// Addresses of 4 DS18B20s
uint8_t sensorX0[8] = { 0x28, 0xAA, 0xB8, 0x5A, 0x4D, 0x14, 0x01, 0x97 };
uint8_t sensorX8[8] = { 0x28, 0xAA, 0x74, 0x9E, 0x4A, 0x14, 0x01, 0xD9 };
uint8_t sensorX4[8] = { 0x28, 0xAA, 0x19, 0xF2, 0x54, 0x14, 0x01, 0x18 };
uint8_t sensorX2[8] = { 0x28, 0xAA, 0x27, 0x04, 0x4D, 0x14, 0x01, 0x82 };

//Variáveis do sensor DS18B20
int deviceCount = 0;
float tempC;
float b1temp;
float b2temp;
float b3temp;
float b4temp;

//Variáveis relacionadas com o sensor de ZMPT101b e SCT013
double tensao250v;
double potencia250v;
double corrente250v;

//Variáveis relacionadas com o sensor de rotação indutivo
int rpm;
int veloc;
volatile byte pulsos;
unsigned long timeold;

//Define o número de sinais por volta
unsigned int pulsos_por_volta = 4;

//Pino CS do módulo SD CARD. Pin 53 para Mega / Pin 10 para UNO
int pinoSS = 53;

//Variáveis relacionadas a interface do serial
bool escolha0 = false;

bool escolha1 = false;

//Variável relacionada com o sensor de corrente SHUNT
float correnteLV;

float calibration_factor = 235533.00;     // fator de calibração aferido na Calibraçao

float massa_combustivel;

//Variáveis relacionadas ao medidor de tensão 12V das baterias
float b1tensao;
float b2tensao;
float b3tensao;
float b4tensao;
float btotaltensao;

float aRef = 5.00; //tensão de referência


// constants won't change. They're used here to set pin numbers:
const int buttonPin = 10;  // the number of the pushbutton pin

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status

bool startState = false; 


//FUNÇÃO LEITURA DE TENSÃO 12V
float lePorta(uint8_t portaAnalogica) {
  float total = 0;
  for (int i = 0; i < AMOSTRAS; i++) {
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

void LEDiniciar () {

  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);
  delay(100);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, HIGH);
  delay(100);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  delay(100);

}

void LEDerrorON () {
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
}

void LEDerrorOFF () {
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
}

void serial0() {

    if (buttonState == HIGH) {
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
  Serial.print("Veloc = ");
  Serial.print(veloc, DEC);
  Serial.print(" || ");

  //============================================ CORRENTE LV SERIAL ================================

  Serial.print("Corrente 48V = ");
  Serial.print(correnteLV);
  Serial.print(" || ");

    //=============================================== Tensão 12v ====================

  Serial.print("Tensao B1: ");
  Serial.print(b1tensao, 2);
  Serial.print ("V");
  Serial.print(" || ");

  Serial.print("Tensao B2: ");
  Serial.print(b2tensao, 2);
  Serial.print ("V");
  Serial.print(" || ");

  Serial.print("Tensao B3: ");
  Serial.print(b3tensao, 2);
  Serial.print ("V");
  Serial.print(" || ");

  Serial.print("Tensao B4: ");
  Serial.print(b4tensao, 2);
  Serial.print ("V");
  Serial.print(" || ");

    //============================================= DS18B20 ===================

  Serial.print("Sensor 1: ");
  Serial.print(b1temp);
  Serial.print("C");
  Serial.print(" || ");

  Serial.print("Sensor 2: ");
  Serial.print(b2temp);
  Serial.print("C");
  Serial.println(" || ");

  Serial.print("Sensor 3: ");
  Serial.print(b3temp);
  Serial.print("C");
  Serial.print(" || ");

  Serial.print("Sensor 4: ");
  Serial.print(b4temp);
  Serial.print("C");
  Serial.print(" || ");


  //============================================= CORRENTE, TENSÃO E POTENCIA HV ===================

  Serial.print("Corrente 220V = ");
  Serial.print(corrente250v);
  Serial.print(" A");
  Serial.print(" || ");

  Serial.print("Potencia 220V = ");
  Serial.print(potencia250v);
  Serial.print(" W");
  Serial.print(" || ");

  Serial.print("Tensão 220V = ");
  Serial.print(tensao250v);
  Serial.print(" V");
  Serial.print(" || ");

  //================================================ MASSA COMBUSTIVEL =============================================

  Serial.print("Massa total: ");                            // imprime no monitor serial
  Serial.print(massa_combustivel, 2);              // imprime peso na balança com 2 casas decimais
  Serial.println(" kg");

  //delay(100);
}
}

void serial1() {

    if (buttonState == HIGH) {
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
  Serial1.print(" || ");

  //============================================ ROTAÇÃO SERIAL ====================================

  //Mostra o valor de RPM no serial monitor
  Serial1.print("Veloc = ");
  Serial1.print(veloc, DEC);
  Serial1.print(" || ");

  //============================================ CORRENTE LV SERIAL ================================

  Serial1.print("Corrente 48V = ");
  Serial1.print(correnteLV);
  Serial1.print(" || ");

    //=============================================== Tensão 12v ====================

  Serial1.print("Tensao B1: ");
  Serial1.print(b1tensao, 2);
  Serial1.print ("V");
  Serial1.print(" || ");

  Serial1.print("Tensao B2: ");
  Serial1.print(b2tensao, 2);
  Serial1.print ("V");
  Serial1.print(" || ");

  Serial1.print("Tensao B3: ");
  Serial1.print(b3tensao, 2);
  Serial1.print ("V");
  Serial1.print(" || ");

  Serial1.print("Tensao B4: ");
  Serial1.print(b4tensao, 2);
  Serial1.print ("V");
  Serial1.print(" || ");

    //============================================= DS18B20 ===================

  Serial1.print("Sensor 1: ");
  Serial1.print(b1temp);
  Serial1.print("C");
  Serial1.print(" || ");

  Serial1.print("Sensor 2: ");
  Serial1.print(b2temp);
  Serial1.print("C");
  Serial1.print(" || ");

  Serial1.print("Sensor 3: ");
  Serial1.print(b3temp);
  Serial1.print("C");
  Serial1.print(" || ");

  Serial1.print("Sensor 4: ");
  Serial1.print(b4temp);
  Serial1.print("C");
  Serial1.print(" || ");


  //============================================= CORRENTE, TENSÃO E POTENCIA HV ===================

  Serial1.print("Corrente 220V = ");
  Serial1.print(corrente250v);
  Serial1.print(" A");
  Serial1.print(" || ");

  Serial1.print("Potencia 220V = ");
  Serial1.print(potencia250v);
  Serial1.print(" W");
  Serial1.print(" || ");

  Serial1.print("Tensão 220V = ");
  Serial1.print(tensao250v);
  Serial1.print(" V");
  Serial1.print(" || ");

  //================================================ MASSA COMBUSTIVEL =============================================

  Serial1.print("Massa total: ");                            // imprime no monitor serial
  Serial1.print(massa_combustivel, 2);              // imprime peso na balança com 3 casas decimais
  Serial1.println(" kg");

  //delay(100);
}
}

void serialTASK () {

    if (buttonState == HIGH) {
  //=================== SERIAL ===========================
  // Função que solicita o usuário qual forma de apresentação dos dados ele deseja.
  //


  if (Serial1.available()) {
    if (Serial1.read() == '2') {                   // se pressionar 2
      escolha1 = true;
    }
    if (Serial1.read() == '1') {                  // se pressionar 1
      escolha1 = false;
    }
  }
  
  else if (Serial.available()) {
    if (Serial.read() == '2') {                  // se pressionar 2
      escolha0 = true;
    }
    if (Serial.read() == '1') {                  // se pressionar 1
      escolha0 = false;
    }
  }

   if (escolha1 == true)
  {
    serial1();            // Chama a função da escrita no monitor serial Bluetooth
  }
  else if (escolha0 == true)
  {
    serial0();            // Chama a função da escrita no monitor serial USB
  }

    // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);


}
}

void gravacaoTask() {

    if (buttonState == HIGH) {
  //=================== SD CARD ===========================

 if (SD.exists("datalog2.csv")) { // Se o Arquivo abrir imprime:

  
  myFile = SD.open("datalog2.csv", FILE_WRITE); // Cria / Abre arquivo .txt



    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);

    if (escolha0 == true || escolha1 == true)                  // se pressionar t ou T
    {
      Serial.println("Escrevendo no Arquivo .txt"); // Imprime na tela
    }


    DateTime now = rtc.now();

    myFile.print(now.day(), DEC);
    myFile.print('/');
    myFile.print(now.month(), DEC);
    myFile.print('/');
    myFile.print(now.year(), DEC);
    myFile.print(";");
    myFile.print(now.hour(), DEC);
    myFile.print(':');
    myFile.print(now.minute(), DEC);
    myFile.print(':');
    myFile.print(now.second(), DEC);
    myFile.print(";"); // Escreve no Arquivo

    
//========================== ROTAÇÃO ============================
    myFile.print(veloc); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
//========================== CORRENTE LV =========================
    myFile.print(correnteLV); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
//========================== TENSÃO LV ===========================
    myFile.print(b1tensao); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
    myFile.print(b2tensao); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
    myFile.print(b3tensao); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
    myFile.print(b4tensao); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
//========================== CORRENTE HV =========================
    myFile.print(corrente250v); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
//========================== TENSÃO HV =========================
    myFile.print(tensao250v); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
//========================== TEMPERATURA BATERIAS =================
    myFile.print(b1temp); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
    myFile.print(b2temp); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
    myFile.print(b3temp); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
    myFile.print(b4temp); // Escreve no Arquivo
    myFile.print(";"); // Escreve no Arquivo
//========================== MASSA COMBUSTIVEL =========================
    myFile.println(massa_combustivel); // Escreve no Arquivo
   // myFile.print(","); // Escreve no Arquivo

    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);

    myFile.close(); // Fecha o Arquivo após escrever

    if (escolha0 == true || escolha1 == true)                  // se pressionar t ou T
    {

      Serial.println("Terminado."); // Imprime na tela
      Serial.println(" ");

    }

    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);


  }

  else {// Se o Arquivo não abrir

    LEDerrorON ();

    if (escolha0 == true || escolha1 == true)                  // se pressionar t ou T
    {

      Serial.println("Erro ao Abrir Arquivo .txt"); // Imprime na tela

    }
    
    delay(1000);

    LEDerrorOFF ();


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
}

void aquisicao1TASK() {

    if (buttonState == HIGH) {

//================================== ROTAÇÃO =======================================

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

//==================================================================================
//**********************************************************************************
//================================== SHUNT =========================================

  int16_t results;

  results = ads.readADC_Differential_0_1();


  correnteLV = ((float)results * 256.0) / 32768.0;//100mv shunt
  correnteLV = correnteLV * 1.333; //uncomment for 75mv shunt *JOGAR ESSES VALORES PARA O TASK DE CALCULO POSTERIORMENTE
  //amps = amps * 2; //uncomment for 50mv shunt

//==================================================================================
//**********************************************************************************
//================================== TENSÃO 12V ====================================

  b1tensao = (lePorta(A8) * aRef) / 1024.0;
  b2tensao = (lePorta(A9) * aRef) / 1024.0;
  b3tensao = (lePorta(A10) * aRef) / 1024.0;
  b4tensao = (lePorta(A11) * aRef) / 1024.0;

//==================================================================================
//**********************************************************************************
//================================== CORRENTE 220V =================================

  corrente250v = SCT013.calcIrms(1380);   // Calcula o valor da Corrente

//==================================================================================
//**********************************************************************************
//================================== TENSÃO 220V ===================================

  ZMPT.calcVI(10, 100); //FUNÇÃO DE CÁLCULO (20 SEMICICLOS, TEMPO LIMITE PARA FAZER A MEDIÇÃO)

  tensao250v = ZMPT.Vrms; //VARIÁVEL RECEBE O VALOR DE TENSÃO RMS OBTIDO

//==================================================================================

}
}


void aquisicao2TASK() {

    if (buttonState == HIGH) {

//================================== TEMPERATURA ===================================

  sensors.requestTemperatures();

  b1temp = sensors.getTempC(sensorX0);
  b2temp = sensors.getTempC(sensorX2);
  b3temp = sensors.getTempC(sensorX4);
  b4temp = sensors.getTempC(sensorX8);

//==================================================================================
//**********************************************************************************
//================================== CÉLULA DE CARGA ===============================

  //Serial.print("Peso 0: ");                            // imprime no monitor serial
  //Serial.print(balanca_0.get_units(), 2);              // imprime peso na balança com 3 casas decimais
  //Serial.print(" kg");                             // imprime no monitor serial
  //delay(500) ;                                       // atraso de 0,5 segundos

  //  Serial.print(" || Peso 1: ");                            // imprime no monitor serial
  //Serial.print(balanca_1.get_units(), 2);              // imprime peso na balança com 3 casas decimais
  //Serial.print(" kg");                             // imprime no monitor serial
  //delay(500) ; 

  if(rpm == 0){
  massa_combustivel = balanca_0.get_units() + balanca_1.get_units();
  }

//==================================================================================
    }
}

void operacoesTASK(){

    if (buttonState == HIGH) {

  b4tensao = (b4tensao * 9.95) - (b3tensao * 7.52);  //fator de calibração para os divisores de tensão.
  b3tensao = (b3tensao * 7.52) - (b2tensao * 4.95);  //fator de calibração para os divisores de tensão.
  b2tensao = (b2tensao * 4.95) - (b1tensao * 2.985);  //fator de calibração para os divisores de tensão.
  b1tensao = (b1tensao * 2.985);            //fator de calibração para os divisores de tensão.

  btotaltensao = b1tensao + b2tensao + b3tensao + b4tensao;

    veloc = 3.1416 * ((0.4572*rpm)/30) * 3.6;

    potencia250v = corrente250v * tensao250v;          // Calcula o valor da Potencia Instantanea
  
}
}

void powerTASK(){

  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    // turn LED on:
    if(startState == true){
      
    setup();
    startState = false;
    }
    
  } else {
    // turn LED off:
    startState = true;
    LEDerrorOFF ();
    escolha1 = false;
    escolha0 = false;

        delay(1000);
        
  }
  
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(9600);


      if (buttonState == HIGH) {

  Serial.println("Iniciando o sistema..."); // Imprime na tela
  Serial1.println("Iniciando o sistema..."); // Imprime na tela

  // Declara pinoSS do SD CARD como saída
  pinMode(pinoSS, OUTPUT);


// Declara o pino da chave geral do sistema.
    pinMode(buttonPin, INPUT);


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

  LEDiniciar ();

  // Define o ganho do ADS1115 / 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  ads.setGain(GAIN_SIXTEEN);
  // Inicia o módulo.
  ads.begin();

  sensors.setResolution(sensorX0, 9);
  sensors.setResolution(sensorX2, 9);
  sensors.setResolution(sensorX4, 9);
  sensors.setResolution(sensorX8, 9);

  LEDiniciar ();

  ZMPT.voltage(A2, VOLT_CAL, 1.7); //PASSA PARA A FUNÇÃO OS PARÂMETROS (PINO ANALÓGIO / VALOR DE CALIBRAÇÃO / MUDANÇA DE FASE)
  SCT013.current(pinSCT, 54.9); // 2000 espiras / resistor (valor stock de 6.0606)

  LEDiniciar ();

  // Initialize library with data output pin, clock input pin and gain factor.
  // Channel selection is made by passing the appropriate gain:
  // - With a gain factor of 64 or 128, channel A is selected
  // - With a gain factor of 32, channel B is selected
  // By omitting the gain factor parameter, the library
  // default "128" (Channel A) is used here.

  balanca_0.begin(DOUT_0, CLK_0, 128);                          // inicializa a balança
  balanca_1.begin(DOUT_1, CLK_1, 128);                          // inicializa a balança


  LEDiniciar ();

  //delay(3000); // wait for console opening *DS3231*

  LEDiniciar ();

  balanca_0.set_scale(calibration_factor);             // ajusta fator de calibração
  balanca_0.tare();                                    // zera a Balança
  balanca_1.set_scale(calibration_factor);             // ajusta fator de calibração
  balanca_1.tare();                                    // zera a Balança

  LEDiniciar ();

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

  LEDiniciar ();

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
        Serial1.println("RTC lost power, lets set the time!");
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

  Serial1.print(now.day(), DEC);
  Serial1.print('/');
  Serial1.print(now.month(), DEC);
  Serial1.print('/');
  Serial1.print(now.year(), DEC);
  Serial1.print(" ");
  Serial1.print(now.hour(), DEC);
  Serial1.print(':');
  Serial1.print(now.minute(), DEC);
  Serial1.print(':');
  Serial1.print(now.second(), DEC);
  Serial1.println();

  LEDiniciar ();

  // Inicializa o SD Card
  if (rtc.begin()) {
    Serial.println("RTC pronto para uso."); // Imprime na tela
        Serial1.println("RTC pronto para uso."); // Imprime na tela
  }

  LEDiniciar ();

  // Inicializa o SD Card
  if (SD.begin()) {
    Serial.println("SD Card pronto para uso."); // Imprime na tela
        Serial1.println("SD Card pronto para uso."); // Imprime na tela
  }

  else {
    Serial.println("Falha na inicialização do SD Card.");
        Serial1.println("Falha na inicialização do SD Card.");
    //return;
  }

  LEDiniciar ();

  Serial.println("Caso deseje visualizar os dados adquiridos via Serial, digite '2' a qualquer momento."); // Imprime na tela


  Serial1.println("Caso deseje visualizar os dados adquiridos via Serial, digite '2' a qualquer momento."); // Imprime na tela
 
//=================================BUZZER=========================================  
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzer, melody[thisNote], noteDuration * 0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    // stop the waveform generation before the next note.
    noTone(buzzer);
  }

  //========================================================================== 

}
}
void loop() {

  aquisicao1TASK();
  aquisicao2TASK();
  operacoesTASK();
  gravacaoTask();
  serialTASK();
  powerTASK();

}
