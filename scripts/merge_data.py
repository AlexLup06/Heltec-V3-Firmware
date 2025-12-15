import json
import re
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Tuple

INPUT_ROOT = Path("./data_json")

# Example filename produced by LoggerManager:
# /<topology>/nodes-<n>/<metric>-<mac>-<topology>-<missionPerMin>-<n>-<nodeId>-vec-run<run>.bin
# We aggregate the JSON-converted files that follow the same naming convention.
FILENAME_RE = re.compile(
    r"(?P<metric>[^-]+)-(?P<mac>[^-]+)-(?P<topo>[^-]+)-(?P<mission>\d+)-(?P<nodes>\d+)-(?P<nodeId>\d+)-vec-run(?P<run>\d+)\.json",
    re.IGNORECASE,
)


def load_json(path: Path) -> Dict:
    with path.open("r") as f:
        return json.load(f)


def aggregate_group(files: List[Path]) -> Tuple[Dict, List[Dict]]:
    """Aggregate by collecting each node's results into a list."""
    metadata: Dict = {}
    per_node: List[Dict] = []

    for p in files:
        data = load_json(p)
        node_meta = data.get("metadata", {}) or {}
        if not metadata:
            metadata = dict(node_meta)
        node_entry: Dict = {"nodeId": node_meta.get("nodeId")}
        if "vectors" in data:
            node_entry["vectors"] = data["vectors"]
        if "counters" in data:
            node_entry["counters"] = data["counters"]
        per_node.append(node_entry)

    return metadata, per_node


def main():
    groups: Dict[Tuple[str, str, str, str, str, str], List[Path]] = defaultdict(list)

    for path in INPUT_ROOT.rglob("*.json"):
        m = FILENAME_RE.match(path.name)
        if not m:
            continue
        key = (
            m.group("metric"),
            m.group("mac"),
            m.group("topo"),
            m.group("mission"),
            m.group("nodes"),
            m.group("run"),
        )
        groups[key].append(path)

    for (metric, mac, topo, mission, nodes, run), files in groups.items():
        # Aggregate
        metadata, per_node_results = aggregate_group(files)

        # Collect nodeIds included
        node_ids = sorted({int(FILENAME_RE.match(p.name).group("nodeId")) for p in files})
        metadata = metadata or {}
        metadata["nodeId"] = "all"
        metadata["nodeIds"] = node_ids

        aggregated = {"metadata": metadata, "vectors": per_node_results}

        # Build output path, mirroring topology/nodes-N structure (stays in data_json)
        out_dir = INPUT_ROOT / topo / f"nodes-{nodes}"
        out_dir.mkdir(parents=True, exist_ok=True)
        out_name = f"{metric}-{mac}-{topo}-{mission}-{nodes}-all-vec-run{run}.json"
        out_path = out_dir / out_name

        with out_path.open("w") as f:
            json.dump(aggregated, f, indent=2)

        print(f"✅ Aggregated {len(files)} files → {out_path}")

        # Remove original per-node files to keep only the aggregated output
        for p in files:
            try:
                p.unlink()
            except FileNotFoundError:
                continue


if __name__ == "__main__":
    main()
