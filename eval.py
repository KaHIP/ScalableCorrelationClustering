#!/usr/bin/env python3
"""Compare benchmark results against baseline. Exit 0 if quality within threshold."""
import sys, math

def parse(path):
    data = {}
    with open(path) as f:
        next(f)  # skip header
        for line in f:
            parts = line.strip().split('\t')
            if len(parts) == 4 and parts[2] not in ('CRASH', 'ERROR'):
                inst, seed, t, c = parts[0], int(parts[1]), float(parts[2]), int(parts[3])
                data.setdefault(inst, []).append((seed, t, abs(c)))
    return data

def geo_mean(vals):
    return math.exp(sum(math.log(v) for v in vals) / len(vals))

def stats(data):
    all_times = [t for runs in data.values() for _, t, _ in runs]
    all_cuts = [c for runs in data.values() for _, _, c in runs]
    return geo_mean(all_times), geo_mean(all_cuts), sum(all_times)

if len(sys.argv) < 2:
    print("Usage: eval.py <current_results> [baseline_results]")
    sys.exit(1)

cur = parse(sys.argv[1])
cur_gt, cur_gc, cur_total = stats(cur)
print(f"Current:  geo_time={cur_gt:.6f}s  geo_|cut|={cur_gc:.1f}  total={cur_total:.2f}s  runs={sum(len(v) for v in cur.values())}")

if len(sys.argv) >= 3:
    base = parse(sys.argv[2])
    base_gt, base_gc, base_total = stats(base)
    print(f"Baseline: geo_time={base_gt:.6f}s  geo_|cut|={base_gc:.1f}  total={base_total:.2f}s")

    speedup = base_gt / cur_gt
    quality_diff = (cur_gc - base_gc) / base_gc * 100
    print(f"Speedup: {speedup:.3f}x  Quality diff: {quality_diff:+.6f}%")

    if abs(quality_diff) > 0.001:
        print("FAIL: quality change > 0.001%")
        sys.exit(1)
    else:
        print("PASS: quality within 0.001%")
else:
    print(f"geo_time: {cur_gt:.6f}")
    print(f"geo_cut: {cur_gc:.1f}")
