#!/usr/bin/python
import datetime
import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import urllib
import urllib2

apk_server = 'http://ned.syd:8115/apks/'
run_micro_benchmark_command = [os.path.abspath('../../../../../tools/perf/run_benchmark'), 'blink_perf.animation']
run_macro_benchmark_command = [os.path.abspath('./run-big-benchmarks.py')]
macro_benchmark_flag = '--big'

def get_apks():
  index = urllib2.urlopen(apk_server).read()
  apks = re.findall('ContentShell.*?apk', index)
  if len(apks) < 1000:
    sys.exit('cant find apks')
  apks.sort()
  return [apk_server + apk for apk in apks]

def apk_date(apk):
  match = re.search('(?<=-)(\d{4})-(\d{2})-(\d{2})', apk)
  return datetime.date(int(match.group(1)), int(match.group(2)), int(match.group(3)))

def get_daily_apks():
  last = datetime.date(1, 1, 1)
  for apk in get_apks():
    date = apk_date(apk)
    if date != last:
      yield apk
    last = date

def get_user():
  return subprocess.Popen(['whoami'], stdout=subprocess.PIPE).communicate()[0].rstrip()

def get_serial():
  return subprocess.Popen(['adb', 'get-serialno'], stdout=subprocess.PIPE).communicate()[0].rstrip()

def run_tests(command, apks, page_filter):
  os.system('adb wait-for-device')
  print 'starting, %d revisions to test' % len(apks)

  user = get_user()
  serial = get_serial()
  out_dir = tempfile.mkdtemp()
  print 'Storing results in %s' % out_dir

  for i, apk in enumerate(apks):
    start = time.time()
    revision = re.search('(?<=r)\d+', apk).group(0)

    apk_file = os.path.join(tempfile.gettempdir(), revision + '.apk')
    urllib.urlretrieve(apk, apk_file)

    os.system('adb install -r %s &> /dev/null' % apk_file)
    os.remove(apk_file)

    out_file = os.path.join(out_dir, '%s-%s-%s-%s.html' % (revision, user, serial, i))
    subprocess.call(command + [
      '--browser=android-content-shell',
      '--reset-results',
      '--page-filter=%s' % page_filter,
      '--output=%s' % out_file,
    ])

    # FIXME: upload results from out_file
    remaining = (time.time() - start) * (len(apks) - i + 1)
    remaining = datetime.timedelta(0, remaining)
    print '%d of %d - %s - %s remaining' % (i + 1, len(apks), out_file, remaining)
  print 'Results stored in %s' % out_dir
  print 'To upload the results run: ./upload-results.py %s/*' % out_dir

base_date = datetime.date(1, 1, 1)
def use_apk_for_api_test(apk):
  date = apk_date(apk)
  return date >= datetime.date(2013, 12, 12) and ((date - base_date).days % 3) == 0

def use_apk_for_css_test(apk):
  date = apk_date(apk)
  return ((date - base_date).days % 7) == 0

def main():
  if os.system('adb version &> /dev/null') != 0:
    sys.exit('adb is not in path')

  if os.system(run_micro_benchmark_command[0] + ' --help &> /dev/null') != 0:
    sys.exit('cant find run_benchmark, are you in PerformanceTests/Animation/PerfWeek?')

  # FIXME: add a flag to switch predicates
  predicate = use_apk_for_api_test
  apks = [apk for apk in get_daily_apks() if predicate(apk)]
  if macro_benchmark_flag in sys.argv:
    sys.argv.remove(macro_benchmark_flag)
    command = run_macro_benchmark_command
  else:
    command = run_micro_benchmark_command
  if len(sys.argv) < 2:
    sys.exit('Usage: %s [%s] page_filter_args' % (sys.argv[0], macro_benchmark_flag))
  page_filter = sys.argv[1]
  run_tests(command, apks, page_filter)

if __name__ == '__main__':
  main()
