/**
 * @file dshot_manager.h
 * @brief DMA-enabled bidirectional dshot communication protocol manager class for multiple ESCs. Adapted from teensyshot.
 * @author Jared Boyer <jaredb@mit.edu>
 * @date 20 Jul 2021
 */

#ifndef DSHOT_MANAGER_H_
#define DSHOT_MANAGER_H_

#include "DMAChannel.h"
#include <Arduino.h>

#define DSHOT_DMA_LENGTH          18            // Number of steps of one DMA sequence (the two last values are zero)
#define DSHOT_DMA_MARGIN          2             // Number of additional bit duration to wait until checking if DMA is over
#define DSHOT_DSHOT_LENGTH        16            // Number of bits in a DSHOT sequence
#define DSHOT_BT_DURATION         1670          // Duration of 1 DSHOT600 bit in ns
#define DSHOT_LP_DURATION         1250          // Duration of a DSHOT600 long pulse in ns
#define DSHOT_SP_DURATION         625           // Duration of a DSHOT600 short pulse in ns
#define MAX_ESC                   6             // Supports up to 6 ESCs
#define TX_WAIT                   115           // Interval in microseconds at which dshot signals are sent
#define RX_WAIT                   65            // Interval in microseconds to wait for incoming receive signal
#define RX_SIGNAL_LENGTH          21            // Number of bits in the dshot feedback received signal
#define RX_BIT_TICK_LENGTH        800           // Received signal bit pulse length in number of ARM cycle ticks (800 ticks = 1.33 us)

class DshotManager
{
public:
	DshotManager(uint8_t n);
	void start_tx();
	void set_throttle_esc(int i, uint16_t input);
	bool ready_for_decoding();
	uint16_t decode_signal(int i);

private:
	void reset_array(int i);
};

void DMA_init(int i);
void tx_ISR();
void rx_ISR();
void assemble_signal_esc(int i);

#endif /* DSHOT_MANAGER_H_ */
