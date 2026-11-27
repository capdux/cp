[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# minipass

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:minipass` ([https://github.com/isaacs/minipass](https://github.com/isaacs/minipass))

Zero-allocation fixed-capacity ring-buffer byte stream with synchronous backpressure and piping for CP — clean-room ISC parity with npm `minipass` (326M+/wk).

纯 CP 零堆分配 4096 字节环形缓冲字节流，支持同步背压控制、`read`/`readExact`/`peek`、`pause`/`resume`/`end`/`destroy` 状态机及流间零分配 `pipeTo` 管道转发与 EOF 传播。

## Install / 安装

```bash
cap add minipass
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `MinipassStream.new` | `(): MinipassStream` | Create a fresh 4096-byte ring-buffer stream |
| `.write` | `(*_, data: []const u8): bool` | Write bytes into ring buffer (`false` on backpressure/ended) |
| `.read` / `.readExact` / `.peek` | `(*_, ...) -> usize` | Read up to `out.len`, exact `n` bytes, or non-destructive peek |
| `.pause` / `.resume` / `.isPaused` | `(*_) / (): bool` | Flow-mode control |
| `.end` / `.isEnded` / `.isDrained` | `(*_) / (): bool` | End-of-stream and drain checks |
| `.bufferedBytes` / `.freeSpace` | `(): usize` | Query buffered and remaining ring-buffer capacity |
| `.pipeTo` | `(*_, dest: *MinipassStream): usize` | Transfer bytes into destination stream and propagate EOF |

## Usage / 使用示例

```cp
import { MinipassStream } from "minipass";

let src = MinipassStream.new();
let dst = MinipassStream.new();
* = src.write("hello minipass");
src.end();
const moved = src.pipeTo(*dst); // 14, dst.isEnded() == true
```
