#! /usr/bin/python

from __future__ import print_function

import json
import os
import re
import sys
import time
import urllib
import urllib2


spreadsheet_url = 'https://script.google.com/macros/s/AKfycbxFeqwigNdhR3KwOVbkXcVk2pBDJM2jp-3NdFBGxUkl5wW5uvaw/exec'
unwanted_test = 'telemetry_page_measurement_results.num_'

def main():
  if len(sys.argv) <= 1:
    print('Usage: %s <revision>-<username>-<android serial number>-<random suffix>.html' % __file__)
    return

  print('Uploading %s files.' % len(sys.argv))
  for path in sys.argv[1:]:
    print('Processing %s...' % path)
    revision, username, android_serial, _ = os.path.basename(path).split('-')
    modified_time = time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime(os.stat(path).st_mtime))
    results_blob = json.loads(re.search('<script id="results-json" type="application/json">(.*?)</script>', open(path).read()).group(1))
    test_results = results_blob[0]['tests']['_BlinkPerfMeasurement']['metrics']
    for test in test_results:
      if test.startswith(unwanted_test):
        continue
      if '.' in test:
        test = test[:test.index('.')]
      print('Uploading results for %s...' % test)
      post_data = urllib.urlencode({
        'revision': revision,
        'username': username,
        'android_serial': android_serial,
        'timestamp': modified_time,
        'test': test,
        'unit': test_results[test]['units'],
        'results': ','.join(map(str, test_results[test]['current'])),
      })
      urllib2.urlopen(spreadsheet_url, post_data)

if __name__ == '__main__':
  main()
