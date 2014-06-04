
class SmoothnessPerfWeek(test.Test):
  """Measures rendering statistics for big animation benchmarks
  """
  tag = 'perf_week'
  test = smoothness.Smoothness
  page_set = 'page_sets/perf_week.py'
  def CustomizeBrowserOptions(self, options):
    options.AppendExtraBrowserArgs(['--enable-experimental-web-platform-features'])
