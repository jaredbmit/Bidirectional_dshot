/**
 * @file dshot_manager.h
 * @brief Bidirectional dshot communication protocol manager class for multiple ESCs
 * @author Jared Boyer <jaredb@mit.edu>
 * @date 07 Jul 2021
 */

#ifndef DSHOT_MANAGER_H_
#define DSHOT_MANAGER_H_

#include <vector>
#include "bidirectional_dshot.h"

class DshotManager
{
public:
	DshotManager(std::vector<int> pins);

	bool esc_ready(int i);
	int decode_esc(int i);
	void set_throttle_esc(int i, int input);
	void send_sync();

	// Getters & setters
	int get_num_esc();
	bool Tx_state();

private:
	std::vector<Dshot> dshot_;
	int NUM_ESC; // Should this be static? 
};

void rx_timer_ISR();

// External ISR for measuring Rx pulse widths
void ISR_0();
void ISR_1();
void ISR_2();
void ISR_3();
void ISR_4();
void ISR_5();
void ISR_6();
void ISR_7();

#endif /* DSHOT_MANAGER_H_ */
