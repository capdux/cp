[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/multipart

> Official zero-copy `multipart/form-data` parser and payload guard plugin for `cap:web`.

- **License**: MIT
- **Type declarations**: [`src/index.d.cp`](./src/index.d.cp)
- **Install / Import**:
  ```cp
  import { mpFindBoundary } from "@web/multipart";
  ```

## Overview

`@web/multipart` implements RFC 7578 `multipart/form-data` boundary scanning, header parsing, and request size/part-count payload guards for `cap:web` applications without unnecessary heap allocations.

For full API signatures and examples, see [`src/index.d.cp`](./src/index.d.cp) or [`README_AGENT.md`](./README_AGENT.md).
