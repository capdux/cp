# fastq — AI Agent Quick Ref

Zero-allocation fast in-memory fixed-concurrency work queue with push, unshift, pause, resume, kill, and drain (clean-room MIT implementation of npm fastq, #253 / 192M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import { FastQueue } from "fastq";
```

## Core Types & Constants

```cp
class FastQueue { buf: [64]u64, head: usize, tail: usize, queued: usize, /* +3 fields */ } // methods: new(...) +12
```

## Minimal Usage

```cp
import { FastQueue } from "fastq";

pub fn main() {
    // Refer to src/index.d.cp for FastQueue fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fastq"`; install via `cap add fastq`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add fastq
cpc check src/main.cp
```
