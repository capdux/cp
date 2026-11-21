# `cap:proc_loadavg` — Linux `/proc/loadavg` & `/proc/uptime` Parser

Zero-allocation pure-integer Linux `/proc/loadavg` and `/proc/uptime` parsers with exact hundredths/millisecond scaling.

## Exports

- `LoadAvg` — `parse(content): LoadAvg?`, `isOverloaded(num_cpus): bool` (`load1_centi`, `load5_centi`, `load15_centi`, `running_procs`, `total_procs`, `last_pid`).
- `SystemUptime` — `parse(content): SystemUptime?`, `uptimeSeconds(): u64` (`uptime_ms`, `idle_ms`).
