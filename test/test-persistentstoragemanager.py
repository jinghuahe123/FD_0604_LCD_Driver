import serial
import time

ser = serial.Serial("COM8", 1000000, timeout=1)

value = 0

while True:
    msg = f"{value:03d}\r\n"
    ser.write(msg.encode())

    line = ser.readline().decode(errors="ignore").strip()
    if line:
        print("RX:", line)

    value = (value + 1) % 1000
    time.sleep(0.015)
