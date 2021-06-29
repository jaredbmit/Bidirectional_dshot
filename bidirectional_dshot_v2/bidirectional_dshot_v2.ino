#include <stdlib.h>
#include <math.h>
#include <TimeLib.h>
#include <cmath>
#include <stdint.h>

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

// dshot600
#define ON_HIGH __asm__(NOP1000 NOP100 NOP100 NOP100 NOP100 NOP100) //1250ns
#define ON_LOW __asm__(NOP100 NOP100 NOP100 NOP100 NOP100) //416.6ns
#define OFF_HIGH __asm__(NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP50) //0625ns
#define OFF_LOW __asm__(NOP1000 NOP100 NOP100 NOP50) //1041.6ns

float input_signal = 90;
unsigned long beginRx;
unsigned long startTime = 0;
static const int telemetry = 0;
bool TX = 1;
bool RX = 0;
bool err;
bool iterate = false;
bool forwards = true;
int checksum_err_counter = 0;
int checksum_succ_counter = 0;

// Dshot feedback ISR
volatile unsigned long timeStamp = 0; //system clock captured in interrupt
volatile unsigned long lastTime = 0;
volatile int timeRecord[21] = {0};
volatile bool first = 0;
volatile int counter = 0;

// Averaging the RPM's down to a lower data rate
int RPM_sum = 0;
int RPM_counter = 0;
int signal_sum = 0;
int signal_counter = 0;
int cmd_counter = 0;



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



int decode_signal(volatile int * array_pointer){

    int counter = 0;
    unsigned int rx_sig = 0;

    for(int i=0; i<21; i++){
      *(array_pointer + i) = rint((*(array_pointer + i))/800.0); // Cycle ticks to pulse unit length
      
      for(int j=0; j<*(array_pointer + i); j++){
        if(i%2 == 1){ // If the array index is odd, which determines if it's a 1 or a 0
          rx_sig += (1 << (20 - (counter + j)));
        }
      }
      
      counter += *(array_pointer + i);
    }

    // Pad 1's at the end
    for(int k=counter; k<21; k++){
      rx_sig += (1 << (20 - k));
    }

    unsigned int gcr = (rx_sig ^ (rx_sig >> 1));

    static const uint8_t gcr_map[32] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 10, 11, 0, 13, 14, 15,
      0, 0, 2, 3, 0, 5, 6, 7, 0, 0, 8, 1, 0, 4, 12, 0 };

    uint16_t fin_sig = gcr_map[gcr & 0x1f];
    fin_sig |= gcr_map[(gcr >> 5) & 0x1f] << 4;
    fin_sig |= gcr_map[(gcr >> 10) & 0x1f] << 8;
    fin_sig |= gcr_map[(gcr >> 15) & 0x1f] << 12;

    uint16_t csum = fin_sig;
    csum = csum ^ (csum >> 8);
    csum = csum ^ (csum >> 4);

    // Checksum calculation
    if((csum & 0xf) != 0xf){
      err = 1;
      checksum_err_counter++;
      return 0;
    }

    checksum_succ_counter++;

    if((fin_sig >> 4) == 0x0fff){
      return 0;
    }

    uint8_t shift = fin_sig >> 13;
    uint16_t period_us = (((fin_sig >> 4) & 0b111111111) << shift);
    float eRPM = 1 / ((float)period_us / 60000000);
    return eRPM/7;

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
  }
  
}



// --------------------------------------------------------------------------------------- //

void setup() {
  pinMode(pinNum, OUTPUT);
  Serial.begin(115200);

  // Start ARM cycle counter
  ARM_DEMCR |= ARM_DEMCR_TRCENA;  // debug exception monitor control register; enables trace and debug
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;

  Serial.print("Input command, "); Serial.println("Dshot RPM Sample");
  
}

// --------------------------------------------------------------------------------------- //

void loop() {
  
  if(TX){

    // If the user sent something through the terminal, update the throttle command
    input_signal = updateCommand(input_signal);

    // Construct & send the signal
    ns2dshot_inv((int)input_signal, telemetry);

    // Update flags & variables for Rx
    TX = 0; RX = 1; first = 1; counter = 0; timeStamp = 0; err = 0;
    beginRx = micros();
    for(int i=0; i<21; i++){
      timeRecord[i] = 0;
    }

    // Switch input and get ready to receive a signal
    pinMode(pinNum, INPUT);
    attachInterrupt(pinNum, getTime, CHANGE);
   
  }

  // This will continually loop while interrupts keep track of the signal change timestamps
  if(RX){

    if(micros() - beginRx >= 60){

      detachInterrupt(pinNum);

      // If we received data
      if(timeRecord[0] != 0){ 
        
        unsigned short RPM = decode_signal(&timeRecord[0]);
        
        if(!err){
          signal_sum += input_signal; RPM_sum += RPM;
          RPM_counter++; signal_counter++;
        }

        if(cmd_counter % 8 == 0){
        
          Serial.print((float)signal_sum/signal_counter); Serial.print(", "); 
          Serial.println((float)RPM_sum/RPM_counter);
          
          signal_sum = 0;
          signal_counter = 0;
          RPM_sum = 0;
          RPM_counter = 0;
          cmd_counter = 0;
          
        }
        
      }      

      // Update flags
      cmd_counter++;
      RX = 0; TX = 1;
      pinMode(pinNum, OUTPUT);
      
    }
  }
}
