/**
 * @file bidirectional_dshot.h
 * @brief Bidirectional dshot communication protocol for teensy 4.0
 * @author Jared Boyer <jaredb@mit.edu>
 * @date 29 Jun 2021
 */

#ifndef BI_DSHOT_H_
#define BI_DSHOT_H_


#include <stdlib.h>
#include <math.h>
#include <TimeLib.h>
#include <cmath>
#include <stdint.h>
#include <Arduino.h>
#include <list>


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

// dshot600 synchronous signal sending pieces (this is not a great way of doing it)
#define BEGIN_WAIT __asm__(NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP50) //0625ns
#define MIDDLE_WAIT __asm__(NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP100 NOP50) //0625ns
#define END_WAIT __asm__(NOP100 NOP100 NOP100 NOP100 NOP100) //416.6ns


class Dshot
{
public:
	Dshot(int pin, void(*isr)(void));

	// TX Methods
	void set_throttle(unsigned short input, unsigned short telem);

	// Synchronous TX Methods
	void set_high();
	void set_low();
	
	// RX Methods
	int decode_signal();
	int get_err_counter();
	int get_success_counter();
	bool ready_for_decoding();
  bool ready_for_tx();
  void tx_flag_off();
  void tx_flag_on();
	void decode_flag_off();
	void decode_flag_on();
	void begin_Rx();

	// Signal
	bool dshot_signal[16];

	// ISR Related
	void set_timeRecord(int i, int val);
	volatile unsigned long timeStamp;
	volatile unsigned long lastTime;
	volatile int ISR_counter;

  int PIN_NUM;

private:
	void(*ISR_FN)(void);

  bool tx;

	volatile int timeRecord[21] = {0};
	volatile bool rx_done;

	int CHECKSUM_ERR_COUNTER;
	int CHECKSUM_SUCCESS_COUNTER;

	// RX Helper functions for decode_signal();
	void reset_array();
	
};


#endif /* BI_DSHOT_H_ */
