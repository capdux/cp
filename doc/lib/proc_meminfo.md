# `cap:proc_meminfo` — Linux `/proc/meminfo` Parser

Zero-allocation Linux `/proc/meminfo` streaming parser and memory utilization calculator converting `kB` fields to exact byte counts.

## Exports

- `MemInfoField` & `MemInfoIter` (`init(content)`, `next(*_): MemInfoField?`)
- `MemInfo` — `parse(content)`, `availableBytes()`, `usedBytes()`, `swapUsedBytes()`, `usedPercent()`.
