# Communicating to BL_Heli32 ESC from teensy 4.0 with DMA-enabled bidirectional dshot600
- This repo serves as a workspace for testing dshot600 communication schemes for later integration into the [MIT ACL Flight stack](https://gitlab.com/mit-acl/fsw/snap-stack/snapio/ioboard).
- Currently the implementation lives on the [teensy 4 branch](https://gitlab.com/mit-acl/fsw/snap-stack/snapio/ioboard/-/tree/teensy) of the I/O firmware repository. 

## Operation notes
To arm the ESC, send a low throttle command (~100), wait for a motor beep, then send a 0 command (can be 0 or 48). After the next beep the ESC will be armed.

## File description

### Scripts/ 
- Contains examples of communicating bidirectionally (use latest version-- outdated versions are kept for reference in Scripts/old/)
- Throttle commands are emulated via arduino serial monitor (in the actual flight stack, throttle commands are received externally)
- Outgoing signals are generated via DMA-monitored PWM pulse generation
- Incoming dshot feedback packets are measured using the ARM cycle counter & pin change interrupts
- Pin numbers & configuration can be found in bidirectional_dshot.cpp, but pin configs can be changed with a little bit of work (see wiring section for a tutorial)

### Tests/ 
- These scripts are designed for use on the ACL optical encoder test stand
- Contains a script to sweep through the dshot command range (70 to 2000) and output feedback data from both dshot and the test stand optical encoder
- Test is initiated after manually arming the ESC and sending a 70 command (arbitrarily chosen) from the serial monitor
- Dshot feedback is averaged down from ~9000 to ~1000 Hz
- Tests/Plotter/ contains a python script to read incoming UART data and writes to csv in real time
- In order to use the python plotter, start the test from the serial monitor, close out of the monitor, then run Serial_plotter.py (only one program can access the UART line at a time)
- Afterwards, run Postprocessor.py. The resulting file will contain throttle command, dshot RPM feedback, and optical encoder RPM feedback.

## Wiring
Currently configured to communicate to a max of 6 ESCs. Pin numbers can be changed following the teensy 4.0 IMXRT manual's muxing options (tutorial below).

| ESC # | Pin # | Pinmux | eFlexPWM Module | Submodule | Channel |
| ----- | ----- | ------ | --------------- | --------- | ------- |
| 1     | 4     | 1      | 2               | 0         | A       |
| 2     | 8     | 6      | 1               | 3         | A       |
| 3     | 2     | 1      | 4               | 2         | A       |
| 4     | 22    | 1      | 4               | 0         | A       |
| 5     | 23    | 1      | 4               | 1         | A       |
| 6     | 9     | 2      | 2               | 2         | B       |

### Changing pin config
Pin configurations (pin number, corresponding PWM modules, pin muxing options) are found in firmware/bidirectional_dshot.cpp. All of these arrays have pin-specific info, so if a pin number is changed, the corresponding values in all of the other config arrays must also be changed. The values for these config arrays can all be found based on what pin number is chosen.

<img src="https://user-images.githubusercontent.com/78260876/126682916-42551243-1609-4438-bf8c-dbc0c05ea4c9.PNG" width="500">

The first step is to use the following teensy 4.0 pin card to find which pad corresponds to your desired pin (under the "Native" column). 

<img src="https://user-images.githubusercontent.com/78260876/126682909-87c9f5cc-6526-41dd-9080-908390506af9.PNG" width="500">

Then, navigate to the [IMXRT1060 manual](https://www.pjrc.com/teensy/IMXRT1060RM_rev2.pdf) and find the IOMUX section (Chapter 10, page 293).

<img src="https://user-images.githubusercontent.com/78260876/126682896-443ae6cd-1d33-40a0-bee1-ca6b92ef0f1b.PNG" width="500">

Ctrl+f the desired pad name in this table and note which eFlexPWM module, submodule, channel, and pin mux it corresponds to (there may be multiple choices). For example, if pin 4 is desired, which corresponds to pad GPIO_EMC_06, the following info is found in the IOMUX table:

<img src="https://user-images.githubusercontent.com/78260876/126682890-e68107ed-8f42-40da-91f6-3ac14a2e2291.PNG" width="500">

This means pin 4 uses eFlexPWM module 2, submodule 0, channel A, and pin mux ALT1. Place these values at the desired index of all the config arrays back in firmware/bidirectional_dshot.cpp (Index 0 = ESC 1, index 1 = ESC 2...). The last config option that has not been accounted for is the dmamux array, which contains info on DMA write requests. The DMA write request addresses can be found in the teensy 4 cores library, but it's easier to just follow the pattern: DMAMUX_SOURCE_FLEXPWM-_WRITE-, where the first - is replaced with the module number and the second - is replaced with the submodule number.

**Note: Some pin choices result in the use of PWM channel X, which is the auxiliary channel (only exists for PWM module 1). The usage of these channels has not been tested and may not work with the current PWM module configuration that happens in firmware/bidirectional_dshot.cpp**
