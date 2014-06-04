# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from telemetry.page.actions.all_page_actions import WaitAction
from telemetry.page.page import Page
from telemetry.page.page_set import PageSet


class PerfWeekPage(Page):
  def __init__(self, url, page_set, wait_until_ready=False):
    super(PerfWeekPage, self).__init__(url=url, page_set=page_set)
    self._wait_until_ready = wait_until_ready

  def RunNavigateSteps(self, action_runner):
    action_runner.NavigateToPage(self)
    action_runner.RunAction(WaitAction({"javascript": "measurementReady"}))

  def RunSmoothness(self, action_runner):
    action_runner.RunAction(WaitAction({"seconds": 10}))

class PerfWeekPageSet(PageSet):
  """Description: A collection of animation performance tests"""
  def __init__(self):
    super(PerfWeekPageSet, self).__init__()
    for filename in os.listdir(os.path.join(os.path.dirname(__file__), 'perf_week')):
      if filename.endswith('.html'):
        self.AddPage(PerfWeekPage('file://perf_week/' + filename, self))
