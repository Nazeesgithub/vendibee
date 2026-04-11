// Stepper motor running with two button

#include <Stepper.h>
int buttonPressPin1 = 2;  //wheel rotation control
int buttonPressPin2 = 3;  //RFID signal control
bool buttonPressed1;
bool buttonPressed2;
const int stepsPerRevolution = 2048;
Stepper stepperName = Stepper(stepsPerRevolution, 8, 10, 9, 11);
void setup() {
 Serial.begin(9600);
  stepperName.setSpeed(5);  
  pinMode(buttonPressPin1, INPUT);
  pinMode(buttonPressPin2, INPUT);
 
  buttonPressed1 = false;  
 buttonPressed2 = true; 
}

void loop() {
  //stepperName.step(2048);
  
    buttonPressed1 = digitalRead(buttonPressPin1);
    buttonPressed2 = digitalRead(buttonPressPin2);
    if(buttonPressed1 == false) { stepperName.step(2048/512);}  // val=0
    if (buttonPressed1 == true) { stepperName.step(0);}       // val=1

    if (buttonPressed2 == false) { stepperName.step(2048/3);}
     

   
    }
    
