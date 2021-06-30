/**
 * @file bidirectional_dshot.cpp
 * @brief Bidirectional dshot communication protocol for teensy 4.0
 * @author Jared Boyer <jaredb@mit.edu>
 * @date 29 Jun 2021
 */

#include "bidirectional_dshot.h"

// ------------------------------------------------------------ //
// Dshot Class definitions
// ------------------------------------------------------------ //

// Constructor, initialize empty parameters
Dshot::Dshot(int pin){
  for(int i=0;i<21;i++){
    timeRecord[i] = 0;  
  }
  array_pointer = &timeRecord[0];
  CHECKSUM_ERR_COUNTER = 0;
  CHECKSUM_SUCCESS_COUNTER = 0;
  PIN_NUM = pin;
}

// Update the dshot throttle command
void Dshot::set_throttle(int input, int telem){
  input_signal = input;
  telemetry = telem;
}

// Assembling & sending the dshot signal
void Dshot::send_inv(){

  int final_sig[16];
  int2bin(input_signal,final_sig);
  final_sig[11] = telemetry; //Telemetry Request
  input_signal = (input_signal<<1)+telemetry;
  int crc = (~(input_signal ^ (input_signal >> 4) ^ (input_signal >> 8))) & 0x0F;
 
  crc2bin(crc,final_sig);

  noInterrupts();
  
  for (int i = 0; i<16; i++) {
    if (final_sig[i] == 1) inv_on();
    else if (final_sig[i] == 0) inv_off();
  }
  digitalWriteFast(PIN_NUM,HIGH);
  interrupts();
  
}

// ---
void Dshot::start_Rx(){
  // Attach interrupt? How to get ISR into class? Is that too connected to hardware?
  begin_timestamp = micros();
}

// Rx state condition: has it been 80 microseconds since we sent the signal
bool Dshot::Rx_timeout(){
  return (micros() - begin_timestamp >= 80);
}

// ---
void Dshot::stop_Rx(){
  // Detach interrupt? Is that too connected to hardware? Is this fn useless?
}

// Decodes the timestamp array into a final signal
// and returns RPM values
int Dshot::decode_signal(){

  if(*array_pointer == 0){ // Did we receive something?
    return 0xffff;
  }

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
    CHECKSUM_ERR_COUNTER++;
    return 0xffff;
  }

  CHECKSUM_SUCCESS_COUNTER++;

  // If we receive the max period, assume motor is not spinning
  if((fin_sig >> 4) == 0x0fff){
    return 0;
  }

  uint8_t shift = fin_sig >> 13;
  uint16_t period_us = (((fin_sig >> 4) & 0b111111111) << shift);
  float eRPM = 1 / ((float)period_us / 60000000);
  return eRPM/7;

}

// Getters for error status
int Dshot::get_err_counter(){
  return CHECKSUM_ERR_COUNTER;
}

int Dshot::get_success_counter(){
  return CHECKSUM_SUCCESS_COUNTER;
}

// Setter for timestamp array
void Dshot::set_timeRecord(int i, int val){
  *(array_pointer + i) = val;
}

// ------------------------------------------------------------ //
// Dshot helper functions
// ------------------------------------------------------------ //

// Signal assembly helper function
void Dshot::int2bin (int num,int * si){
  
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

// Signal assembly helper function
void Dshot::crc2bin (int num,int * si){
  
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

// Send a "1"
void Dshot::inv_on(){
  
  digitalWriteFast(PIN_NUM,LOW);
  ON_HIGH;
  digitalWriteFast(PIN_NUM,HIGH);
  ON_LOW;
  
}

// Send a "0"
void Dshot::inv_off(){

  digitalWriteFast(PIN_NUM, LOW);
  OFF_HIGH;
  digitalWriteFast(PIN_NUM, HIGH);
  OFF_LOW;
  
}
