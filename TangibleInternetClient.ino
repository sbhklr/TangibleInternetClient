#include <ZTimer.h>

#define BAUD_RATE 19200

#define ROTARY_PIN 2
#define RECEIVER_LEVER 3
#define SELENOID_PIN 4
#define MICROPHONE_PIN A0

#define DIAL_FINISHED_TIMEOUT 100
#define IP_ADDRESS_LENGTH 12
#define INPUT_COMMAND_SIZE 4
#define RINGTONE_DELAY 45
#define RINGTONE_WAIT_TIME 2750
#define TOKEN_BASE_RESISTOR 1000

#define MODE_ARTICLE "a"
#define MODE_DEVELOPER "d"
#define MODE_INCOGNITO "i"
#define MODE_BROWSER_HISTORY "h"

#define MODE_ARTICLE_RESISTOR_VALUE 4700
#define MODE_DEVELOPER_RESISTOR_VALUE 560
#define MODE_INCOGNITO_RESISTOR_VALUE 220
#define MODE_BROWSER_HISTORY_RESISTOR_VALUE 1000
#define MODE_RESISTOR_VALUE_TOLERANCE 0.05


int previousDialState = LOW;
int previousReceiverLeverState = LOW;
String currentMode = MODE_ARTICLE;

int dialHighStateCounter = 0;
unsigned long lastDialReadTime = 0;
int ipAddressDigits[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
int currentIPDigitIndex = 0;

ZTimer ringTimer;

void setupPins(){  
  pinMode(ROTARY_PIN, INPUT);
  pinMode(RECEIVER_LEVER, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SELENOID_PIN, OUTPUT);
  pinMode(MICROPHONE_PIN, INPUT);
}

void setup() {
  Serial.begin(BAUD_RATE);
  setupPins();

  ringTimer.SetWaitTime(RINGTONE_WAIT_TIME);
  ringTimer.SetCallBack([&]() {    
    ring();    
  });
}

void loop() {
  processIncomingCommand();
  stopRingingIfNecessary();
  ringTimer.CheckTime();
  switchMode();
  handlePickingHangingUp();
  handleDialling();  
  delay(10);
}

//LOOP HANDLING

void handlePickingHangingUp(){
  int receiverLeverState = digitalRead(RECEIVER_LEVER);
  if(receiverLeverState != previousReceiverLeverState){
    int hungUp = receiverLeverState == LOW;
    Serial.print(hungUp ? "h:" : "p:");
    Serial.print('\n');

    if(hungUp){
      currentIPDigitIndex = 0;
    }
  }
  previousReceiverLeverState = receiverLeverState;
}

void handleDialling(){
  int dialState = digitalRead(ROTARY_PIN);

  if(dialState != previousDialState && dialState == HIGH){
    ++dialHighStateCounter;    
    lastDialReadTime = millis();
  }

  previousDialState = dialState;  

  bool finishedDiallingDigit = millis() > lastDialReadTime + DIAL_FINISHED_TIMEOUT && dialHighStateCounter > 0;
  
  if(finishedDiallingDigit){    
    ipAddressDigits[currentIPDigitIndex] = getDialledNumber(dialHighStateCounter);    
    sendDiallingCommand(ipAddressDigits[currentIPDigitIndex]);
    dialHighStateCounter = 0;
    ++currentIPDigitIndex;
  }

  bool finishedDiallingNumber = currentIPDigitIndex == IP_ADDRESS_LENGTH;

  if(finishedDiallingNumber) {
   sendConnectCommand();
  }
}

//BUSINESS LOGIC

bool stopRingingIfNecessary(){
  if(digitalRead(RECEIVER_LEVER) == HIGH){
    ringTimer.StopTimer();
    return true;
  }
  return false;
}

int getDialledNumber(int stateChanges) {
  if(stateChanges == 10) return 0;
  return stateChanges;
}

bool checkResistorValue(int resistorValue, int targetValue){
  int lowerBound = targetValue * (1.0 - MODE_RESISTOR_VALUE_TOLERANCE);
  int upperBound = targetValue * (1.0 + MODE_RESISTOR_VALUE_TOLERANCE);
  return resistorValue >= lowerBound && resistorValue <= upperBound;
}

void switchMode(){
  int tokenValue = analogRead(MICROPHONE_PIN);
  float vout = (tokenValue / 1024.0) * 5.0;  
  int resistorValue = (TOKEN_BASE_RESISTOR * vout) / (5.0 - vout);
  
  if(resistorValue <= 0 || vout == 5.0) return;

  String newMode;

  if(checkResistorValue(resistorValue, MODE_ARTICLE_RESISTOR_VALUE)){
    newMode = MODE_ARTICLE;    
  } else if(checkResistorValue(resistorValue, MODE_INCOGNITO_RESISTOR_VALUE)){
    newMode = MODE_INCOGNITO;    
  } else if(checkResistorValue(resistorValue, MODE_DEVELOPER_RESISTOR_VALUE)){
    newMode = MODE_DEVELOPER;    
  } else if(checkResistorValue(resistorValue, MODE_BROWSER_HISTORY_RESISTOR_VALUE)){
    newMode = MODE_BROWSER_HISTORY;    
  } else {
    return;
  }

  if(newMode != currentMode) {
    currentMode = newMode;
    Serial.print("m:");
    Serial.print(currentMode);
    Serial.print('\n');
  }    
}

//SERIAL COMMUNICATION

void processIncomingCommand(){
  if(Serial.available() >= INPUT_COMMAND_SIZE){
    String command = Serial.readStringUntil('\n');    
    if(command.substring(0,3) == "r:1"){                      
      ringTimer.ResetTimer(true);      
      ringTimer.SetLastTime(ringTimer.GetNow()-RINGTONE_WAIT_TIME);
    } else if(command.substring(0,3) == "r:0"){
      ringTimer.StopTimer();
    }
  }
}

void sendConnectCommand(){
  String ipAddress = "";
  for(int i=0; i<IP_ADDRESS_LENGTH; ++i){
      ipAddress += String(ipAddressDigits[i]);
  }
  ipAddress += '\n';
  currentIPDigitIndex = 0;
  String command = "c:";
  Serial.print(command + ipAddress);
}

void sendPickupCommand(){
  Serial.print("p:");
  Serial.print('\n');
}

void sendHangUpCommand(){
  Serial.print("h:");
  Serial.print('\n');
}

void sendDiallingCommand(int digit){
  Serial.print("d:");
  Serial.print(digit);
  Serial.print('\n');
}

//ACTUATORS
void ring() {  
  for (int i = 0; i < 12; ++i) {
    digitalWrite(SELENOID_PIN, HIGH);
    delay(RINGTONE_DELAY);
    digitalWrite(SELENOID_PIN, LOW);
    delay(RINGTONE_DELAY);
    if(stopRingingIfNecessary()) return;
  }
}