//#include <ZTimer.h>

#define BAUD_RATE 9600
#define ROTARY_PIN 2
#define DIAL_FINISHED_TIMEOUT 100
#define IP_ADDRESS_LENGTH 12

int previousDialState = 0;
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
  int dialState = digitalRead(ROTARY_PIN);

  if(dialState != previousDialState && dialState == HIGH){
    ++dialHighStateCounter;    
    lastDialReadTime = millis();
  }

  previousDialState = dialState;  

  if(millis() > lastDialReadTime + DIAL_FINISHED_TIMEOUT && dialHighStateCounter > 0){
    //Serial.println(getDialledNumber(dialHighStateCounter));
    ipAddressDigits[currentIPDigitIndex] = getDialledNumber(dialHighStateCounter);
    dialHighStateCounter = 0;
    ++currentIPDigitIndex;
  }

  if(currentIPDigitIndex == IP_ADDRESS_LENGTH) {
    String ipAddress = "";
    for(int i=0; i<IP_ADDRESS_LENGTH; ++i){
        ipAddress += String(ipAddressDigits[i]);
    }
    ipAddress += '\n';
    currentIPDigitIndex = 0;
    String command = "c:";
    Serial.print(command + ipAddress);
  }

  delay(10);
}

int getDialledNumber(int stateChanges) {
  if(stateChanges == 10) return 0;
  return stateChanges;
}