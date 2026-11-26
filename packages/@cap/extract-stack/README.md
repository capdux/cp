# @cap/extract-stack

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:extract-stack` ([https://github.com/sindresorhus/extract-stack](https://github.com/sindresorhus/extract-stack))

Zero-allocation & zero-copy `Error.stack` frame extractor for CP — clean-room MIT parity with npm `extract-stack` v2.

纯 CP 零堆分配、零拷贝错误堆栈提取库（对标 npm `extract-stack` v2），自动跳过单行或多行 `Error: message` 错误头，从首个 `    at ...` 行提取纯栈帧文本、解析独立帧数组或分离错误头。

## Install / 安装

```bash
cap add @cap/extract-stack
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `extractStack` | `(stack: string): string` | Zero-copy slice of stack frames starting at the first `    at ...` line |
| `extractStackLines` | `(stack: string, out: []string): usize` | Extracts frame lines with leading whitespace and `at ` stripped into `out` |
| `extractErrorHeader` | `(stack: string): string` | Zero-copy slice of the error header preceding the stack frames |

## Usage / 使用示例

```cp
import { extractStack, extractStackLines, extractErrorHeader } from "@cap/extract-stack";

const raw = "Error: boom\n    at foo (app.js:10:5)\n    at main (app.js:20:1)";
const header = extractErrorHeader(raw); // "Error: boom"
const frames = extractStack(raw);       // "    at foo (app.js:10:5)\n    at main (app.js:20:1)"

let lines: [8]string = undefined;
const n = extractStackLines(raw, lines[0..8]); // ["foo (app.js:10:5)", "main (app.js:20:1)"]
```
