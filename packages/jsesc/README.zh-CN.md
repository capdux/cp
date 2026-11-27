[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# jsesc

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation JavaScript & JSON string escaper with ES6 code point, script-context, and minimal modes (clean-room MIT implementation of npm jsesc v3, #148 / 250M wk)

## 安装与导入

```bash
cap add jsesc
```

```cp
import { jescHexChar } from "jsesc";
```

## 核心导出 API

- `export fn jescHexChar(nibble: u32, lowercase: bool): u8;`
- `export fn jescPush(out: []u8, pos: usize, b: u8): usize;`
- `export fn jescPush2(out: []u8, pos: usize, b0: u8, b1: u8): usize;`
- `export fn jescPushHex2(out: []u8, pos: usize, val: u32, lowercase: bool): usize;`
- `export fn jescPushUnicode4(out: []u8, pos: usize, val: u32, lowercase: bool): usize;`
- `export fn jescPushEs6CodePoint(out: []u8, pos: usize, cp: u32, lowercase: bool): usize;`
- `export fn jescDecodeUtf8(input: string, idx: usize): JescDecoded;`
- `export fn jescAsciiLower(b: u8): u8;`
- `export fn jescMatchesHtmlComment(input: string, idx: usize): bool;`
- `export fn jescMatchesCloseScript(input: string, idx: usize): bool;`
- `export fn jescEscapeCodePoint(out: []u8, pos: usize, cp: u32, is_json: bool, use_es6: bool, lowercase: bool): usize;`
- `export fn jsescWithOptions(input: string, opts: JsescOptions, out: []u8): usize;`
- `export fn jsesc(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`jsesc@1.0.0`)，许可证：MIT
