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


class Dshot
{
public:
	Dshot(int pin);

	// TX Methods
	void set_throttle(int input, int telem);
	void send_inv(); // Send inverse (bidirectional) dshot signal
	
	// RX Methods
	void start_Rx();
	bool Rx_timeout();
	void reset_array();
	int decode_signal();
	int get_err_counter();
	int get_success_counter();

	// ISR Related
	void set_timeRecord(int i, int val);

private:
	int PIN_NUM;

	int input_signal;
	int telemetry;

	volatile int timeRecord[21];
	volatile int * array_pointer;
	unsigned long begin_timestamp;

	int CHECKSUM_ERR_COUNTER;
	int CHECKSUM_SUCCESS_COUNTER;

	// TX Helper functions for send_inv()
	void inv_on();
	void inv_off();
	void int2bin(int num,int * si);
	void crc2bin(int num,int * si);
};


#endif
