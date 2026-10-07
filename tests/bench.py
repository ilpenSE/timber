#!/bin/env python3

import subprocess
avg_mt = 0
avg_st = 0

for i in range(0, 10):
    res_mt = float(subprocess.run("./test.py -v O3 -t 25_stress_test_mt | awk '/^Throughput:/ {print $2}'", shell=True, check=True, text=True, capture_output=True).stdout.strip().replace(".", "").replace(",", "."))
    res_st = float(subprocess.run("./test.py -v O3 -t 20_stress_test_st | awk '/^Throughput:/ {print $2}'", shell=True, check=True, text=True, capture_output=True).stdout.strip().replace(".", "").replace(",", "."))
    avg_mt += res_mt
    avg_st += res_st
    print(f"st={res_st:,.2f}, mt={res_mt:,.2f}")

avg_mt /= 10
avg_st /= 10
print(f"[DROP POLICY] AVERAGE: st={avg_st:,.2f}, mt={avg_mt:,.2f}")
