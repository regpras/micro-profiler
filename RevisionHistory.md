  * [v1.1.581](https://drive.google.com/folderview?id=0B3C94qNXgdQBaFN3ejZVa2l1SGs&usp=drive_web)
    * VisualStudio Integration redone: MicroProfiler is now a VSPackage. This made it possible to integrate with VisualStudio 2013 and 2014;
    * Menu items visibility changed: now there is only 'Enable Profiling' menu item is visible for a non-profiled project;
    * The bug that prevented MicroProfiler window from popping up (after closing it, while minimized) is now fixed;
    * One may want to remove ghost menu items from previous versions. This can be done by issuing a command for a corresponding development environment: 'devenv /ResetAddin MicroProfiler.Addin.MicroProfiler'

  * v1.1.567
    * The integration is now fixed for all VisualStudio versions prior VS 12.0 (2013).

  * v1.1.565
    * The installer is no longer silent - it displays basic pages.

  * v1.1.552
    * The first version placed in VisualStudio Gallery;
    * Profiler window placement is now restored on next start;
    * Statistic lists now preserve widths of columns and sorting order;
    * Build is now almost entirely relies on shell scripts instead of cscript.

  * [v1.0.530](https://drive.google.com/folderview?id=0B2lZ03dFgtG7OVFObVJKSE51cGM&tid=0B2lZ03dFgtG7ZHJDMHZvR3FGVjA#list)
    * Double return values, passed via XMM0 register were corrupted - fixed;

  * [v1.0.527](https://drive.google.com/folderview?id=0B2lZ03dFgtG7OVNVTU41alozUjg#list)
    * DLLs profiling is now supported;

  * v1.0.520
    * Two-fold increase in collection speed (two times less impact on the profiled program comparing to that of previous profiler versions);

  * v1.0.500
    * Crash at profiling application exiting fixed;
    * x64 installer now installs VS-integration addin;

  * v1.0.493
    * Profiler's window is now not closed at the profiling application's finish and is held until explicitly closed by user;

  * v1.0.487
    * x64 platform support implemented. A very alpha version, but it worth trying!

  * v1.0.480
    * There was a bug leading to proportional increase in all timings. It was caused by that processor's ticks duration were incorrectly measured. Now the bug fixed;

  * v1.0.473
    * MicroProfiler is now coming as MSI installation package;
    * Executable' name of a profiled application is now displayed in the profiler's window caption.

  * [r443](https://code.google.com/p/micro-profiler/source/detail?r=443)
    * MicroProfiler no longer require DIA to be installed on the target machine. It now uses Debug Helper library that comes with Windows;

  * [r440](https://code.google.com/p/micro-profiler/source/detail?r=440)
    * Windows XP compatibility fixed (RegGetValue is not available on this platform);

  * [r436](https://code.google.com/p/micro-profiler/source/detail?r=436)
    * Integration with VS 2005 (8.0) is now fully functional;

  * [r427](https://code.google.com/p/micro-profiler/source/detail?r=427)
    * Children/Parent statistics is now displayed. It has been moved from experimental to the main branch of development and backed by tests;
    * VisualStudio integration implemented (works with VS 7.1, 8.0, 9.0, 10.0, 11.0). It allows:
      * Enable profiling on a project in one click (using context menu in solution explorer);
      * Easy removal of all profiler traces from the project;
      * No need in additional copying of micro-profiler.dll to the directory of an application being profiled.
    * Performance improvements in analysis/frontend parts. Now the frontend updates are being performed less frequently than call trace analyses, not spending CPU time on UI updates;
    * MS DIA usage changed. Now the newest available version from the list of 10.0, 9.0, 8.0, 7.1 is being picked.

  * [r353](https://code.google.com/p/micro-profiler/source/detail?r=353) (experimental)
    * VisualStudio 9.0 SP (2008) integration implemented. It allows:
      * Enable profiling on a project in one click (using context menu in solution explorer);
      * Easy removal of all profiler traces from the project;
    * Performance improvements in analysis part.

  * [r265](https://code.google.com/p/micro-profiler/source/detail?r=265) (experimental)
    * Statistics of parent callers is now visible for the focused function. Also it is possible to drill-up by double-clicking the caller function.

  * [r138](https://code.google.com/p/micro-profiler/source/detail?r=138) (experimental)
    * Statistics of child calls made from a function selected is now available. You now can drill-down by double-clicking in child statistics list.

  * [r102](https://code.google.com/p/micro-profiler/source/detail?r=102)
    * Ability to create frontend as in-process server (by specifying `micro_profiler::profiler_frontend pf(&micro_profiler::create_inproc_frontend)`).

  * [r98](https://code.google.com/p/micro-profiler/source/detail?r=98)
    * Windows XP+ visual styles supported;
    * Resizing of the main window supported;
    * Statistics list view headers now have sorting direction arrows;
    * Ability to copy all the statistics to the clipboard as text;
    * Window icon added.

  * [r89](https://code.google.com/p/micro-profiler/source/detail?r=89)
    * Collector and Frontend are merged into one dll. You have to register only one file - the dll. Frontend is run from a surrogate COM-process;
    * Recursion control: inclusive times are collected only for top-level function calls, plus maximal recursion depth is displayed for a function;
    * Significant speedup in analysis thread by using address-specific hash-functions for statistics and recursion level dictionaries;
    * Test runner (run-tests.cmd) is included into deployment package to ensure platform compatibility. Requires VS2008 with unit-testing framework installed.

  * [r68](https://code.google.com/p/micro-profiler/source/detail?r=68)
    * Initial revision. Separate collector/frontend parts.