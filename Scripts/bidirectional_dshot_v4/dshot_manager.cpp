/**
 * @file dshot_manager.cpp
 * @brief Bidirectional dshot communication protocol manager class for multiple ESCs
 * @author Jared Boyer <jaredb@mit.edu>
 * @date 02 Jul 2021
 */

#include "dshot_manager.h"

#define RX_LISTEN_TIME 75 // Time in microseconds spent in input mode

/* Which of these global vars go in the constructor and which go in different files? */

// No telemetry request for bidirectional dshot
static constexpr int telemetry = 0;

static Dshot* DshotPtrs[8];

void(*isr[8])(void) = {ISR_0, ISR_1, ISR_2, ISR_3, ISR_4, ISR_5, ISR_6, ISR_7};
void(*rx_isr)(void) = rx_timer_ISR;

IntervalTimer rxTimer;

DshotManager::DshotManager(std::vector<int> pins){

  // Start ARM cycle counter
  ARM_DEMCR |= ARM_DEMCR_TRCENA; // debug exception monitor control register; enables trace and debug
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;

  NUM_ESC = pins.size();

  for(int i=0; i<8; i++){
    if(i<NUM_ESC){
  	dshot_.emplace_back(Dshot(pins[i], isr[i]));
  	DshotPtrs[i] = &(dshot_.back());
    	DshotPtrs[i]->ISR_counter = 0;
    }else{
    	DshotPtrs[i] = 0; // Null pointer
    }
  }

}

bool DshotManager::esc_ready(int i){
	return DshotPtrs[i]->ready_for_decoding();
}

int DshotManager::decode_esc(int i){
	DshotPtrs[i]->tx_flag_on();
	return DshotPtrs[i]->decode_signal();
}

void DshotManager::set_throttle_esc(int i, int input){
	DshotPtrs[i]->set_throttle(input, telemetry);
}

int DshotManager::get_num_esc(){
	return NUM_ESC;
}

bool DshotManager::Tx_state(){
  for(int i=0; i<NUM_ESC; i++){
    if(!DshotPtrs[i]->ready_for_tx()){return false;}
  }
  return true;
}

// This is a bootleg way of sending signals synchronously, but who knows it might work
void DshotManager::send_sync(){

  int i; int j;
	noInterrupts();
	// 16 bit signals
	for(i = 0; i<16; i++){ 

		// 3 distinct portions of each bit: beginning, middle, end.
		// The beginning is always low, end is always high, but 
		// the middle is low if it's a 1 bit and high if it's a 0 bit.
    // (This is inverted dshot, highs / lows are reversed from normal)
    for(j = 0; j<NUM_ESC; j++){ digitalWriteFast(DshotPtrs[j]->PIN_NUM, LOW); }
    BEGIN_WAIT;

    for(j = 0; j<NUM_ESC; j++){
      if(DshotPtrs[j]->dshot_signal[i]){ digitalWriteFast(DshotPtrs[j]->PIN_NUM, LOW); }
      else{ digitalWriteFast(DshotPtrs[j]->PIN_NUM, HIGH); }
    }
    MIDDLE_WAIT;

    for(j = 0; j<NUM_ESC; j++){ digitalWriteFast(DshotPtrs[j]->PIN_NUM, HIGH); }
    END_WAIT;

	}

	interrupts();
  
	for(i = 0; i<NUM_ESC; i++){
    DshotPtrs[i]->tx_flag_off();
		DshotPtrs[i]->begin_Rx();
	}

	rxTimer.begin(rx_isr, RX_LISTEN_TIME);

}

//=============================================================================
// RX ISR: not a Dshot class member
//=============================================================================

void rx_timer_ISR(){ //what needs to be declared volatile?
  for(int i=0; i<8; i++){
    // If not a null pointer
    if(DshotPtrs[i]){ DshotPtrs[i]->decode_flag_on(); }
  }
  rxTimer.end();
}

// ISR's have no info on the class that called them, meaning we have to explicitly
// write which object (and corresponding variables) are being changed by using multiple
// unique ISR's

// We'll set 8 ISR's, meaning this code is viable for use with rotorcrafts
// with up to 8 motors.

void ISR_0() {
  if(DshotPtrs[0]->ISR_counter == 0){
    DshotPtrs[0]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[0]->ISR_counter++;
  }else{
    DshotPtrs[0]->lastTime = DshotPtrs[0]->timeStamp; 
    DshotPtrs[0]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[0]->set_timeRecord(DshotPtrs[0]->ISR_counter-1, DshotPtrs[0]->timeStamp - DshotPtrs[0]->lastTime);
    DshotPtrs[0]->ISR_counter++;
  }
}

void ISR_1() {
  if(DshotPtrs[1]->ISR_counter == 0){
    DshotPtrs[1]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[1]->ISR_counter++;
  }else{
    DshotPtrs[1]->lastTime = DshotPtrs[1]->timeStamp; 
    DshotPtrs[1]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[1]->set_timeRecord(DshotPtrs[1]->ISR_counter-1, DshotPtrs[1]->timeStamp - DshotPtrs[1]->lastTime);
    DshotPtrs[1]->ISR_counter++;
  }
}

void ISR_2() {
  if(DshotPtrs[2]->ISR_counter == 0){
    DshotPtrs[2]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[2]->ISR_counter++;
  }else{
    DshotPtrs[2]->lastTime = DshotPtrs[2]->timeStamp; 
    DshotPtrs[2]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[2]->set_timeRecord(DshotPtrs[2]->ISR_counter-1, DshotPtrs[2]->timeStamp - DshotPtrs[2]->lastTime);
    DshotPtrs[2]->ISR_counter++;
  }
}

void ISR_3() {
  if(DshotPtrs[3]->ISR_counter == 0){
    DshotPtrs[3]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[3]->ISR_counter++;
  }else{
    DshotPtrs[3]->lastTime = DshotPtrs[3]->timeStamp; 
    DshotPtrs[3]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[3]->set_timeRecord(DshotPtrs[3]->ISR_counter-1, DshotPtrs[3]->timeStamp - DshotPtrs[3]->lastTime);
    DshotPtrs[3]->ISR_counter++;
  }
}

void ISR_4() {
  if(DshotPtrs[4]->ISR_counter == 0){
    DshotPtrs[4]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[4]->ISR_counter++;
  }else{
    DshotPtrs[4]->lastTime = DshotPtrs[4]->timeStamp; 
    DshotPtrs[4]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[4]->set_timeRecord(DshotPtrs[4]->ISR_counter-1, DshotPtrs[4]->timeStamp - DshotPtrs[4]->lastTime);
    DshotPtrs[4]->ISR_counter++;
  }
}

void ISR_5() {
  if(DshotPtrs[5]->ISR_counter == 0){
    DshotPtrs[5]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[5]->ISR_counter++;
  }else{
    DshotPtrs[5]->lastTime = DshotPtrs[5]->timeStamp; 
    DshotPtrs[5]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[5]->set_timeRecord(DshotPtrs[5]->ISR_counter-1, DshotPtrs[5]->timeStamp - DshotPtrs[5]->lastTime);
    DshotPtrs[5]->ISR_counter++;
  }
}

void ISR_6() {
  if(DshotPtrs[6]->ISR_counter == 0){
    DshotPtrs[6]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[6]->ISR_counter++;
  }else{
    DshotPtrs[6]->lastTime = DshotPtrs[6]->timeStamp; 
    DshotPtrs[6]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[6]->set_timeRecord(DshotPtrs[6]->ISR_counter-1, DshotPtrs[6]->timeStamp - DshotPtrs[6]->lastTime);
    DshotPtrs[6]->ISR_counter++;
  }
}

void ISR_7() {
  if(DshotPtrs[7]->ISR_counter == 0){
    DshotPtrs[7]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[7]->ISR_counter++;
  }else{
    DshotPtrs[7]->lastTime = DshotPtrs[7]->timeStamp; 
    DshotPtrs[7]->timeStamp = ARM_DWT_CYCCNT; 
    DshotPtrs[7]->set_timeRecord(DshotPtrs[7]->ISR_counter-1, DshotPtrs[7]->timeStamp - DshotPtrs[7]->lastTime);
    DshotPtrs[7]->ISR_counter++;
  }
}
