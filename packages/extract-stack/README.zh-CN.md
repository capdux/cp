[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `extract-stack`

> **源包溯源**：`npm:extract-stack` ([sindresorhus/extract-stack](https://github.com/sindresorhus/extract-stack)) — 协议：MIT

纯 CP 实现的零堆分配、零拷贝 `Error.stack` 堆栈帧提取器与结构化帧解析器，同时兼容 V8/Node.js 与 Zig 堆栈格式。

## 零成本迁移对照（TS → CP）

直接沿用原包名 `extract-stack`，无需改名：

```typescript
// TypeScript / Node.js (npm: extract-stack)
import extractStack from "extract-stack";
const frames = extractStack(err.stack);
const lines = extractStack.lines(err.stack);
```

```cp
// CP（零拷贝切片 + 结构化帧解析）
import { extractStack, extractStackLines, parseStackFrames, StackFrame } from "extract-stack";

let raw = "Error: boom\n    at run (/app/main.ts:12:5)\n    at main (/app/main.ts:20:1)";
let frames_text = extractStack(raw);

let lines: [16]string = [""; 16];
let line_count = extractStackLines(raw, lines[0..16]);

let parsed: [16]StackFrame = undefined;
let frame_count = parseStackFrames(raw, parsed[0..16]);
```

## 安装配置

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "extract-stack": "1.0.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `StackFrame` | `class { function_name: string, file: string, line: u32, column: u32, is_native: bool, is_async: bool, format: u8, valid: bool }` | 解析后的堆栈帧（`format`: `0`=未知，`1`=V8，`2`=Zig） |
| `extractStack` | `(stack: string): string` | 零拷贝提取从首个缩进 `at ...` 行开始的堆栈正文 |
| `extractErrorHeader` | `(stack: string): string` | 零拷贝提取错误名与（支持多行的）错误描述头部 |
| `extractStackLines` | `(stack: string, out: []string): usize` | 提取逐行堆栈并去除首尾空白与 `"at "` 前缀 |
| `parseStackFrame` | `(frame_line: string): StackFrame` | 将单行 V8/Node.js 或 Zig 帧解析为结构化对象 |
| `parseStackFrames` | `(stack: string, out: []StackFrame): usize` | 批量解析整段堆栈中的所有有效帧 |
| `countStackFrames` | `(stack: string): usize` | 零缓冲区统计有效堆栈帧总数 |
| `exs*` | 底层辅助函数（`exsIsFrameLine`、`exsTrimLine`、`exsParseLocation` 等） | 行判定、裁剪与位置解析底层原语 |

## 实战示例

### 1. 零拷贝分离错误头与堆栈行

```cp
import { extractErrorHeader, extractStack, extractStackLines, countStackFrames } from "extract-stack";

let trace = "TypeError: invalid input\n  details: bad token\n    at parse (/src/parser.cp:42:9)\n    at main (/src/main.cp:10:3)";

let header = extractErrorHeader(trace); // "TypeError: invalid input\n  details: bad token"
let body = extractStack(trace);         // "    at parse (/src/parser.cp:42:9)\n    at main (/src/main.cp:10:3)"
let total = countStackFrames(trace);    // 2

let lines: [8]string = [""; 8];
let n = extractStackLines(trace, lines[0..8]);
// lines[0] == "parse (/src/parser.cp:42:9)"
// lines[1] == "main (/src/main.cp:10:3)"
```

### 2. 解析 V8 与 Zig 结构化帧

```cp
import { parseStackFrame, StackFrame } from "extract-stack";

// V8 / Node.js 帧
let v8 = parseStackFrame("    at async fetchData (/app/net.ts:88:14)");
// v8.valid == true, v8.is_async == true, v8.function_name == "fetchData"
// v8.file == "/app/net.ts", v8.line == 88, v8.column == 14, v8.format == 1

// Zig 堆栈帧
let zig = parseStackFrame("/app/src/main.zig:51:12: 0x104a20 in run (app)");
// zig.valid == true, zig.file == "/app/src/main.zig", zig.line == 51, zig.column == 12, zig.format == 2
```

## 注意事项与边界说明

- **零堆分配与零拷贝**：所有返回的 `string` 与 `StackFrame` 字段均直接切片自输入 `stack`，生命周期与输入一致。
- **多行错误信息兼容**：自动跳过首行至首个缩进 `at ` 帧之间的所有多行错误消息。
- **格式标识**：`StackFrame.format` 取值为 `0`（未知）、`1`（V8/Node.js）、`2`（Zig）。
