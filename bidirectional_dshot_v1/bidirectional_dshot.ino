#include <stdlib.h>
#include <algorithm>
#include <math.h>
#include <TimeLib.h>
#include <string>
#include <bitset>

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
volatile int timeRecord[21] = {0};
//int receiveFrame[21] = {0};
std::bitset<21> receiveFrame;
std::bitset<21> gcr;
//int gcr[21];
unsigned long beginRx;
bool TX = 1;
bool RX = 0;
volatile bool first = 0;
volatile int counter = 0;
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



std::string gcr_map(std::string binary){

  if(binary == "11001"){return "0000";}
  else if(binary == "11011"){return "0001";}
  else if(binary == "10010"){return "0010";}
  else if(binary == "10011"){return "0011";}
  else if(binary == "11101"){return "0100";}
  else if(binary == "10101"){return "0101";}
  else if(binary == "10110"){return "0110";}
  else if(binary == "10111"){return "0111";}
  else if(binary == "11010"){return "1000";}
  else if(binary == "01001"){return "1001";}
  else if(binary == "01010"){return "1010";}
  else if(binary == "01011"){return "1011";}
  else if(binary == "11110"){return "1100";}
  else if(binary == "01101"){return "1101";}
  else if(binary == "01110"){return "1110";}
  else if(binary == "01111"){return "1111";}
  else{return "0000";} //something
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
  
}

// --------------------------------------------------------------------------------------- //

void loop() {

// ----- TRANSMISSION STATE ----- //

  if(TX){

    // If the user sent something through the terminal, update the throttle command
    input_signal = updateCommand(input_signal);

    // Construct & send the signal
    ns2dshot_inv((input_signal, telemetry); // Remove wait time? Should go directly to listening

    // Update flags & variables for Rx
    TX = 0; RX = 1; first = 1;
    beginRx = micros();
    counter = 0; timeStamp = 0;
    for(int i=0; i<21; i++){
      receiveFrame[i] = 0;
      timeRecord[i] = 0;
    }

    // Switch input and get ready to receive a signal
    pinMode(pinNum, INPUT);
    attachInterrupt(pinNum, getTime, CHANGE); // turn on interrupt for signal change
    
  }

// ----- RECEIVE STATE ----- //

  // This will continually loop while interrupts keep track of the signal change timestamps
  if(RX){

    if(micros() - beginRx >= 65){ // Wait 75 us for telemetry signal

      detachInterrupt(pinNum);

      // If we received data
      if(timeRecord[0] != 0){ 

        // These for loops are used to convert the timeRecord array of 
        // pulse lengths to a binary sequence of type std::bitset<16> 
        int index = 0;
        for(int i=0; i<21; i++){

          // Convert from # of ticks to pulse bit length
          int a = rint((double)timeRecord[i]/800);
          timeRecord[i] = a;
          
          for(int j=index; j<index + timeRecord[i]; j++){
              // Check if it's a 0 or a 1 based on index, received signal always starts with a 0
              // Starting from opposite side because bitset has index 0 as LSB
              if(i%2 == 0){receiveFrame[20 - j] = 0;}
              else{receiveFrame[20 - j] = 1;}
          }
          
          index += timeRecord[i];
        }

        // Add trailing 1's for any bits leftover
        for(int k=index; k<21; k++){
          receiveFrame[20 - k] = 1; 
        }

        gcr = (receiveFrame ^ (receiveFrame >> 1));
  
        std::string gcr_str = gcr.to_string();
        //std::reverse(gcr_str.begin(), gcr_str.end()); // Reverse to make MSB first
  
        // Map gcr to 16bit string, ignore the first bit
        std::string sig_str = "";
        for(int i=1; i<21; i+=5){
          sig_str += gcr_map(gcr_str.substr(i,5));
        }

        //std::reverse(sig_str.begin(), sig_str.end()); // Reverse to make LSB index 0      
        std::bitset<16> sig (sig_str); // Convert the signal string back to an array for bitwise operations
  
//        // Debug print statements
//        Serial.print("Ticks: ");
//        for(int i=0; i<21; i++){
//          Serial.print(timeRecord[i]); Serial.print(", ");
//        }
//        Serial.println(".");
        
//        Serial.print("Bits: ");
//        for(int i=20; i>=0; i--){
//          Serial.print(receiveFrame[i]); Serial.print(", ");
//        }
//        Serial.println(".");
  
//        Serial.print("Gcr: ");
//        for(int i=20; i>=0; i--){
//          Serial.print(gcr[i]); Serial.print(", ");
//        }
//        Serial.println(".");
  
//        Serial.print("Gcr string: ");
//        for(int i=0; i<21; i++){
//          Serial.print(gcr_str[i]); Serial.print(", ");
//        }
//        Serial.println(".");
//  
//        Serial.print("Signal string: ");
//        //std::reverse(sig_str.begin(), sig_str.end()); // Reverse again so MSB is index 0 for printing
//        for(int i=0; i<16; i++){
//          Serial.print(sig_str[i]); Serial.print(", ");
//        }
//        Serial.println(".");
  
//        Serial.print("Final signal: ");
//        for(int i=15; i>=0; i--){
//          Serial.print(sig[i]); Serial.print(", ");
//        }
//        Serial.println(".");

// (mychar & 0x0F) & 0b1101
  
        // Confirm checksum
        std::bitset<16> mask (0x0F);
        std::bitset<16> checksum_calc (((sig ^ (sig >> 4) ^ (sig >> 8)) >> 4) & mask);
        std::bitset<16> checksum_received (sig & mask);
//        if(checksum_calc == checksum_received){ // Shift an extra 4 to the right since we have 16 bits, not 12
//        Serial.println("Checksum good");
        std::bitset<16> telem_mask (std::string("0001111111110000"));
        int shift = (int)((sig >> 13).to_ulong());
        std::bitset<16> period_bits (((sig & telem_mask) >> 4) << shift);
        unsigned long period = period_bits.to_ulong(); // Rotational period in microseconds
        double eRPM = 1 / ((double)period / 60000000.); // Reciprocal of period in minutes
//        Serial.print("ERPM: "); Serial.println(eRPM);
        Serial.print("RPM: "); Serial.println((long)eRPM / 7.);
          
//        }else{
//          Serial.println("Checksum no good");
//          
//          Serial.print("Received checksum: "); 
//          for(int i=20;i>=0;i--){
//            Serial.print(checksum_received[i]);
//            Serial.print(", "); 
//          }
//          Serial.println("");
//          
//          Serial.print("Calculated checksum: "); 
//          for(int i=20;i>=0;i--){
//            Serial.print(checksum_calc[i]);
//            Serial.print(", "); 
//          }
//          Serial.println("");
//        
//        }

      
        
      }      

      // Update flags & variables
      RX = 0; TX = 1;

      // Wait a little, switch to output
      //delayMicroseconds(20);
      pinMode(pinNum, OUTPUT);
      
    }
    
  }
  
}
