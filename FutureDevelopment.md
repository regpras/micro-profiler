  * ~~Collection of child calls statistics. This will allow defining the most hot execution paths at a glance~~;
  * ~~Redevelopment of a frontend using TDD - otherwise I cannot be sure it has production quality~~;
  * ~~Eliminate dependency from MS DIA library~~;
  * ~~DLL profiling support~~;
  * ~~Performance improvement - there is a potential of up to x3 times `_penter`/`_pexit` speedup~~;
  * ~~X64 support - it just requires some more ASM-hacking :)~~
  * Time-based profile - a chart with ability to snap statistics profile to a specific time range giving user an opportunity to see what was happening at that exact moment of time;
  * Calls/second summary metric. Beside all may ease an estimation the level of latency induced by the profiler;
  * Integration for VS 12.0 (2013) via VSPackage. ~~Integration fix for localized versions of VisualStudio~~;
  * Pie-charts for hot functions;
  * On-the-fly image instrumentation to eliminate the need of recompiling (and even the requirement to have VisualStudio installed).