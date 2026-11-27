[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/xxhash

> Native xxHash 0.8.3 non-cryptographic hash bindings (XXH32 / XXH64 / XXH3) for CP.

- **License**: BSD-2-Clause
- **Type declarations**: [`src/index.d.cp`](./src/index.d.cp)
- **Install / Import**:
  ```cp
  import { xxh32, xxh64, XXH3_64bits } from "@cap/xxhash";
  ```

## Overview

`@cap/xxhash` exposes native C xxHash 0.8.3 (`XXH32`, `XXH64`, `XXH3_64bits`) alongside ergonomic CP slice wrappers (`xxh32`, `xxh64`) operating at RAM bandwidth speed.

## Quick Example

```cp
import { xxh32, xxh64 } from "@cap/xxhash";

fn checksum(data: &[]u8): u64 {
    let h32 = xxh32(data, 0);
    return xxh64(data, h32 as u64);
}
```

For full API signatures, see [`src/index.d.cp`](./src/index.d.cp) or [`README_AGENT.md`](./README_AGENT.md).
