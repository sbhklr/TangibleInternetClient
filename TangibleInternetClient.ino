#include <ZTimer.h>

#define BAUD_RATE 9600

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

#define MODE_ARTICLE 0
#define MODE_INCOGNITO 1
#define MODE_DEVELOPER 2
#define MODE_BROWSER_HISTORY 3

int previousDialState = LOW;
int previousReceiverLeverState = LOW;
int currentMode = MODE_ARTICLE;

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

void switchMode(){
  int tokenValue = analogRead(MICROPHONE_PIN);
  float vout = (tokenValue / 1024.0) * 5;
  int resistorValue = TOKEN_BASE_RESISTOR * (5.0 / vout -1);
  int newMode;

  if(resistorValue > 4500 && resistorValue < 5100){
    newMode = MODE_DEVELOPER;
    return;
  }

  if(resistorValue > 6000 && resistorValue < 7000){
    newMode = MODE_DEVELOPER;
    return;
  }

  newMode = MODE_ARTICLE;

  if(newMode != currentMode) {
    currentMode = newMode;
    Serial.print("m:");
    Serial.println(currentMode);
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