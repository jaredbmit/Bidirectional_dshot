/**
 * @file bidirectional_dshot_v6.ino
 * @brief Example dshot600 communication scheme, serial comm with flight computer emulated from arduino serial monitor
 * @author Jared Boyer <jaredb@mit.edu>
 * @date 14 Apr 2021
 */

/* Usage: send motor command as an integer through arduino serial monitor.
 * To arm the ESC, send a low motor command (100 ish), wait for beep, then 
 * send a 48 command, wait for second beep, then the motor is armed.
 */

#include "bidirectional_dshot.h"

//=============================================================================
// General configuration options
//=============================================================================

static constexpr int NUM_ESC = 6; // number of ESCs / output pins

//=============================================================================
// DSHOT configuration options
//=============================================================================

/* Pin definitions are found in bidirectional_dshot.cpp (also found in README).
Pin definitions can be changed using the IMXRT manual to find the necessary
eFlexPWM module configuration for each pin (more info found in README) */

// Desired frequency for sending motor feedback (Hz)
static constexpr int FEEDBACK_FREQ = 1000; 
static constexpr int COMM_PERIOD = 1000000 / FEEDBACK_FREQ; // in microseconds

// ESC comms init
DshotManager manager;

//=============================================================================
// global variables
//=============================================================================

uint16_t input_signal;

uint32_t last_feedback_time = 0;

uint32_t eRPM;

//=============================================================================
// initialize
//=============================================================================

void setup()
{
  Serial.begin(115200); // Serial monitor to emulate snapdragon interaction

  for (int i = 0; i < NUM_ESC; i++) {
    manager.set_throttle_esc(i, 48);
  }
  manager.start_tx(); // Startup IntervalTimer to trigger tx continuously
}

//=============================================================================
// loop
//=============================================================================

void loop()
{
  updateCommand();

  if ((micros() - last_feedback_time) > COMM_PERIOD) {
    last_feedback_time = micros();
    for (int i = 0; i < NUM_ESC; i++) {
      eRPM = manager.average_eRPM(i);
      if (eRPM != 0xffffffff) { // Have any DSHOT packets been received from ESC?
        Serial.print("Motor "); Serial.print(i); Serial.print(": "); Serial.println(eRPM);
      }
    }
  }
}

//=============================================================================
// temporary comms helper function
//=============================================================================

void updateCommand()
{
  // Get user input command
  if (Serial.available()) {
    char new_input[4];
    int n = 0;
    while (Serial.available()) {
      new_input[n] = Serial.read();
      n++;
    }
    input_signal = atoi(new_input);
    for (int i=0; i < NUM_ESC; i++) {
      manager.set_throttle_esc(i, input_signal);
    }
  }
}
