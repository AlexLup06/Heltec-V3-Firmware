import serial
import re
from pathlib import Path

# Adjust this to your ESP32 serial port
# On macOS it's usually something like /dev/cu.usbserial-* or /dev/cu.SLAB_USBtoUART
SERIAL_PORT = "/dev/cu.usbserial-0001"
BAUD = 115200

output_dir = Path("esp32_logs")
output_dir.mkdir(exist_ok=True)

# Open serial connection
ser = serial.Serial(SERIAL_PORT, BAUD, timeout=2)
print(f"📡 Listening on {SERIAL_PORT}...")

current_file = None
expected_size = 0
received_bytes = 0

try:
    while True:
        line = ser.readline()
        if not line:
            continue
        line = line.decode(errors="ignore").strip()

        # Start of new file
        if line.startswith("BEGIN_FILE:"):
            _, name, size = line.split(":")
            expected_size = int(size)
            current_file = open(output_dir / Path(name).name, "wb")
            received_bytes = 0
            print(f"⬇️  Receiving {name} ({expected_size} bytes)")
            continue

        # End of file marker
        if line.startswith("END_FILE:"):
            if current_file:
                current_file.close()
                print(f"✅ Saved file ({received_bytes}/{expected_size} bytes)\n")
                current_file = None
            continue

        # End of all files
        if "ALL_DONE" in line:
            print("🎉 All files received!")
            break

        # Raw binary data (no "BEGIN_FILE" or "END_FILE" line)
        if current_file:
            # In this simple protocol, binary data is sent as raw bytes,
            # not through readline(), so check in buffer
            data = ser.read(expected_size - received_bytes)
            if data:
                current_file.write(data)
                received_bytes += len(data)
                if received_bytes >= expected_size:
                    current_file.close()
                    print(f"✅ File complete ({received_bytes} bytes)\n")
                    current_file = None
            continue

except KeyboardInterrupt:
    print("🛑 Stopped by user.")
finally:
    ser.close()
