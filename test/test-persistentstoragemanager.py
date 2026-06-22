# Test script for PersistentStorageManager circular buffer functionality.

import serial

ser = serial.Serial("COM8", 1000000, timeout=1)

value = 0
buffer = ""
chunk_start = "====================="
chunk_end = "====================="

while True:
    msg = f"{value:03d}\r\n"
    ser.write(msg.encode())

    # read data with timeout
    line = ser.readline().decode(errors="ignore").strip()
    
    if line:
        buffer += line + "\n"
        
        # check for a complete chunk with start and end markers
        if chunk_start in buffer and chunk_end in buffer:
            # extract the chunk including markers
            start_idx = buffer.find(chunk_start)
            end_idx = buffer.find(chunk_end, start_idx + len(chunk_start))
            
            if end_idx != -1:
                # get the complete chunk
                full_chunk = buffer[start_idx:end_idx + len(chunk_end)]
                print(full_chunk)
                
                # remove processed chunk from buffer
                buffer = buffer[end_idx + len(chunk_end):]

    value = (value + 1) % 1000
