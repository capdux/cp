[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/debug

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:debug` ([https://github.com/debug-js/debug](https://github.com/debug-js/debug))

Zero-allocation namespace debug log filter, glob exclusion engine, deterministic ANSI coloring, and diff-ms formatter for CP — clean-room MIT parity with npm `debug`.

零堆分配的 CP 命名空间调试日志过滤与格式化引擎，完整支持 npm `DEBUG` 环境变量语法（逗号/空格分隔、任意 `*` 通配符、`-` 前缀排除优先）、确定性 ANSI 配色与 `+ms`/`+s`/`+m`/`+h` 耗时差格式化（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/debug
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `DebugFilter` | `pub struct { raw_pattern: string }` | 过滤器封装，提供 `DebugFilter.from(pattern)` 与 `isEnabled(ns)` |
| `DebugLogger` | `pub struct { namespace, color_code, last_ms, enabled }` | 命名空间日志器封装，提供 `DebugLogger.create(pattern, ns)` 与 `formatLine(...)` |
| `isNamespaceEnabled` | `(pattern: string, namespace: string) -> bool` | 零堆 glob 通配匹配与 `-` 排除优先判定 |
| `selectColor` | `(namespace: string) -> u8` | 确定性哈希映射至标准 ANSI SGR 色码（`31..36`） |
| `formatDiffMs` | `(diff_ms: i64, out: []u8) -> string` | 耗时差格式化（`+0ms`, `+42ms`, `+2s`, `+3m`, `+1h`） |
| `formatDebugLine` | `(ns: string, msg: string, diff_ms: i64, use_color: bool, out: []u8) -> string` | 输出无色或带 ANSI 色彩的标准调试日志行 |

## Usage / 使用示例

```cp
import { DebugFilter, DebugLogger, isNamespaceEnabled, formatDebugLine } from "@cap/debug";

const filter = DebugFilter.from("http:*,db:*,-http:verbose");
const ok = filter.isEnabled("http:req");       // true
const skip = filter.isEnabled("http:verbose"); // false

let buf: [256]u8 = undefined;
const line = formatDebugLine("http:req", "GET /api", 12, false, buf[0..256]);
// => "http:req GET /api +12ms"
```

## License

MIT
