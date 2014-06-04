#! /usr/bin/python

from __future__ import print_function

import os
import subprocess
import sys

current_path = os.path.abspath('.')
if '/src' not in current_path:
  print('Must run from inside chromium/src checkout')
  sys.exit(1)
src_path = current_path[:current_path.index('/src') + 4]
perf_tools_path = src_path + '/tools/perf'
page_sets_path = perf_tools_path + '/page_sets'
perf_week_path = src_path + '/third_party/WebKit/PerformanceTests/Animation/PerfWeek'


def ensureSmoothnessPerfWeekBenchmark():
  if not os.path.exists(page_sets_path + '/perf_week.py'):
    subprocess.call(['cp', perf_week_path + '/resources/perf_week.py', page_sets_path])
    with open(perf_tools_path + '/benchmarks/smoothness.py', 'a') as smoothness_benchmarks:
      smoothness_benchmarks.write(open(perf_week_path + '/resources/smoothness_perf_week.py').read())
    print('Patched existing smoothness benchmarks with perf_week BigBenchmarks target')

def refreshPerfWeekFolder():
  source = perf_week_path + '/BigBenchmarks'
  destination = page_sets_path + '/perf_week'
  print('Mirroring\n%s\nas\n%s' % (destination, source))
  subprocess.call(['rm', '-rf', destination])
  subprocess.call(['cp', '-r', source, destination])

def runBigBenchmarks():
  subprocess.call([perf_tools_path + '/run_benchmark', 'smoothness.perf_week'] + sys.argv[1:])

def main():
  ensureSmoothnessPerfWeekBenchmark()
  refreshPerfWeekFolder()
  runBigBenchmarks()

if __name__ == '__main__':
  main()
