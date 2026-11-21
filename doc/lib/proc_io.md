# `cap:proc_io` — Linux `/proc/<pid>/io` Zero-Allocation Parser

Zero-allocation, pure-CP single-pass parser for Linux `/proc/<pid>/io` process I/O counters with overflow-safe aggregations and sampling-window deltas.

## Exports

### Types

- `ProcIoStats`
  - Fields (`u64`): `rchar`, `wchar`, `syscr`, `syscw`, `read_bytes`, `write_bytes`, `cancelled_write_bytes`
  - Methods:
    - `netWriteBytes(): u64` — Saturating `write_bytes - cancelled_write_bytes`
    - `totalStorageBytes(): u64` — Saturating `read_bytes + netWriteBytes()`
    - `totalSyscalls(): u64` — Saturating `syscr + syscw`
    - `totalCharBytes(): u64` — Saturating `rchar + wchar`
    - `diff(earlier: ProcIoStats): ProcIoStats` — Per-field saturating difference (`self - earlier`)

### Functions

- `parseProcIo(content: []const u8): ProcIoStats?` — Parses `key: value` lines across `\n` and `\r\n`, skips unknown extension keys, strictly rejects invalid or overflowing numbers on known keys, and returns `null` when no known key is present.

## Example

```typescript
import { parseProcIo } from "cap:proc_io";

const sample =
    "rchar: 1048576\n" ++
    "wchar: 524288\n" ++
    "syscr: 120\n" ++
    "syscw: 45\n" ++
    "read_bytes: 262144\n" ++
    "write_bytes: 131072\n" ++
    "cancelled_write_bytes: 4096\n";

if (io := parseProcIo(sample)) {
    const net_w = io.netWriteBytes();         // 126976
    const total = io.totalStorageBytes();     // 389120
    const calls = io.totalSyscalls();         // 165
}
```
