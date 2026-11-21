# `cap:proc_stat` — Linux `/proc/<pid>/stat` Parser

Zero-allocation Linux `/proc/<pid>/stat` parser that safely handles process names (`comm`) containing spaces or parentheses by scanning to the last `')'`.

## Exports

- `ProcState` (`running` | `sleeping` | `disk_sleep` | `zombie` | `stopped` | `tracing_stop` | `dead` | `idle` | `unknown`) & `parseProcState(ch: u8)`
- `ProcStat` — `parse(line): ProcStat?`, `totalCpuTicks()`, `rssBytes(page_size)`, `isZombie()`.
