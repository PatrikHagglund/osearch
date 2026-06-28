#!/usr/bin/env python3
"""Inject per-flag effectiveness weights (w_speed / w_size) into a config.

Usage: annotate.py <data_dir> <measure_config> <target_config>

<data_dir> holds one osearch JSON dump per (mode, benchmark), named
"size_*.json"/"size_*.out" (-> w_size) and "perf_*.json"/"perf_*.out"
(-> w_speed); see annotate.sh, which produces them. The target config is
rewritten in place. Weights are a soft PRIOR: they only order/bias the search
(see README "Effectiveness-ranked options"), never exclude.
"""
import json
import os
import re
import sys
from collections import defaultdict

data_dir, measure_cfg, target_cfg = sys.argv[1], sys.argv[2], sys.argv[3]


def collect(mode):
    """Mean marginal effect per option for one objective.

    The JSON "diff" is already improvement-normalised (negative = better) for
    both additions and removals, so it is used as-is; "flags" names the option.
    """
    diffs = defaultdict(list)
    for fn in sorted(os.listdir(data_dir)):
        if not fn.startswith(mode + "_") or not (
            fn.endswith(".json") or fn.endswith(".out")
        ):
            continue
        with open(os.path.join(data_dir, fn)) as f:
            content = f.read()
        if "[" not in content:
            continue
        try:
            data = json.loads(content[content.rfind("[") :])
        except json.JSONDecodeError:
            continue
        for e in data:
            if e.get("equal"):
                continue
            d = e.get("diff")
            if d is None:
                continue
            toks = e.get("flags", "").split()
            if len(toks) < 2:
                continue
            name = toks[-1].lstrip("!")  # last token, sans removal marker
            diffs[name].append(d)
    return {k: sum(v) / len(v) for k, v in diffs.items()}


def scale(avg_map):
    """Lower diff = better -> higher weight, normalised to [-8, 8].

    Reference is the 85th-percentile |effect| (not the max) so a single huge
    outlier — e.g. the -O level change — doesn't compress every other option to
    zero; the top ~15% saturate and the rest still spread.
    """
    if not avg_map:
        return {}
    nz = sorted(abs(v) for v in avg_map.values() if v)
    if not nz:
        return {k: 0 for k in avg_map}
    ref = nz[min(len(nz) - 1, int(0.85 * len(nz)))] or 1.0
    return {k: max(-8, min(8, round(8 * (-v) / ref))) for k, v in avg_map.items()}


w_size = scale(collect("size"))
w_speed = scale(collect("perf"))

# Options present in the measure config get a +1 membership base, so a measured
# curated flag still outranks an unmeasured full-only flag (reached only in an
# uncapped audit run).
measured = set()
with open(measure_cfg) as f:
    for line in f:
        if "<flag" not in line:
            continue
        m = re.search(r'value="([^"]+)"', line)
        if m:
            measured.update(m.group(1).split("|"))


def clamp(x):
    return max(-9, min(9, x))


def weight_for(value, is_enum):
    # enums are structural (the -O level, cost models, ...): always rank top.
    if is_enum:
        return 9, 9
    base = 1 if value in measured else 0
    return clamp(base + w_speed.get(value, 0)), clamp(base + w_size.get(value, 0))


with open(target_cfg) as f:
    lines = f.readlines()

out = []
annotated = 0
for line in lines:
    m = re.search(r'<flag\s+type="(simple|enum)"\s+value="([^"]+)"', line)
    if not m:
        out.append(line)
        continue
    kind, value = m.group(1), m.group(2)
    ws, wz = weight_for(value, kind == "enum")
    indent = line[: len(line) - len(line.lstrip())]
    out.append(
        f'{indent}<flag type="{kind}" value="{value}" '
        f'w_speed="{ws}" w_size="{wz}" />\n'
    )
    annotated += 1

with open(target_cfg, "w") as f:
    f.writelines(out)

print(
    f"Annotated {annotated} flags in {target_cfg} "
    f"({len(measured)} measured options)",
    file=sys.stderr,
)
