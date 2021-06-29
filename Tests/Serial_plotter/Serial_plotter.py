# -*- coding: utf-8 -*-
import serial
import csv

def setpriority(pid=None,priority=3):
    """ Set The Priority of a Windows Process.  Priority is a value between 0-5 where
        2 is normal priority.  Default sets the priority of the current
        python process but can take any valid process ID. """
        
    import win32api,win32process,win32con
    
    priorityclasses = [win32process.IDLE_PRIORITY_CLASS,
                       win32process.BELOW_NORMAL_PRIORITY_CLASS,
                       win32process.NORMAL_PRIORITY_CLASS,
                       win32process.ABOVE_NORMAL_PRIORITY_CLASS,
                       win32process.HIGH_PRIORITY_CLASS,
                       win32process.REALTIME_PRIORITY_CLASS]
    if pid == None:
        pid = win32api.GetCurrentProcessId()
    handle = win32api.OpenProcess(win32con.PROCESS_ALL_ACCESS, True, pid)
    win32process.SetPriorityClass(handle, priorityclasses[priority])
    
setpriority()

ser = serial.Serial('COM5')
ser.flushInput()
with open("test_data_pre1.csv","a") as f:
    writer = csv.writer(f,delimiter=",")

    while True:
        try:
            ser_bytes = ser.readline()
#            decoded_bytes = (ser_bytes[0:len(ser_bytes)-2].decode("utf-8")).split(", ")
#            print(decoded_bytes)
#            if len(decoded_bytes) == 3:
#               writer.writerow([decoded_bytes[0],decoded_bytes[1],decoded_bytes[2]])
            writer.writerow([ser_bytes])
        except KeyboardInterrupt:
            ser.close()
            print('Interrupted')
        except UnicodeDecodeError:
            print('Unicode error')