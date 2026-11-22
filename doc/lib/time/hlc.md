# `cap:hlc` — Zero-Allocation Hybrid Logical Clock (HLC)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:time`，导入请使用 `import { ... } from "cap:time"`。

Pure CP, zero-heap-allocation implementation of Kulkarni et al.'s **Hybrid Logical Clock** (used by CockroachDB / YugabyteDB) providing 48-bit physical millisecond + 16-bit logical counter + 32-bit node ID causal timestamps, compact 64-bit bit-packing, clock-drift / overflow guards, and lexicographically sortable 26-byte canonical text codecs.

## Constants

| Constant | Value | Description |
| :--- | :--- | :--- |
| `HLC_MAX_PHYSICAL_MS` | `0x0000FFFFFFFFFFFF` | Max 48-bit physical ms (valid through year 10889). |
| `HLC_MAX_LOGICAL` | `0xFFFF` | Max 16-bit logical counter. |
| `HLC_DEFAULT_MAX_DRIFT_MS` | `60000` | Default maximum tolerated clock drift (60 s). |
| `HLC_STR_LEN` | `26` | Canonical hex format length (`PPPPPPPPPPPP-LLLL-NNNNNNNN`). |

## Core Types & Functions

- **`HlcTimestamp`**: `{ physical_ms: u64, logical: u16, node_id: u32 }`
  - `packU64(): u64` — Packs `(physical_ms << 16) | logical` into a monotonic 64-bit integer.
  - `HlcTimestamp.unpackU64(raw: u64, node_id: u32): HlcTimestamp` — Unpacks 64-bit compact HLC with `node_id`.
  - `compare(other: HlcTimestamp): i8` — Total order (`physical_ms` → `logical` → `node_id`), returns `-1` / `0` / `1`.
  - `equals(other)` / `isBefore(other)` / `isAfter(other)` — Convenience total-order predicates.
- **`HlcClock`**: `{ last: HlcTimestamp, max_drift_ms: u64 }`
  - `HlcClock.init(node_id: u32, max_drift_ms: u64): HlcClock`
  - `now(*_, wall_ms: u64): HlcTimestamp?` — Issues a strictly monotonic timestamp for local/send events; enforces drift and logical overflow bounds.
  - `receive(*_, wall_ms: u64, remote: HlcTimestamp): HlcTimestamp?` — Merges a remote timestamp using Kulkarni's 4-branch algorithm while keeping local `node_id` and enforcing drift/overflow bounds.
- **Canonical Lexicographical Text Codec**:
  - `formatHlc(ts: HlcTimestamp, out: *[]u8): usize?` — Encodes to 26-byte uppercase big-endian hex `PPPPPPPPPPPP-LLLL-NNNNNNNN` whose byte-wise lexicographical order matches `compare()`.
  - `parseHlc(text: []const u8): HlcTimestamp?` — Strictly validates 26-byte length, `'-'` delimiters, and hex characters.

## Example

```cp
import {
    HLC_DEFAULT_MAX_DRIFT_MS,
    HlcClock,
    formatHlc,
    parseHlc,
} from "cap:time";

fn example(buf: *[]u8) {
    let clk = HlcClock.init(0x00000001, HLC_DEFAULT_MAX_DRIFT_MS);
    if (ts := clk.now(1700000000000)) {
        if (n := formatHlc(ts, buf)) {
            * = parseHlc(buf.*[0..n]);
        }
    }
}
```
