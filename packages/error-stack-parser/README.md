[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/stacktrace

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:error-stack-parser` ([https://github.com/stacktracejs/error-stack-parser](https://github.com/stacktracejs/error-stack-parser))（原库名 `error-stack-parser` → CP 包名 `@cap/stacktrace`）

零堆分配的 CP 多语言调用栈解析与清洗引擎 — 洁净室 MIT 实现，对标 npm `error-stack-parser`、`stacktrace-parser` 与 `clean-stack`。

Zero-allocation multi-language stack trace parser & cleaner for CP — clean-room MIT parity with npm `error-stack-parser`, `stacktrace-parser`, and `clean-stack`.

## 特性 / Features

- **三大主流格式全支持 / 3 Major Formats**：
  - **V8 / Node / Chrome / Bun**：`at myFunc (/app/src/main.ts:42:13)`、`at /app/src/main.ts:42:13`、`at Object.<anonymous> (node:internal/modules:10:5)`、`at native`
  - **SpiderMonkey / Firefox / Safari**：`myFunc@https://example.com/app.js:42:13`、`@app.js:10:2`、`forEach@[native code]`
  - **Rust / Zig / CP**：`src/main.cp:88:12`、`0x1234 in handleRequest (src/server.zig:120)`
- **内部帧智能清洗 / Clean Stack**：自动识别 `node:internal/`、`internal/`、`node:`、`<anonymous>`、`native` 等运行时内部帧，`cleanStack` 一键输出整洁应用调用栈。
- **零堆分配 / Zero Heap Allocation**：所有 `StackFrame` 字符串零拷贝切片自原始输入，格式化直写调用方 `out: []u8`。

## 安装 / Install

```bash
cap add @cap/stacktrace
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `StackFrame` | `{ func_name, file, line, column, is_native, is_internal }` |
| `ParsedStack` | `{ frames[32], count, message }` + `frameAt(i)` |
| `parseFrameLine(line)` | 解析单行调用栈帧，返回 `StackFrame?` |
| `parseStack(raw)` | 解析完整多行调用栈（含首行错误消息与全部栈帧） |
| `formatFrame(f, out)` | 零分配格式化单帧为规范 `at func (file:line:col)` |
| `cleanStack(raw, out)` | 过滤内部帧并输出整洁调用栈文本 |

## 使用示例 / Usage

```cp
import { parseStack, cleanStack } from "@cap/stacktrace";

const raw = "Error: boom\n    at handleReq (/app/src/api.ts:42:13)\n    at Module._compile (node:internal/modules/cjs/loader:1105:14)";
const parsed = parseStack(raw);
// parsed.message == "Error: boom", parsed.count == 2

let buf: [1024]u8 = undefined;
const cleaned = cleanStack(raw, buf[0..1024]);
// => "Error: boom\n    at handleReq (/app/src/api.ts:42:13)"
```

## License

MIT
