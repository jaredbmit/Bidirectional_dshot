/**
 * @file bidirectional_dshot_v4.ino
 * @brief Dshot communication example w/ multiple ESCs using Dshot and DshotManager classes
 * @author Jared Boyer <jaredb@mit.edu>
 * @date 04 Jul 2021
 */

#include "dshot_manager.h"
#include "snapio-protocol/sio_serial.h"

//=============================================================================
// pin definitions
//=============================================================================

std::vector<int> PIN_NUMS = {1,2,3,4}; // = {2,3,4,5,6,7}; // for 6 motors

//=============================================================================
// configuration options
//=============================================================================

static constexpr int BAUD = 921600; // baud for snap <-> snapio comms

//=============================================================================
// global variables
//=============================================================================

// serial stuff
sio_serial_message_t msg_buf;

// 
unsigned short RPM;
int input_signal = 90; // Default input signal

//=============================================================================
// ESC comms init
//=============================================================================

DshotManager manager(PIN_NUMS);

//=============================================================================
// setup
//=============================================================================

void setup(){
  //Serial1.begin(BAUD);
  Serial.begin(115200); // Serial monitor comms for now

}

//=============================================================================
// loop
//=============================================================================

void loop(){

  input_signal = updateCommand(input_signal);
  for(int i=0; i<manager.get_num_esc(); i++){
    manager.set_throttle_esc(i, input_signal);
  }

  if(manager.Tx_state()){
    manager.send_sync();
  }
  // Where does sending the signal go? Still need to answer the question of how to
  // synchronize send/receive.
  /* The snapdragon likely won't send signals too fast such that we interrupt Rx process,
  * also we can just make an IDLE flag so that it wouldn't send anyways, but what if the 
  * flight computer sends signals too slow? Hard to make it event based if the events don't
  * line up with the very synchronized process that is dshot
  */

  for(int i=0; i<manager.get_num_esc(); i++){
    if(manager.esc_ready(i)){
      RPM = manager.decode_esc(i); // When do we initialize variables in loop?
      Serial.print("Motor @ pin # "); Serial.print(PIN_NUMS[i]); Serial.print(": "); Serial.println(RPM); // To be replaced with Serial1.write()
    }
  } 

}

//=============================================================================
// temporary comms helper function
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


// //=============================================================================
// // handle received serial data
// //=============================================================================

// // What kind of data is being sent over serial? Will it be throttle command? 
// // Percentage throttle? How do we decode this data any differently than PWM?
// // How do we account for dividing data up based on number of ESC's? How do we 
// // have to change sio_serial.h to accomodate non-PWM values?

// void serialEvent1()
// {
//   while (Serial1.available()) {
//     uint8_t in_byte = (uint8_t) Serial1.read();
//     if (sio_serial_parse_byte(in_byte, &msg_buf)) {
//       switch (msg_buf.type) {
//         case SIO_SERIAL_MSG_MOTORCMD:
//         {
//           sio_serial_motorcmd_msg_t msg;
//           sio_serial_motorcmd_msg_unpack(&msg, &msg_buf);
//           handle_motorcmd_msg(msg);
//           break;
//         }
//       }
//     }
//   }
// }

// //=============================================================================
// // handle received messages
// //=============================================================================

// void handle_motorcmd_msg(const sio_serial_motorcmd_msg_t& msg)
// {
//     // Old... // analogWrite(PWM_PINS[i], usec2dutycycle(msg.usec[i]));
//     for(int i=0; i<manager.get_num_esc(); i++){
//      manager.set_throttle_esc(i, msg.something /* Something goes here */);
//     }
//     
// }
