// Snippet to test circuit before connecting to cloud.
// If all goes well, you should observe:
//    - Motor turns once in each direction, 10 times.
//    - Bulb connected to the relay should turn on and off, 10 times.
//    - LED built in to the MKR1000 should light up in sync with the bulb.
//    - Stepper motor should not overheat if you leave the circuit on for 10 minutes.

const int stepPin = 3; 
const int dirPin = 2;
const int sleepPin = 4;
const int resetPin = 5;
const int relayPin = 7;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN,HIGH);
  
  // Sets output pins 
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(sleepPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  // Pull reset pin to high for enabling driver
  digitalWrite(resetPin, HIGH);
  
  // Cut off current to stepper when not in use
  digitalWrite(sleepPin, LOW);
 
  delay(3000);
}


void loop() {
  digitalWrite(sleepPin, HIGH);     // Wake up driver

  // Ten cycles
  for(int i=0;i<10;i++) {
    digitalWrite(relayPin, LOW);    // Turn light on
    digitalWrite(dirPin,LOW);       // Set to one direction
    digitalWrite(LED_BUILTIN,HIGH);

     // Make 400 steps for a single rotation
    for(int x = 0; x < 4000; x++) {
      digitalWrite(stepPin,HIGH); 
      delayMicroseconds(300); 
      digitalWrite(stepPin,LOW); 
      delayMicroseconds(300); 
    }
    
    delay(500);                     // Wait half a second

    digitalWrite(relayPin, HIGH);   // Turn light off
    digitalWrite(dirPin,HIGH);      // Set to the other direction
    digitalWrite(LED_BUILTIN,LOW);

    // Make 400 steps for a single rotation
    for(int x = 0; x < 4000; x++) {
      digitalWrite(stepPin,HIGH); 
      delayMicroseconds(300); 
      digitalWrite(stepPin,LOW); 
      delayMicroseconds(300); 
    }   
  }
  
  digitalWrite(sleepPin, LOW);      // Put driver to sleep
}
