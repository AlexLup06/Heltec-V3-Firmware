import serial
from pathlib import Path
import argparse

parser = argparse.ArgumentParser(description="Set serial port number")
parser.add_argument("serial_number", help="USB serial number (e.g., 0001)")
args = parser.parse_args()

SERIAL_PORT = f"/dev/cu.usbserial-{args.serial_number}"
print(f"Using serial port: {SERIAL_PORT}")

BAUD = 115200
output_root = Path("./data")          
output_root.mkdir(exist_ok=True)

ser = serial.Serial(SERIAL_PORT, BAUD, timeout=2)
print(f"📡 Listening on {SERIAL_PORT} at {BAUD} baud...")

current_file = None
expected_size = 0
received_bytes = 0

try:
    while True:
        line = ser.readline()
        if not line:
            continue
        line = line.decode(errors="ignore").strip()

        if line.startswith("BEGIN_FILE:"):
            try:
                _, path_str, size_str = line.split(":", 2)
                expected_size = int(size_str)
            except ValueError:
                print(f"⚠️  Invalid BEGIN_FILE line: {line}")
                continue

            rel_path = Path(path_str.lstrip("/"))
            local_path = output_root / rel_path

            local_path.parent.mkdir(parents=True, exist_ok=True)

            current_file = open(local_path, "wb")
            received_bytes = 0

            print(f"⬇️  Receiving {rel_path} ({expected_size} bytes)")

            while received_bytes < expected_size:
                chunk = ser.read(expected_size - received_bytes)
                if not chunk:
                    continue 
                current_file.write(chunk)
                received_bytes += len(chunk)

            current_file.close()
            current_file = None
            print(f"✅ Saved {rel_path} ({received_bytes}/{expected_size} bytes)\n")
            continue

        if line.startswith("END_FILE:"):
            continue

        if "ALL_DONE" in line:
            print("All files received successfully!")
            break

except KeyboardInterrupt:
    print("Stopped.")
finally:
    if current_file and not current_file.closed:
        current_file.close()
    ser.close()
    print("Serial.")
