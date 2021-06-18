// Written by Bill Kuhl with guidance from Parker Lusk for the MIT Aerospace Controls Lab
//
// Works with teensy 4.0
// 28 May 2021

#include <stdlib.h>
#include <math.h>
#include <TimeLib.h>

#define pinNum 1

// P1-5 are 100-500 ns pauses, tested with an oscilloscope (2 second
// display persistence) and a Teensy 3.2 compiling with
// Teensyduino/Arduino 1.8.1, "faster" setting
#define NOP1 "nop\n\t"
#define NOP5 "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
#define NOP10 "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
#define NOP50 NOP10 NOP10 NOP10 NOP10 NOP10
#define NOP100 NOP50 NOP50
#define NOP1000 NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP100

// dshot150
//#define ON_HIGH __asm__(NOP1000 NOP1000 NOP1000 NOP1000 NOP1000 NOP1000 NOP1 NOP1) //5000ns
//#define ON_LOW __asm__(NOP1000 NOP1000) //1666ns
//#define OFF_HIGH __asm__(NOP1000 NOP1000 NOP1000 NOP1) //2500ns
//#define OFF_LOW __asm__(NOP1000 NOP1000 NOP1000 NOP1000 NOP1000) //4166ns

// dshot300
//#define ON_HIGH __asm__(NOP1000 NOP1000 NOP1000 NOP1) //2500ns
//#define ON_LOW __asm__(NOP1000)//833.3ns
//#define OFF_HIGH __asm__(NOP1000 NOP100 NOP100 NOP100 NOP100 NOP100) //1250ns
//#define OFF_LOW __asm__(NOP1000 NOP1000 NOP100 NOP100 NOP100 NOP100 NOP100 NOP1) //2083.3ns

// dshot600
#define ON_HIGH __asm__(NOP1000 NOP100 NOP100 NOP100 NOP100 NOP100) //1250ns
#define ON_LOW __asm__(NOP100 NOP100 NOP100 NOP100 NOP100) //416.6ns
#define OFF_HIGH __asm__(NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP50) //0625ns
#define OFF_LOW __asm__(NOP1000 NOP100 NOP100 NOP50) //1041.6ns

// dshot1200
//#define ON_HIGH __asm__(NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP50)//0625ns
//#define ON_LOW __asm__(NOP100 NOP100 NOP50) //208.33ns
//#define OFF_HIGH __asm__(NOP100 NOP100 NOP100 NOP50 NOP5 NOP5 NOP5 NOP5 NOP5)//312.5ns
//#define OFF_LOW __asm__(NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP5 NOP5 NOP5 NOP5 NOP5)//520.8ns

int input_signal = 104;
volatile unsigned long timeStamp = 0; //system clock captured in interrupt
volatile unsigned long lastTime = 0;
int timeRecord[21] = {0};
int receiveFrame[21] = {0};
unsigned long beginRx;
bool TX = 1;
bool RX = 0;
bool first = 0;
bool writeToList = 0;
int counter = -1;
int duration;
int sum;
int telemetry = 0;



void int2bin (int num,int* si){
  
  int rev[11];
  int i = 0;
  while (num > 0) {
    rev[i] = num % 2;
    num = num/2;
    i++;
  }
  while (i<11) {
      rev[i] = 0;
      i++;
  }
  
  //reverse it
  for (int j = 0; j<11; j++){
      si[j] = rev[10-j];
  } 
}



void crc2bin (int num,int* si){
  
  int rev[4];
  int i = 0;
  while (num > 0) {
    rev[i] = num % 2;
    num = num/2;
    i++;
  }
  while (i<4) {
      rev[i] = 0;
      i++;
  }
  
  //reverse it
  for (int j = 0; j<4; j++){
      si[12+j] = rev[3-j]; 
  }
}



void inv_on(){
  
  digitalWriteFast(pinNum,LOW);
  ON_HIGH;
  digitalWriteFast(pinNum,HIGH);
  ON_LOW;
  
}



void inv_off(){

  digitalWriteFast(pinNum, LOW);
  OFF_HIGH;
  digitalWriteFast(pinNum, HIGH);
  OFF_LOW;
  
}



void ns2dshot_inv(int signal_ds, int telemetry){

    int final_sig[16];
    int2bin(signal_ds,final_sig);
    final_sig[11] = telemetry; //Telemetry Request
    signal_ds = (signal_ds<<1)+telemetry;
    int crc = (~(signal_ds ^ (signal_ds >> 4) ^ (signal_ds >> 8))) & 0x0F;
   
    crc2bin(crc,final_sig);

    noInterrupts();
    
    for (int i = 0; i<16; i++) {
      if (final_sig[i] == 1) inv_on();
      else if (final_sig[i] == 0) inv_off();
    }
    digitalWriteFast(pinNum,HIGH);
    interrupts();
    delayMicroseconds(20);
  
}



int updateCommand(int input_signal){

  // Get user input command
  if (Serial.available()){
    char new_input[4];
    int n = 0;
    while (Serial.available()){
      new_input[n] = Serial.read();
      n++;
    }
    input_signal = atoi(new_input);
  }

  return input_signal;
  
}



void getTime() {
  
  if(first){
    timeStamp = ARM_DWT_CYCCNT; 
    lastTime = timeStamp;
    first = 0;
  }else{
    lastTime = timeStamp; 
    timeStamp = ARM_DWT_CYCCNT; 
    timeRecord[counter] = timeStamp - lastTime;
    counter++;
    Serial.print("*I*");
  }
  
}

// --------------------------------------------------------------------------------------- //

void setup() {
  pinMode(pinNum, OUTPUT);
  Serial.begin(115200);

  // Start ARM cycle counter
  ARM_DEMCR |= ARM_DEMCR_TRCENA;  // debug exception monitor control register; enables trace and debug
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
  
}

// --------------------------------------------------------------------------------------- //

void loop() {

// ----- TRANSMISSION STATE ----- //

  if(TX){

    // If the user sent something through the terminal, update the throttle command
    input_signal = updateCommand(input_signal);

    // Construct & send the signal
    ns2dshot_inv(input_signal, telemetry); // Remove wait time? Should go directly to listening

    // Update flags & variables for Rx
    TX = 0; RX = 1; first = 1;
    beginRx = micros();
    counter = -1; timeStamp = 0;
    for(int i=0; i<21; i++){
      receiveFrame[i] = 0;
      timeRecord[i] = 0;
    }

    // Switch input and get ready to receive a signal
    pinMode(pinNum, INPUT);
    attachInterrupt(pinNum, getTime, CHANGE); // turn on interrupt for signal change
    
  }

// ----- RECEIVE STATE ----- //

  // This will continually loop, documenting updates that happen as the attachInterrupt()
  // marks the timestamps of signal switches
  if(RX){

    if(micros() - beginRx == 30 || micros() - beginRx == 45 || micros() - beginRx == 15){
      Serial.print("Time: "); Serial.print(micros() - beginRx); Serial.print(", Counter: "); Serial.println(counter);
    }
    
    if(micros() - beginRx > 60){ // If Rx times out

      detachInterrupt(pinNum);
      
      // Debug print statements
      Serial.println("Wait 60 micros since signal starts ---");
      Serial.print("Micros passed: "); Serial.println(micros() - beginRx);
      Serial.print("Timer: ");
      for(int i=0; i<21; i++){
        Serial.print(timeRecord[i]); Serial.print(", ");
      }
      Serial.println(".");

      for(int i=0; i<21; i++){
        int a = rint((double)timeRecord[i]/800);
        timeRecord[i] = a;
      }

      for(int i=0; i<21; i++){
        Serial.print(timeRecord[i]); Serial.print(", ");
      }
      Serial.println(".");

      // Check if we have received the full signal
      sum = 0;
      for(int i=0; i<21; i++){
        sum += timeRecord[i];
      }

      // Convert the timeRecord array to an array of 1's and 0's
      // Only if we have a valid signal
      if(sum >= 21){ 
        
        int index = 0;
        for(int i=0; i<21; i++){
          for(int j=0; j<timeRecord[i]; j++){
      
            // Stop if we have more data than the expected 21 bits received
            if(index + j < 21){ 
              
              // Check if it's a 0 or a 1 based on index, received signal always starts with a 0
              if(i%2 == 0){receiveFrame[index + j] = 0;}
              else{receiveFrame[index + j] = 1;}
            }
          }
          
          index += timeRecord[i];
        }
      }

      // Update flags & variables
      RX = 0; TX = 1;

      // Wait a little, switch to output
      delayMicroseconds(35);
      pinMode(pinNum, OUTPUT);
      
    }
    
  }
  
}
