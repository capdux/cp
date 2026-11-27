[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# chokidar

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:chokidar` ([https://github.com/paulmillr/chokidar](https://github.com/paulmillr/chokidar))（原库名 `chokidar` → CP 包名 `chokidar`）

Zero-heap, pure CP file watcher core state machine, snapshot differ, write-finish debouncer, and glob ignore filter (clean-room MIT implementation inspired by npm `chokidar` v4).

## Features

- **Zero heap allocation**: Fixed 32-slot tracked snapshot table and 32-slot FIFO event ring buffer.
- **Composable**: Reuses `fast-glob` (`FastGlobOptions`, `GlobPatternSet`) for ignore patterns, dotfile filtering, and depth limiting.
- **Full watch lifecycle**: Emits `EVT_ADD`, `EVT_CHANGE`, `EVT_UNLINK`, `EVT_ADD_DIR`, and `EVT_UNLINK_DIR` with `ignore_initial` and `await_write_finish_ms` debounce support.

## Usage

```cp
import {
    ChokidarOptions, FileWatcherCore, EVT_ADD, EVT_CHANGE, EVT_UNLINK
} from "chokidar";

let watcher = FileWatcherCore.new(ChokidarOptions.defaults());
* = watcher.addIgnore("node_modules/**");
let k = watcher.recordSnapshot("src/main.cp", false, 1000 as u64, 42 as u64, false);
```
