# pt4droid 0.2.1-test.23

- Fixed a crash on the first **Start** after applying graphics settings from the launcher.
- Preserved ray-tracing choices until the selected driver's support is known, and retained pending settings if graphics initialization fails.
- Preserved native crash reports after restarting and included them in diagnostic exports.
- Improved Java crash-report storage and diagnostic collection during log rotation.
- Included pending graphics/preset changes and driver startup failures in diagnostic exports.
- Added app version, build identity and timestamps to crash reports, plus a build identity file in diagnostic exports.

Includes the rendering improvements and graphics options from Test22. The bundled driver remains **ESMERINGHOY 27.0.0-devel**.

[Full changelog from Test19 onward](https://github.com/XHYN-PH/pt4droid/blob/pt4droid/CHANGELOG.md).

Install over your existing pt4droid installation to keep game files, saves and settings.
