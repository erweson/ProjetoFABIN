#include <DS1307.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ultrasonic.h>
#include <SD.h>
#include <String.h>
#include "LiquidCrystal.h"

//PINOS DO SENSOR ULTRASSONICO
#define pino_trigger 4
#define pino_echo 5

//PINO DO SENSOR DE TEMPERATURA INTERNA
#define ONE_WIRE_BUS 3

//PINO DO SENSOR DE VAZÃO
#define FLOWSENSORPIN 2

//DIFERENÇA DE TEMPERATURA
#define diferencaTemp 10

//PINO DA VALVULA DO RESERVATORIO
int pino_valvula_reservatorio = 7;

//PINO DA VALVULA DA BOMBA
int pino_valvula_bomba = 8;

//PINO DA BOMBA
int pino_bomba = 9;

//PINO DO SENSOR DE TEMPERATURA AMBIENTE
const int LM35 = A0;

//**Sensor Ultrassonico**
Ultrasonic ultrasonic(pino_trigger, pino_echo); 

//**Sensor de Temperatura**
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature temperature(&oneWire); 
DeviceAddress address;

//**Modulo RTC**
DS1307 rtc(A4, A5); 

//**Cartão SD**
Sd2Card cartaoSD; 
const int pinoCartaoSD = 6; 
//Serve para printar as informações no SD pela primeira vez.
int count = 0;

//**Sensor de Fluxo**
//Conta os pulsos do sensor de fluxo
volatile uint16_t pulses = 0;
//Controla o estado do pino de pulso
volatile uint8_t lastflowpinstate;
//Registra o tempo entre os pulsos
volatile uint32_t lastflowratetimer = 0;
//Usa o lastflowratetimer para calcular o flowrate
volatile float flowrate;


 
void setup(void)
{
  Serial.begin(9600);
  Serial.print("DiaDaSemana;Data;Hora;TemperaturaAgua;Distancia;TemperaturaAmbiente;Vazao;\n"); 
  temperature.begin();
  iniciaComunicacaoSD();
  if (!temperature.getAddress(address, 0)) 
  mostra_endereco_sensor(address); 
  SD.begin(pinoCartaoSD);
  ////Define a parada do RTC    
  rtc.halt(false);   
  ////Define o dia da semana
  //rtc.setDOW(SEXTA); 
  ////Define o horario     
  //rtc.setTime(13, 03, 00);
  ////Define o dia, mes e ano      
  //rtc.setDate(11, 11, 2016);     
  ////Definicoes do pino SQW/Out
  rtc.setSQWRate(SQW_RATE_1);
  rtc.enableSQW(true);
  //Define o pino do sensor de fluxo
  pinMode(FLOWSENSORPIN, INPUT);
  
  //Define o pino do sensor de fluxo como high
  digitalWrite(FLOWSENSORPIN, HIGH);
  
  //colhe as informações do pino do sensor de fluxo e armazena na variavel lastflowpinstate
  lastflowpinstate = digitalRead(FLOWSENSORPIN);
  useInterrupt(true);
  
  //DEFINE O PINO DA VALVULA DO RESERVATORIO COMO SAIDA
  pinMode(pino_valvula_reservatorio, OUTPUT); 

  //DEFINE O PINO DA VALVULA DA BOMBA COMO SAIDA
  pinMode(pino_valvula_bomba, OUTPUT);

  //DEFINE O PINO DA BOMBA
  pinMode(pino_bomba, OUTPUT);
}
 
void loop(){
   float tempAmbiente = calcTemperaturaAmbiente();
   float temperaturaReservAquecido = mostrarTemperaturaReservAquecido();
   float temperaturaReserv = mostrarTemperaturaReserv();
   float distancia = mostrarDistancia();
   float vazao = mostrarVazaoGas();
   String data = mostrarData();  
   String hora = mostrarHora();
   String diaDaSemana = mostrarDiaDaSemana();    

   if(temperaturaReservAquecido > (temperaturaReserv-diferencaTemp)){
      ligarValvulaBomba();
      ligarBomba();
      ligarValvulaReservatorio();
      delay(234000);
      desligarBomba();
      desligarValvulaBomba();
      delay(60000);
      desligarValvulaReservatorio();
   }
   
   
   Serial.print(diaDaSemana);
   Serial.print(";");
   Serial.print(data);
   Serial.print(";");
   Serial.print(hora);
   Serial.print(";");
   Serial.print(temperaturaReservAquecido);
   Serial.print(";");
   Serial.print(temperaturaReserv);
   Serial.print(";");
   Serial.print(distancia);
   Serial.print(";");
   Serial.print(tempAmbiente);
   Serial.print(";");
   Serial.print(vazao);
   Serial.print(";\n");

   gravarSDs(diaDaSemana);
   gravarSDs(data);
   gravarSDs(hora);
   gravarSDf(temperaturaReservAquecido);
   gravarSDf(temperaturaReserv);
   gravarSDf(distancia);
   gravarSDf(tempAmbiente);
   gravarSDf(vazao);
   gravarSDf(-100);
   delay(12000);
}

//**FUNÇÕES DO SENSOR DE TEMPERATURA INTERNA**
float mostrarTemperaturaReservAquecido(){
  temperature.requestTemperatures();
  float tempC = temperature.getTempC(address);
  return tempC;
}

float mostrarTemperaturaReserv(){
  temperature.requestTemperatures();
  float tempC = temperature.getTempC(address);
  return tempC;
}

void mostra_endereco_sensor(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // Adiciona zeros se necessário
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

//**FUNÇÃO DO SENSOR DE TEMPERATURA AMBIENTE**
float calcTemperaturaAmbiente(){  
  float temperaturaAmbiente;
  temperaturaAmbiente = (float(analogRead(LM35))*5/(1023))/0.01;
  return temperaturaAmbiente;
}

//**FUNÇÕES DO SENSOR DE DISTANCIA**
float mostrarDistancia(){
  float cmMsec;
  long microsec = ultrasonic.timing();
  cmMsec = ultrasonic.convert(microsec, Ultrasonic::CM);
  return cmMsec;
}

//**FUNÇÕES DO CARTÃO SD**
void iniciaComunicacaoSD(){
  if (!SD.begin(pinoCartaoSD)) 
  {
    Serial.println("Falha ao acessar o cartao !");
    Serial.println("Verifique o cartao/conexoes e reinicie o Arduino...");
    return;
  }
  Serial.println("Cartao iniciado corretamente !");
}

void gravarSDf(float sensor){
  File dataFile = SD.open("arquivo.txt", FILE_WRITE);
  if(count == 0){
    dataFile.println("Temperatura;Distancia;DiaDaSemana;Data;Hora;Vazão;");
    count++;
  }  
    // Grava os dados no arquivo
    if (dataFile) 
    {
      if(sensor == -100){
        dataFile.println();
      }else{
        dataFile.print(sensor);
        dataFile.print(";");
      }
      dataFile.close();
    }  
    else 
    {
      Serial.println("Erro ao abrir arquivo.txt !");
    } 
}

void gravarSDs(String sensor){
  File dataFile = SD.open("arquivo.txt", FILE_WRITE);
    if (dataFile) 
    {
      dataFile.print(sensor);
      dataFile.print(";");
      dataFile.close();
    }  
    else 
    {
      Serial.println("Erro ao abrir arquivo.txt !");
    } 
}

//**FUNÇÕES DO MODULO RTC**
String mostrarData(){
  String data = "";
  data = rtc.getDateStr(FORMAT_SHORT);
  return data;
}

String mostrarHora(){
  String hora = "";
  hora = rtc.getTimeStr();
  return hora;
}

String mostrarDiaDaSemana(){
  String dia = "";
  dia = rtc.getDOWStr(FORMAT_SHORT);
  return dia;
}

//**FUNÇÕES DO SENSOR DE VAZÃO DE GÁS**
float mostrarVazaoGas(){
  float liters = pulses;
  liters /= 7.5;
  liters /= 60;
  return liters *= 1000;
}

//A INTERRUPÇÃO É CHAMADA UMA VEZ POR MILISSEGUNDO, PARA PROCURAR POR PULSOS DO SENSOR
SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(FLOWSENSORPIN);
  
  if (x == lastflowpinstate) {
    lastflowratetimer++;
    return; // nothing changed!
  }
  
  if (x == HIGH) {
    //TRANSIÇÃO DO LOW PARA HIGH
    pulses++;
  }
  lastflowpinstate = x;
  flowrate = 1000.0;
  flowrate /= lastflowratetimer;  // EM HERTZ
  lastflowratetimer = 0;
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
  }
}

//**FUNÇÕES DA VALVULA DO RESERVATORIO**
void ligarValvulaReservatorio(){
  digitalWrite(pino_valvula_reservatorio, HIGH);
}

void desligarValvulaReservatorio(){
  digitalWrite(pino_valvula_reservatorio, LOW);
}

//**FUNÇÕES DA VALVULA DA BOMBA**
void ligarValvulaBomba(){
  digitalWrite(pino_valvula_bomba, HIGH);
}

void desligarValvulaBomba(){
  digitalWrite(pino_valvula_bomba, LOW);
}

//**FUNÇÕES DA BOMBA**
void ligarBomba(){
  digitalWrite(pino_bomba, HIGH);
}

void desligarBomba(){
  digitalWrite(pino_bomba, LOW);
}
