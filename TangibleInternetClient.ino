//#include <ZTimer.h>

#define BAUD_RATE 9600

#define ROTARY_PIN 2
#define RECEIVER_LEVER 3

#define DIAL_FINISHED_TIMEOUT 100
#define IP_ADDRESS_LENGTH 12

int previousDialState = LOW;
int previousReceiverLeverState = LOW;

int dialHighStateCounter = 0;
unsigned long lastDialReadTime = 0;
int ipAddressDigits[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
int currentIPDigitIndex = 0;

//ZTimer dialTimer;

void setupPins(){  
  pinMode(ROTARY_PIN, INPUT);
}

void setup() {
  Serial.begin(BAUD_RATE);
  setupPins();

  /*
  dialTimer.SetWaitTime(1000);
  dialTimer.SetCallBack([&]() {
    Serial.println(getDialledNumber(dialHighStateCounter));    
    dialTimer.StopTimer();
  });
  */

  //Serial.println("Tangible Internet ready.");
}

void loop() {    
  handlePickingHangingUp();
  handleDialling();
  delay(10);
}

//LOOP HANDLING

void handlePickingHangingUp(){
  int receiverLeverState = digitalRead(RECEIVER_LEVER);
  if(receiverLeverState != previousReceiverLeverState){
    int pickedUp = receiverLeverState == HIGH;
    Serial.print(pickedUp ? "p:" : "h:");
    Serial.print('\n');
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
    if(currentIPDigitIndex == 0) sendDiallingCommand();
    
    ipAddressDigits[currentIPDigitIndex] = getDialledNumber(dialHighStateCounter);
    dialHighStateCounter = 0;
    ++currentIPDigitIndex;
  }

  bool finishedDiallingNumber = currentIPDigitIndex == IP_ADDRESS_LENGTH;

  if(finishedDiallingNumber) {
   sendConnectCommand();
  }
}


//BUSINESS LOGIC

int getDialledNumber(int stateChanges) {
  if(stateChanges == 10) return 0;
  return stateChanges;
}

//COMMANDS

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

void sendDiallingCommand(){
  Serial.print("d:");
  Serial.print('\n');
}