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
Dshot::Dshot(int pin, void(*isr)(void)){
  CHECKSUM_ERR_COUNTER = 0;
  CHECKSUM_SUCCESS_COUNTER = 0;

  PIN_NUM = pin;
  ISR_FN = isr;
  
  decode_flag_off();
  pinMode(pin, OUTPUT);

  tx = true;

}

// Assemble and set the dshot signal
void Dshot::set_throttle(unsigned short input, unsigned short telem){
    
    input = (input << 1) + telem; 
    unsigned short crc = (~(input ^ (input >> 4) ^ (input >> 8))) & 0x0F;
    unsigned short final_sig = (input << 4) + crc;
    
    // Convert from 16bit integer to iterable of booleans
    // Will have to reverse after using the % 2 method
    for(int i=15; i>=0; i--){
        
      if(final_sig % 2 == 1){ dshot_signal[i] = true; }
      else{ dshot_signal[i] = false; }
      
      final_sig /= 2;
      
    }
}

// Synchronous TX 
void Dshot::set_high(){digitalWriteFast(PIN_NUM, HIGH);}

void Dshot::set_low(){digitalWriteFast(PIN_NUM, LOW);}

// Rx state condition: has it been 80 microseconds since we sent the signal
bool Dshot::ready_for_decoding(){
  return rx_done;
}

bool Dshot::ready_for_tx(){
  return tx;
}

// Decodes the timestamp array into a final signal
// and returns RPM values
int Dshot::decode_signal(){

  decode_flag_off();
  detachInterrupt(PIN_NUM); 
  pinMode(PIN_NUM, OUTPUT);

  if(timeRecord[0] == 0){ // Did we receive something?
    reset_array();
    return 0xffff;
  }

  int counter = 0;
  unsigned int rx_sig = 0;
  
  for(int i=0; i<21; i++){
    timeRecord[i] = rint( timeRecord[i]/900.0 ); // Cycle ticks to pulse unit length
    
    for(int j=0; j<timeRecord[i]; j++){
      if(i%2 == 1){ // If the array index is odd, which determines if it's a 1 or a 0
        rx_sig += (1 << (20 - (counter + j)));
      }
    }
    
    counter += timeRecord[i];
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
    Serial.println("Checksum err");
    CHECKSUM_ERR_COUNTER++;
    reset_array();
    return 0xffff;
  }

  CHECKSUM_SUCCESS_COUNTER++;

  // If we receive the max period, assume motor is not spinning
  if((fin_sig >> 4) == 0x0fff){
    reset_array();
    return 0;
  }

  uint8_t shift = fin_sig >> 13;
  uint16_t period_us = (((fin_sig >> 4) & 0b111111111) << shift);
  float eRPM = 1 / ((float)period_us / 60000000);
  reset_array();
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
  timeRecord[i] = val;
}

// 
void Dshot::begin_Rx(){
  ISR_counter = 0;
  pinMode(PIN_NUM, INPUT);
  attachInterrupt(PIN_NUM, ISR_FN, CHANGE); 
}

//
void Dshot::decode_flag_on(){
  rx_done = true;
}

//
void Dshot::decode_flag_off(){
  rx_done = false;
}

void Dshot::tx_flag_on(){
  tx = true;
}

void Dshot::tx_flag_off(){
  tx = false;
}

// ------------------------------------------------------------ //
// Dshot helper functions
// ------------------------------------------------------------ //

//
void Dshot::reset_array(){
  for(int i=0;i<21;i++){
    timeRecord[i] = 0;
  }
}
