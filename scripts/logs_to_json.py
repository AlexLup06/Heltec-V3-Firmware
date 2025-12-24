import struct
import json
from pathlib import Path

input_root = Path("./data")
output_root = Path("./data_json")
output_root.mkdir(parents=True, exist_ok=True)

METRIC_DEFS = {
    "sent_mission_rts": ("<IBH", ["time", "source", "missionId"]),
    "received_complete_mission": ("<IBH", ["time", "source", "missionId"]),
    "received_mission_id_fragment": ("<HBB", ["missionId", "hopId", "sourceId"]),
    "received_neighbour_id_fragment": ("<HB", ["missionId", "sourceId"]),
    "received_effective_bytes": ("<H", ["bytes"]),
    "received_bytes": ("<H", ["bytes"]),
    "sent_effective_bytes": ("<H", ["bytes"]),
    "sent_bytes": ("<H", ["bytes"]),
}

METRIC_ENUM_TO_NAME = {
    0: "sent_mission_rts",
    1: "received_complete_mission",
    2: "received_mission_id_fragment",
    3: "received_neighbour_id_fragment",
    4: "received_effective_bytes",
    5: "received_bytes",
    6: "sent_effective_bytes",
    7: "sent_bytes",
    8: "collisions",
    9: "time_on_air",
    10: "single_values",
}


HEADER_FORMAT = "<IHHB32s32sHBHIB"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

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
        "networkName": networkName.split(b"\x00", 1)[0].decode(
            "ascii", errors="ignore"
        ),
        "currentMac": currentMac.split(b"\x00", 1)[0].decode("ascii", errors="ignore"),
        "missionMessagesPerMin": missionMessagesPerMin,
        "numberOfNodes": numberOfNodes,
        "runNumber": runNumber,
        "timestamp": timestamp,
        "nodeId": nodeId,
    }


for file in input_root.rglob("*.bin"):
    rel_path = file.relative_to(input_root)
    json_path = output_root / rel_path.with_suffix(".json")
    json_path.parent.mkdir(parents=True, exist_ok=True)

    data = file.read_bytes()
    if len(data) < HEADER_SIZE:
        print(f"⚠️ Skipping {file}: too small ({len(data)} bytes).")
        continue

    header = parse_header(data)
    entry_size = header["entrySize"]
    payload = data[HEADER_SIZE:]

    name = file.stem.lower()
    struct_key = next((k for k in METRIC_DEFS if k in name), None)
    if not struct_key:
        struct_key = METRIC_ENUM_TO_NAME.get(header["metric"])
    if not struct_key:
        print(
            f"⚠️ Unknown struct type for {file} (metric={header['metric']}), skipping."
        )
        continue

    if struct_key == "single_values":
        text = payload.decode("ascii", errors="ignore")
        counters = {}
        for line in text.splitlines():
            if "=" not in line:
                continue
            name, value = line.split("=", 1)
            try:
                counters[name.strip()] = float(value.strip())
            except ValueError:
                continue
        json_data = {"metadata": header, "counters": counters}
    else:
        metric_def = METRIC_DEFS.get(struct_key)
        if metric_def is None:
            print(
                f"⚠️ No struct definition for {struct_key} in {file} (metric={header['metric']}), skipping."
            )
            continue

        fmt, fields = metric_def
        fmt_size = struct.calcsize(fmt)
        if entry_size != fmt_size:
            print(
                f"Entry size mismatch for {file}: header={entry_size}, expected={fmt_size} for {struct_key}"
            )

        vectors = {f: [] for f in fields}

        for i in range(0, len(payload), entry_size):
            chunk = payload[i : i + entry_size]
            if len(chunk) < entry_size:
                break
            values = struct.unpack(fmt, chunk)
            for f, v in zip(fields, values):
                vectors[f].append(v)

        json_data = {"metadata": header, "vectors": vectors}
    with open(json_path, "w") as f:
        json.dump(json_data, f, indent=2)

    print(f"✅ Parsed {file} → {json_path}")

print("All files parsed and saved to", output_root)
