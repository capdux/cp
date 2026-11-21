# `cap:proc_statm` — Linux `/proc/<pid>/statm` Zero-Allocation Parser

Zero-allocation, pure-CP parser for Linux `/proc/<pid>/statm` 7-column memory page counters with overflow-safe byte conversion helpers.

## Exports

### Types

- `ProcStatm`
  - Fields:
    - `size_pages: u64` — Total virtual memory size in pages (`VmSize`)
    - `resident_pages: u64` — Resident set size in pages (`VmRSS`)
    - `shared_pages: u64` — Resident shared pages
    - `text_pages: u64` — Text (code) segment pages
    - `lib_pages: u64` — Library pages (unused / 0 on Linux 2.6+)
    - `data_pages: u64` — Data + stack pages
    - `dirty_pages: u64` — Dirty pages (unused / 0 on Linux 2.6+)
  - Methods:
    - `vmBytes(page_size: u64): u64` — Saturating `size_pages * page_size`
    - `rssBytes(page_size: u64): u64` — Saturating `resident_pages * page_size`
    - `sharedBytes(page_size: u64): u64` — Saturating `shared_pages * page_size`
    - `textBytes(page_size: u64): u64` — Saturating `text_pages * page_size`
    - `dataBytes(page_size: u64): u64` — Saturating `data_pages * page_size`
    - `privateRssPages(): u64` — Saturating `resident_pages - shared_pages`
    - `privateRssBytes(page_size: u64): u64` — Saturating `privateRssPages() * page_size`

### Functions

- `parseProcStatm(content: []const u8): ProcStatm?` — Strictly parses 7 whitespace-separated unsigned `u64` integers (supports leading/trailing whitespace and `\r\n`, rejects fewer than 7 or more than 7 fields, negative numbers, non-digits, and `u64` overflows).

## Example

```typescript
import { parseProcStatm } from "cap:proc_statm";

const sample = "25600 4096 1024 512 0 2048 0\n";
if (m := parseProcStatm(sample)) {
    const rss = m.rssBytes(4096);           // 16_777_216 (16 MiB)
    const priv = m.privateRssBytes(4096);   // 12_582_912 (12 MiB)
}
```
