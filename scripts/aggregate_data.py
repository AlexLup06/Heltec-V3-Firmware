import json
import re
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Tuple
import math

SIMULATION_DURATION_S = 300

INPUT_ROOT = Path("data_json")
OUTPUT_ROOT = Path("data_aggregated")

MERGED_RE = re.compile(
    r"(?P<metric>[^-]+)-(?P<protocol>[^-]+)-(?P<topology>[^-]+)-(?P<mission>\d+)-(?P<nodes>\d+)-all-vec-run(?P<run>\d+)\.json",
    re.IGNORECASE,
)


def load_json(path: Path) -> Dict:
    with path.open("r") as f:
        return json.load(f)


def collect_inputs() -> Dict[Tuple[str, str, str, str], Dict[str, List[Path]]]:
    """Group merged files by (protocol, topology, mission, nodes)."""
    groups: Dict[Tuple[str, str, str, str], Dict[str, List[Path]]] = defaultdict(lambda: defaultdict(list))
    for path in INPUT_ROOT.rglob("*-all-vec-*.json"):
        m = MERGED_RE.match(path.name)
        if not m:
            continue
        metric = m.group("metric")
        key = (m.group("protocol"), m.group("topology"), m.group("mission"), m.group("nodes"))
        groups[key][metric].append(path)
    return groups


def aggregate_counters(files: List[Path]) -> Dict[str, float]:
    """Collect counters from merged counter files (per node)."""
    values: Dict[str, List[float]] = {
        "time_on_air": [],
        "collisions": [],
        "received_bytes": [],
        "received_effective_bytes": [],
        "sent_bytes": [],
        "sent_effective_bytes": [],
    }

    for path in files:
        data = load_json(path)
        for node_entry in data.get("vectors", []):
            counters = node_entry.get("counters", {}) or {}
            for key in values.keys():
                if key in counters:
                    try:
                        values[key].append(float(counters[key]))
                    except (TypeError, ValueError):
                        continue

    return {
        "time_on_air_values": values["time_on_air"],
        "collisions_total": sum(values["collisions"]),
        "received_bytes_total": sum(values["received_bytes"]),
        "received_effective_bytes_total": sum(values["received_effective_bytes"]),
        "sent_bytes_total": sum(values["sent_bytes"]),
        "sent_effective_bytes_total": sum(values["sent_effective_bytes"]),
    }


def compute_stats(values: List[float]) -> Dict[str, float]:
    if not values:
        return {"count": 0, "mean": None, "std": None, "ci95": [None, None]}
    n = len(values)
    mean = sum(values) / n
    if n > 1:
        variance = sum((v - mean) ** 2 for v in values) / (n - 1)
        std = math.sqrt(variance)
        margin = 1.96 * (std / math.sqrt(n))
    else:
        std = 0.0
        margin = 0.0
    return {
        "count": n,
        "mean": mean,
        "std": std,
        "ci95": [mean - margin, mean + margin],
    }


def summarize_values(val) -> Dict:
    if isinstance(val, dict) and ("mean" in val or "ci95" in val or "std" in val or "count" in val):
        cleaned = {k: v for k, v in val.items() if v is not None}
        if "ci95" in cleaned and (cleaned["ci95"] is None or any(v is None for v in cleaned["ci95"])):
            cleaned.pop("ci95", None)
        if cleaned.get("std") is None:
            cleaned.pop("std", None)
        cleaned.pop("count", None)
        return cleaned

    if isinstance(val, dict) and not ("values" in val):
        return {k: summarize_values(v) for k, v in val.items()}

    if isinstance(val, dict) and "values" in val:
        base = list(val["values"])
    elif isinstance(val, list):
        base = list(val)
    else:
        base = [val]

    base = [b for b in base if b is not None]
    count = len(base)
    if count == 0:
        return {}
    mean = sum(base) / count
    result = {"count": count, "mean": mean}
    if count > 1:
        variance = sum((b - mean) ** 2 for b in base) / (count - 1)
        std = math.sqrt(variance)
        margin = 1.96 * (std / math.sqrt(count))
        result["std"] = std
        result["ci95"] = [mean - margin, mean + margin]
    return result


def compute_jain(values: List[float]) -> float:
    if not values:
        return None
    s = sum(values)
    s2 = sum(v * v for v in values)
    n = len(values)
    if s2 == 0:
        return None
    return (s * s) / (n * s2)


COMPLEX_NEIGHBOUR_MAP = {
    32: 4,
    92: 2,
    4: 3,
    100: 2,
    52: 3,
    104: 2,
    248: None,
    16: 2,
}


def expected_neighbours(topology: str, source: int, num_nodes: int) -> int:
    if topology == "TOPOLOGY_FULLY_MESHED":
        return max(num_nodes - 1, 1)
    if topology == "TOPOLOGY_COMPLEX":
        val = COMPLEX_NEIGHBOUR_MAP.get(source)
        if val:
            return val
    return max(num_nodes - 1, 1)


def aggregate_node_reachability(sent_files: List[Path], received_files: List[Path], num_nodes: int) -> Dict[str, Dict]:
    """Compute reachability ratios (from received only) and propagation times (if fully received)."""
    earliest_sent: Dict[Tuple[int, int], float] = {}  # (missionId, source) -> time
    received_map: Dict[Tuple[int, int], List[Tuple[int, float]]] = defaultdict(list)
    present_nodes: set = set()

    for path in sent_files:
        data = load_json(path)
        for node_entry in data.get("vectors", []):
            vec = node_entry.get("vectors", {}) or {}
            times = vec.get("time", []) or []
            sources = vec.get("source", []) or []
            missions = vec.get("missionId", []) or []
            for t, s, m in zip(times, sources, missions):
                key = (int(m), int(s))
                if key not in earliest_sent or t < earliest_sent[key]:
                    earliest_sent[key] = t

    for path in received_files:
        data = load_json(path)
        for node_entry in data.get("vectors", []):
            vec = node_entry.get("vectors", {}) or {}
            times = vec.get("time", []) or []
            sources = vec.get("source", []) or []
            missions = vec.get("missionId", []) or []
            node_id = node_entry.get("nodeId")
            for t, s, m in zip(times, sources, missions):
                key = (int(m), int(s))
                received_map[key].append((int(node_id) if node_id is not None else None, t))
            if node_id is not None:
                present_nodes.add(int(node_id))

    ratios: List[float] = []
    propagation_times: List[float] = []

    denom = max(len(present_nodes) - 1, 1)
    for key, recv_list in received_map.items():
        recv_nodes = {n for n, _t in recv_list if n is not None}
        ratio = len(recv_nodes) / denom
        ratios.append(ratio)
        if ratio >= 1.0 and recv_list and key in earliest_sent:
            propagation_times.append(max(t for _n, t in recv_list) - earliest_sent[key])

    return {
        "ratio": compute_stats(ratios),
        "propagation_time": compute_stats(propagation_times),
    }


def aggregate_reception_success(files: List[Path], topology: str, num_nodes: int, is_mission: bool) -> Dict[str, float]:
    """Compute reception success ratio for fragments keyed by (missionId, sourceId, hopId)."""
    triplet_to_nodes: Dict[Tuple[int, int, int], set] = defaultdict(set)
    for path in files:
        data = load_json(path)
        for node_entry in data.get("vectors", []):
            node_id = node_entry.get("nodeId")
            vec = node_entry.get("vectors", {}) or {}
            missions = vec.get("missionId", []) or []
            sources = vec.get("sourceId", []) or []
            hops = vec.get("hopId", []) or []
            for idx, (m, s, h) in enumerate(zip(missions, sources, hops)):
                key = (int(m), int(s), int(h) if h is not None else 0)
                triplet_to_nodes[key].add(node_id)
    ratios: List[float] = []
    for (mid, src, hop), nodes_set in triplet_to_nodes.items():
        denom = expected_neighbours(topology, src, num_nodes)
        ratios.append(len(nodes_set) / denom if denom > 0 else 0.0)
    return ratios


def write_output(protocol: str, topology: str, metric_name: str, data_entries: List[Dict]):
    missions = sorted({entry["timeToNextMission"] for entry in data_entries if "timeToNextMission" in entry})
    nodes = sorted({entry["numberNodes"] for entry in data_entries if "numberNodes" in entry})

    topo_clean = topology.replace("TOPOLOGY_", "")
    metric_dir = OUTPUT_ROOT / metric_name
    metric_dir.mkdir(parents=True, exist_ok=True)
    out_path = metric_dir / f"{protocol}_{topo_clean}_{metric_name}.json"

    payload = {
        "metadata": {
            "macProtocol": protocol,
            "topology": topo_clean,
            "timeToNextMission": missions,
            "numberNodes": nodes,
        },
        "data": [],
    }

    normalized_entries = []
    for entry in data_entries:
        meta = {}
        if "timeToNextMission" in entry:
            meta["timeToNextMission"] = entry["timeToNextMission"]
        if "numberNodes" in entry:
            meta["numberNodes"] = entry["numberNodes"]
        stats = summarize_values(entry.get("data"))
        normalized_entries.append({"metadata": meta, "data": stats})

    payload["data"] = normalized_entries

    out_path.write_text(json.dumps(payload, indent=2))
    print(f"✅ Wrote {out_path}")


def write_time_on_air(protocol: str, topology: str, data_entries: List[Dict]):
    """Write Jain's fairness index for time-on-air (no std/ci95)."""
    missions = sorted({entry["timeToNextMission"] for entry in data_entries if "timeToNextMission" in entry})
    nodes = sorted({entry["numberNodes"] for entry in data_entries if "numberNodes" in entry})

    topo_clean = topology.replace("TOPOLOGY_", "")
    metric_dir = OUTPUT_ROOT / "time-on-air"
    metric_dir.mkdir(parents=True, exist_ok=True)
    out_path = metric_dir / f"{protocol}_{topo_clean}_time-on-air.json"

    payload = {
        "metadata": {
            "macProtocol": protocol,
            "topology": topo_clean,
            "timeToNextMission": missions,
            "numberNodes": nodes,
        },
        "data": [],
    }

    normalized_entries = []
    for entry in data_entries:
        meta = {}
        if "timeToNextMission" in entry:
            meta["timeToNextMission"] = entry["timeToNextMission"]
        if "numberNodes" in entry:
            meta["numberNodes"] = entry["numberNodes"]
        normalized_entries.append({"metadata": meta, "data": {"mean": entry.get("data")}})

    payload["data"] = normalized_entries

    out_path.write_text(json.dumps(payload, indent=2))
    print(f"✅ Wrote {out_path}")


def write_output_propagation(protocol: str, topology: str, data_entries: List[Dict]):
    """Write propagation_time stats, preserving count/std/ci95."""
    missions = sorted({entry["timeToNextMission"] for entry in data_entries if "timeToNextMission" in entry})
    nodes = sorted({entry["numberNodes"] for entry in data_entries if "numberNodes" in entry})

    topo_clean = topology.replace("TOPOLOGY_", "")
    metric_dir = OUTPUT_ROOT / "propagation-time"
    metric_dir.mkdir(parents=True, exist_ok=True)
    out_path = metric_dir / f"{protocol}_{topo_clean}_propagation_time.json"

    payload = {
        "metadata": {
            "macProtocol": protocol,
            "topology": topo_clean,
            "timeToNextMission": missions,
            "numberNodes": nodes,
        },
        "data": [],
    }

    normalized_entries = []
    for entry in data_entries:
        meta = {}
        if "timeToNextMission" in entry:
            meta["timeToNextMission"] = entry["timeToNextMission"]
        if "numberNodes" in entry:
            meta["numberNodes"] = entry["numberNodes"]
        stats = entry.get("data", {})
        normalized_entries.append({"metadata": meta, "data": stats})

    payload["data"] = normalized_entries

    out_path.write_text(json.dumps(payload, indent=2))
    print(f"✅ Wrote {out_path}")


def main():
    groups = collect_inputs()

    per_metric: Dict[Tuple[str, str, str], List[Tuple[str, str, Dict[str, List[Path]]]]] = defaultdict(list)
    node_reach_groups: Dict[Tuple[str, str], List[Tuple[str, str, Dict[str, List[Path]]]]] = defaultdict(list)
    reception_success_groups: Dict[Tuple[str, str], List[Tuple[str, str, Dict[str, List[Path]]]]] = defaultdict(list)
    for (protocol, topology, mission, nodes), metrics in groups.items():
        for metric_name, paths in metrics.items():
            per_metric[(protocol, topology, metric_name)].append((mission, nodes, {metric_name: paths, **metrics}))
        if "sent_mission_rts" in metrics and "received_complete_mission" in metrics:
            node_reach_groups[(protocol, topology)].append(
                (mission, nodes, metrics)
            )
        if "received_mission_id_fragment" in metrics:
            reception_success_groups[(protocol, topology)].append((mission, nodes, metrics))

    for (protocol, topology, metric_name), entries in per_metric.items():
        combined_data = []
        collisions_data = []
        bytes_data = []
        for mission, nodes_val, metrics in entries:
            if metric_name == "counter":
                counter_stats = aggregate_counters(metrics.get("counter", []))
                toa_values = counter_stats["time_on_air_values"]
                collisions_sum = counter_stats["collisions_total"]
                bytes_totals = {
                    "allBytes": counter_stats["received_bytes_total"],
                    "effectiveBytes": counter_stats["received_effective_bytes_total"],
                }

                try:
                    collisions_sum /= max(int(nodes_val), 1)
                except Exception:
                    pass
                toa_fairness = compute_jain(toa_values)
                combined_data.append(
                    {
                        "timeToNextMission": int(mission) / 1000.0,
                        "numberNodes": int(nodes_val),
                        "data": toa_fairness,
                    }
                )
                collisions_data.append(
                    {
                        "timeToNextMission": int(mission) / 1000.0,
                        "numberNodes": int(nodes_val),
                        "data": collisions_sum,
                    }
                )
                bytes_data.append(
                    {
                        "timeToNextMission": int(mission) / 1000.0,
                        "numberNodes": int(nodes_val),
                        "data": bytes_totals,
                    }
                )
            else:
                continue

        if metric_name == "counter":
            if combined_data:
                write_time_on_air(protocol, topology, combined_data)
            if collisions_data:
                write_output(protocol, topology, "collision-per-node", collisions_data)
            nd_throughput: List[Dict] = []
            ne_throughput: List[Dict] = []
            mac_efficiencies: List[Dict] = []
            for entry in bytes_data:
                nodes_int = entry["numberNodes"]
                denom = SIMULATION_DURATION_S * max(nodes_int - 1, 1)
                bytes_obj = entry["data"]
                nd = bytes_obj["allBytes"] / denom if denom else 0.0
                ne = bytes_obj["effectiveBytes"] / denom if denom else 0.0
                nd_throughput.append(
                    {
                        "timeToNextMission": entry["timeToNextMission"],
                        "numberNodes": nodes_int,
                        "data": nd,
                    }
                )
                ne_throughput.append(
                    {
                        "timeToNextMission": entry["timeToNextMission"],
                        "numberNodes": nodes_int,
                        "data": ne,
                    }
                )
                ctrl_bytes = max(bytes_obj["allBytes"] - bytes_obj["effectiveBytes"], 0)
                denom_mac = bytes_obj["effectiveBytes"] + ctrl_bytes
                mac_eta = (bytes_obj["effectiveBytes"] / denom_mac) if denom_mac > 0 else None
                mac_efficiencies.append(
                    {
                        "timeToNextMission": entry["timeToNextMission"],
                        "numberNodes": nodes_int,
                        "data": mac_eta,
                    }
                )
            if nd_throughput:
                write_output(protocol, topology, "normalized-data-throughput", nd_throughput)
            if ne_throughput:
                write_output(protocol, topology, "normalized-effective-data-throughput", ne_throughput)
            if mac_efficiencies:
                write_output(protocol, topology, "mac-efficiency", mac_efficiencies)

    for (protocol, topology), entries in node_reach_groups.items():
        combined_reach: List[Dict] = []
        combined_prop: List[Dict] = []
        for mission, nodes, metrics in entries:
            sent_files = metrics.get("sent_mission_rts", [])
            recv_files = metrics.get("received_complete_mission", [])
            data_obj = aggregate_node_reachability(sent_files, recv_files, int(nodes))
            combined_reach.append(
                {
                    "timeToNextMission": int(mission) / 1000.0,
                    "numberNodes": int(nodes),
                    "data": data_obj.get("ratio", {}),
                }
            )
            combined_prop.append(
                {
                    "timeToNextMission": int(mission) / 1000.0,
                    "numberNodes": int(nodes),
                    "data": data_obj.get("propagation_time", {}),
                }
            )
        if combined_reach:
            write_output(protocol, topology, "node-reachability", combined_reach)
        if combined_prop:
            write_output_propagation(protocol, topology, combined_prop)

    for (protocol, topology), entries in reception_success_groups.items():
        combined_success: List[Dict] = []
        for mission, nodes, metrics in entries:
            ratios: List[float] = []
            mission_files = metrics.get("received_mission_id_fragment", [])
            if mission_files:
                ratios.extend(aggregate_reception_success(mission_files, topology, int(nodes), True))
            if ratios:
                combined_success.append(
                    {
                        "timeToNextMission": int(mission) / 1000.0,
                        "numberNodes": int(nodes),
                        "data": compute_stats(ratios),
                    }
                )
        if combined_success:
            write_output(protocol, topology, "reception-success-ratio", combined_success)


if __name__ == "__main__":
    main()
