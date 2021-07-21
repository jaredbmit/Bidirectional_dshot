/**
 * @file bidirectional_dshot_v5.ino
 * @brief DMA-enabled dshot600 communication example w/ multiple ESCs
 * @author Jared Boyer <jaredb@mit.edu>
 * @date 20 Jul 2021
 */

#include "dshot_manager.h"

//=============================================================================
// configuration options
//=============================================================================

static constexpr int NUM_ESC = 2; // Max up to 6 ESC's

//=============================================================================
// ESC comms init
//=============================================================================

DshotManager manager();

//=============================================================================
// Global variables
//=============================================================================

uint16_t RPM;

//=============================================================================
// setup
//=============================================================================

void setup(){
  Serial.begin(115200); // Serial monitor comms for now

  for(int i = 0; i < NUM_ESC; i++){
    manager.set_throttle_esc(i, 90 + i * 50);
  }

  manager.start_tx();

}

//=============================================================================
// loop
//=============================================================================

void loop(){

  updateCommand();

  if( manager.ready_for_decoding() ){
    for( int i = 0; i < NUM_ESC; i++ ){
      RPM = manager.decode_signal(i);
      Serial.print("Motor "); Serial.print(i); Serial.print(": "); Serial.println(RPM);
    }
  }

}

//=============================================================================
// temporary comms helper function
//=============================================================================

void updateCommand(){

  // Get user input command
  if(Serial.available()){
    char new_input[4];
    int n = 0;
    while (Serial.available()){
      new_input[n] = Serial.read();
      n++;
    }
    uint16_t input_signal = atoi(new_input);
    for(int i = 0; i < NUM_ESC; i++){
      manager.set_throttle_esc(i, input_signal);
    }
  }

}
