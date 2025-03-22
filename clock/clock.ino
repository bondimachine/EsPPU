void setup() {
  TCCR1 = 1<<CTC1 | 0<<COM1A0 | 1<<CS10; // CTC mode
  GTCCR = 1<<COM1B0;                     // Toggle OC1B
  PLLCSR = 0<<PCKE;                      // System clock as clock source
  OCR1C = 64;
  pinMode(4, OUTPUT);
}

void loop() {
  for(;;);
/*
  digitalWrite(4, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(50);                      // wait for a second
  digitalWrite(4, LOW);   // turn the LED off by making the voltage LOW
  delay(50);                      // wait for a second
*/
}