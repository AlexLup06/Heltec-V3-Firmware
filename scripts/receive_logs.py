import serial
from pathlib import Path

# --- CONFIG ---
SERIAL_PORT = "/dev/cu.usbserial-0001"  # your port
BAUD = 115200
output_root = Path("./data")          # where to mirror LittleFS
output_root.mkdir(exist_ok=True)

# --- OPEN SERIAL PORT ---
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

        # --- BEGIN_FILE:/mesh/nodes-1/filename.bin:512 ---
        if line.startswith("BEGIN_FILE:"):
            try:
                _, path_str, size_str = line.split(":", 2)
                expected_size = int(size_str)
            except ValueError:
                print(f"⚠️  Invalid BEGIN_FILE line: {line}")
                continue

            # Remove leading slash from device path
            rel_path = Path(path_str.lstrip("/"))
            local_path = output_root / rel_path

            # Create parent directories
            local_path.parent.mkdir(parents=True, exist_ok=True)

            current_file = open(local_path, "wb")
            received_bytes = 0

            print(f"⬇️  Receiving {rel_path} ({expected_size} bytes)")

            # Read the raw binary data
            while received_bytes < expected_size:
                chunk = ser.read(expected_size - received_bytes)
                if not chunk:
                    continue  # wait for more data
                current_file.write(chunk)
                received_bytes += len(chunk)

            current_file.close()
            current_file = None
            print(f"✅ Saved {rel_path} ({received_bytes}/{expected_size} bytes)\n")
            continue

        # --- END_FILE (optional marker, ignored) ---
        if line.startswith("END_FILE:"):
            continue

        # --- ALL_DONE ---
        if "ALL_DONE" in line:
            print("🎉 All files received successfully!")
            break

except KeyboardInterrupt:
    print("🛑 Stopped by user.")
finally:
    if current_file and not current_file.closed:
        current_file.close()
    ser.close()
    print("🔌 Serial closed.")
