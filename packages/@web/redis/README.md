[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/redis

> Official thin Redis lifecycle, distributed namespace key builder, and zero-heap RESP2/3 command encoder plugin for `cap:web`.

- **License**: MIT
- **Type declarations**: [`src/index.d.cp`](./src/index.d.cp)
- **Install / Import**:
  ```cp
  import { rdsDefaultPing, rdsIsSafeKeySegment } from "@web/redis";
  ```

## Overview

`@web/redis` provides safe namespaced key construction and zero-heap RESP2/RESP3 wire command encoding for `cap:web` services.

For full API signatures and examples, see [`src/index.d.cp`](./src/index.d.cp) or [`README_AGENT.md`](./README_AGENT.md).
