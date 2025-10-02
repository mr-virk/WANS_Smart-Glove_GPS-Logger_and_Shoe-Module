#define buzzer 11 // this pin use for Alarm

#define echopin 7 // echo pin
#define trigpin 8 // Trigger pin

int Alarm = 1, Vibrator = 1;
int cm; // Duration used to calculate distance
int val = 0 ; //Water Level

void setup() { // put your setup code here, to run once
  Serial.begin(9600);// initialize serial communication at 9600 bits per second:

  pinMode(irPin, INPUT);

  pinMode(buzzer, OUTPUT); //declare buzzer as output

  pinMode(trigpin, OUTPUT); // declare ultrasonic sensor Echo pin as input
  pinMode(echopin, INPUT);  // declare ultrasonic sensor Trigger pin as Output
  delay(100);
}

void loop() {
  pitholeDetect();
  delay(10);
  waterCheck();
}

void pitholeDetect() {
  // Write a pulse to the HC-SR04 Trigger Pin
  digitalWrite(trigpin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigpin, HIGH);
  delayMicroseconds(10);

  // Measure the response from the HC-SR04 Echo Pin
  long ultra_time = pulseIn (echopin, HIGH);
  // Determine distance from duration
  // Use 343 metres per second as speed of sound
  cm =  ultra_time / 29 / 2;

  Serial.print("cm:"); Serial.println(cm);

  if (cm < 30) {
    digitalWrite(buzzer, LOW); // Turn off Buzzer
  }

  if (cm > 30) {
    digitalWrite(buzzer, HIGH); // Turn off Buzzer
  }
}

void waterCheck() {
  int s1 = analogRead(A0); // Water Level Sensor output pin connected A0
  Serial.println(s1);  // See the Value In Serial Monitor
  delay(100);      // for timer
  if (s1 > 500 )
  {
    for (int i = 0; i < 10; i++) {
      Serial.print("Water Detected");
      tone(buzzer, 400, 300);
      delay(300);
      noTone(buzzer);
      delay(300);
    }
  }
  else {
    digitalWrite(buzzer, LOW);
  }
}
