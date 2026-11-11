# How far each ship's heading is off the way it actually moves, from a run's `# fmwatch` lines
# (tests/probes/fleet_heading.eeai): per unit, over windows of about 1 s game time (the lines come every ~170 ms)
# while it covers at least 0.3 tiles, the angle between its heading and its displacement - "steady" the windows its
# heading turned 8 degrees or less in (a ship pushed along sideways), "turning" the rest (in an arc the heading leads
# the chord); the mean, the 90th percentile and the share off by more than 20 degrees. Then the order's `fmove sum`.
# usage: python scripts/results/fleet_heading.py <log.csv.gz> ...
import math, os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import logs

WINDOW_MS = 1000
MIN_MOVE = 0.3
TURNING = 8.0


def angle(a):
    return abs((a + 180) % 360 - 180)


def one(path):
    track, sums = {}, []
    for line in logs.open_log(path):
        if line.startswith('# fmwatch '):
            p = line.split()
            track.setdefault(p[3], []).append((int(p[2]), float(p[4]), float(p[5]), float(p[6])))
        elif line.startswith('# setup: fmove sum') and 'fmove sum:' in line:
            sums.append(line.split('fmove sum: ')[1].split(';')[0])
    kinds = {'steady': [], 'turning': []}
    for pts in track.values():
        i = 0
        for j in range(len(pts)):
            while pts[j][0] - pts[i][0] > WINDOW_MS:
                i += 1
            if pts[j][0] - pts[i][0] < WINDOW_MS * 0.75:
                continue
            dx, dy = pts[j][1] - pts[i][1], pts[j][2] - pts[i][2]
            if math.hypot(dx, dy) < MIN_MOVE:
                continue
            off = angle(pts[j][3] - math.degrees(math.atan2(dx, dy)))   # fmwatch's heading: atan2(f.x, f.y)
            kinds['turning' if angle(pts[j][3] - pts[i][3]) > TURNING else 'steady'].append(off)
    print(path)
    for k, a in kinds.items():
        if a:
            a.sort()
            print('  %-8s windows %5d  mean %5.1f  p90 %5.1f  >20 deg %5.1f%%' % (
                k, len(a), sum(a) / len(a), a[int(len(a) * 0.9)], 100.0 * sum(d > 20 for d in a) / len(a)))
    for s in sums:
        print('  sum: ' + s)


for p in logs.log_paths(sys.argv[1:]):
    one(p)
