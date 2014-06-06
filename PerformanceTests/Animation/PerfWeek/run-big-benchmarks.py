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
resource_perf_week_page_set_path = perf_week_path + '/resources/perf_week.py'
smoothness_perf_week_page_set_path = page_sets_path + '/perf_week.py'


def ensureSmoothnessPerfWeekBenchmark():
  if not os.path.exists(smoothness_perf_week_page_set_path):
    with open(perf_tools_path + '/benchmarks/smoothness.py', 'a') as smoothness_benchmarks:
      smoothness_benchmarks.write(open(perf_week_path + '/resources/smoothness_perf_week.py').read())
      print('Patched existing smoothness benchmarks with perf_week BigBenchmarks target')
  # Ensure the version of the perf_week.py page set is up to date.
  subprocess.call(['cp', resource_perf_week_page_set_path, smoothness_perf_week_page_set_path])

def refreshPerfWeekFolder():
  source = perf_week_path + '/BigBenchmarks_generated'
  destination = page_sets_path + '/perf_week'
  subprocess.call(['rm', '-r', destination])
  subprocess.call(['cp', '-r', source, destination])

def runBigBenchmarks(run_benchmark_args):
  subprocess.call([perf_tools_path + '/run_benchmark', 'smoothness.perf_week'] + run_benchmark_args)

def main():
  ensureSmoothnessPerfWeekBenchmark()
  refreshPerfWeekFolder()
  runBigBenchmarks(sys.argv[1:])

if __name__ == '__main__':
  main()
