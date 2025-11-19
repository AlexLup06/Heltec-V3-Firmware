import struct
import json
from pathlib import Path

# --- Input and output roots ---
input_root = Path("./data")  # Where dumped .bin files are stored
output_root = Path("./data_json")  # Where parsed .json files will be saved
output_root.mkdir(parents=True, exist_ok=True)

# --- Define struct formats (Little Endian) ---
STRUCT_MAP = {
    "sent_effective_bytes": "<H",
    "sent_bytes": "<H",
    "received_effective_bytes": "<H",
    "received_bytes": "<H",
    "received_complete_mission": "<IBH",
    "sent_mission_rts": "<IBH",
    "sent_mission_fragment": "<IBH",
    "time_of_last_trajectory": "<H",
}

FIELDS = {
    "<H": ["value"],
    "<IBH": ["time", "source", "missionId"],
}


HEADER_FORMAT = "<IHHB32s32sHBHIB"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)


# --- Helper to parse header ---
def parse_header(data: bytes):
    (
        magic,
        version,
        entry_size,
        metric,
        networkName,
        currentMac,
        missionMessagesPerMin,
        numberOfNodes,
        runNumber,
        timestamp,
        nodeId,
    ) = struct.unpack(HEADER_FORMAT, data[:HEADER_SIZE])

    return {
        "magic": magic,
        "version": version,
        "entrySize": entry_size,
        "metric": metric,
        "networkName": networkName.decode("ascii", errors="ignore").rstrip("\x00"),
        "currentMac": currentMac.decode("ascii", errors="ignore").rstrip("\x00"),
        "missionMessagesPerMin": missionMessagesPerMin,
        "numberOfNodes": numberOfNodes,
        "runNumber": runNumber,
        "timestamp": timestamp,
        "nodeId": nodeId,
    }


# --- Process every .bin file recursively ---
for file in input_root.rglob("*.bin"):
    rel_path = file.relative_to(input_root)  # keep same relative path
    json_path = output_root / rel_path.with_suffix(".json")
    json_path.parent.mkdir(parents=True, exist_ok=True)

    data = file.read_bytes()
    if len(data) < HEADER_SIZE:
        print(f"⚠️ Skipping {file}: too small ({len(data)} bytes).")
        continue

    header = parse_header(data)
    entry_size = header["entrySize"]
    payload = data[HEADER_SIZE:]

    # --- Detect struct type based on filename ---
    name = file.stem.lower()
    struct_key = next((k for k in STRUCT_MAP if k in name), None)
    if not struct_key:
        print(f"⚠️ Unknown struct type for {file}, skipping.")
        continue

    fmt = STRUCT_MAP[struct_key]
    fields = FIELDS[fmt]
    vectors = {f: [] for f in fields}

    # --- Parse all entries ---
    for i in range(0, len(payload), entry_size):
        chunk = payload[i : i + entry_size]
        if len(chunk) < entry_size:
            break
        values = struct.unpack(fmt, chunk)
        for f, v in zip(fields, values):
            vectors[f].append(v)

    # --- Save parsed file ---
    json_data = {"metadata": header, "vectors": vectors}
    with open(json_path, "w") as f:
        json.dump(json_data, f, indent=2)

    print(f"✅ Parsed {file} → {json_path}")

print("🎉 All files parsed and saved to", output_root)
