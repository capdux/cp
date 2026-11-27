[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/db

> Official unified database context and request-scoped automatic transaction plugin for `cap:web`.

- **License**: MIT
- **Type declarations**: [`src/index.d.cp`](./src/index.d.cp)
- **Install / Import**:
  ```cp
  import { DbDialect, dbBeforeHandle, dbTxStats } from "@web/db";
  ```

## Overview

`@web/db` integrates request-scoped database lifecycle management and automatic read/write transaction wrapping into `cap:web` request handlers.

For full API signatures and examples, see [`src/index.d.cp`](./src/index.d.cp) or [`README_AGENT.md`](./README_AGENT.md).
