[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`yallist`](https://github.com/isaacs/yallist)（`ISC`）（包名 `yallist`）。

# `yallist`

Zero-heap-allocation O(1) index-pool doubly linked list in pure CP (clean-room ISC implementation inspired by npm `yallist` v5).

## Features

- **`YallistU64`** — Fixed 64-slot freelist-backed doubly linked list with zero heap allocations.
- **O(1) Ends & Handle Ops** — `push`, `pop`, `unshift`, `shift`, `removeNode`, `unshiftNode`, `pushNode`, `headVal`, `tailVal`.
- **Traversal & Utilities** — `get(idx)` (bidirectional nearest-end walk), `reverse()`, `toArray(out)`, `len()`, `isEmpty()`.

## Usage

```cp
import { YALLIST_CAP, YallistU64 } from "yallist";
```

## License

ISC
