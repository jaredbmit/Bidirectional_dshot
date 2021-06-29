### Working implementation of communicating to BL_Heli32 ESC from teensy 4.0 with bidirectional dshot

To arm the ESC, send a low throttle command (~100), wait for a motor beep, then send a 0 command (can be 0 or 48). After the next beep the ESC will be armed.

#### Scripts/ 
- Contains an example of communicating bidirectionally with ESC (use latest version)
- Throttle commands are sent via arduino serial monitor (after the ESC is manually armed)
- Incoming dshot feedback data is collected using ARM cycle counter to measure pulse widths

#### Tests/ 
- These scripts are designed for use on the ACL optical encoder test stand
- Contains a script to sweep through the dshot command range (70 to 2000) and output feedback data from both dshot and the test stand optical encoder
- Test is initiated after manually arming the ESC and sending a 70 command from the serial monitor
- Dshot feedback is averaged down from ~9000 to ~1000 Hz
- Tests/Plotter/ contains a python script to read incoming UART data and writes to csv in real time
- In order to use the python plotter, start the test from the serial monitor, close out of the monitor, then run Serial_plotter.py
- Afterwards, run Postprocessor.py. The resulting file will contain throttle command, dshot RPM feedback, and optical encoder RPM feedback.
