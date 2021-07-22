/**
 * @file example_dshot.ino
 * @brief Dshot communication example using bidirectional_dshot.h/.cpp
 * @author Jared Boyer <jaredb@mit.edu>
 * @date 30 Jun 2021
 */

#include "bidirectional_dshot.h"

//=============================================================================
// Pin definitions
//=============================================================================

#define PIN_NUM 1

//=============================================================================
// Global variables
//=============================================================================

static constexpr int telemetry = 0; // No telemetry request for bidirectional dshot
int input_signal = 90; // Default start command
bool TX = true; // State 

//=============================================================================
// RX ISR variables
//=============================================================================

volatile unsigned long timeStamp = 0; //system clock captured in interrupt
volatile unsigned long lastTime = 0;
volatile int counter = 0;

//=============================================================================
// Dshot protocol object
//=============================================================================

Dshot dshot(PIN_NUM);

//=============================================================================
// Throttle command input via serial monitor
//=============================================================================

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

//=============================================================================
// RX ISR
//=============================================================================

void pulseduration_ISR() {
  
  if(counter == 0){
    timeStamp = ARM_DWT_CYCCNT; 
    lastTime = timeStamp;
    counter++;
  }else{
    lastTime = timeStamp; 
    timeStamp = ARM_DWT_CYCCNT; 
    dshot.set_timeRecord(counter-1, timeStamp - lastTime);
    counter++;
  }
  
}

//=============================================================================
// Setup
//=============================================================================

void setup() {
  pinMode(PIN_NUM, OUTPUT);
  Serial.begin(115200);

  // Start ARM cycle counter
  ARM_DEMCR |= ARM_DEMCR_TRCENA;  // debug exception monitor control register; enables trace and debug
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;

  Serial.print("Input command, "); Serial.println("Dshot RPM Sample");
  
}

//=============================================================================
// Loop
//=============================================================================

void loop() {
  
  if(TX){

    // If the user sent something through the terminal, update the throttle command
    input_signal = updateCommand(input_signal);
    dshot.set_throttle(input_signal, telemetry);
    // This would be replaced by a serial input ISR ^^

    // Construct & send the signal
    dshot.send_inv();

    // Setup for Rx
    TX = 0; counter = 0;
    dshot.start_Rx();
    pinMode(PIN_NUM, INPUT);
    attachInterrupt(PIN_NUM, pulseduration_ISR, CHANGE);
   
  }

  // This will continually loop while interrupts keep track of the signal change timestamps
  if(!TX){

    if(dshot.Rx_timeout()){

      detachInterrupt(PIN_NUM);

      // Decode the signal to RPM
      unsigned short RPM = dshot.decode_signal();
      if(RPM != 0xffff){ // decode_signal() returns 0xffff in the case of an error or no signal receive
        Serial.print(input_signal); Serial.print(", "); Serial.println(RPM);  
      }

      dshot.reset_array();
      pinMode(PIN_NUM, OUTPUT);
      TX = 1;
      
    }
  }
}
