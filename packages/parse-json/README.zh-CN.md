[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# parse-json

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 8259 JSON diagnostic validator with line/column locations and codeFrame rendering (npm parse-json v8 parity) for CP

## 安装与导入

```bash
cap add parse-json
```

```cp
import { pjsIsWs } from "parse-json";
```

## 核心导出 API

- `export enum JsonDiagKind`
- `export fn pjsIsWs(ch: u8): bool;`
- `export fn pjsIsDigit(ch: u8): bool;`
- `export fn pjsIsHex(ch: u8): bool;`
- `export fn pjsIsIdentStart(ch: u8): bool;`
- `export fn pjsSkipWs(p: &mut PjsParser);`
- `export fn pjsSetErr(p: &mut PjsParser, kind: JsonDiagKind, offset: usize);`
- `export fn pjsLineCol(input: string, offset: usize): PjsLoc;`
- `export fn pjsParseString(p: &mut PjsParser);`
- `export fn pjsParseNumber(p: &mut PjsParser);`
- `export fn pjsExpectKeyword(p: &mut PjsParser, kw: string);`
- `export fn pjsParseValue(p: &mut PjsParser, depth: usize): u8;`
- `export fn validateJsonWithDiag(input: string): JsonDiagResult;`
- `export fn pjsDiagMessage(kind: JsonDiagKind): string;`
- `export fn pjsWriteByte(w: &mut PjsWriter, b: u8);`
- `export fn pjsWriteStr(w: &mut PjsWriter, s: string);`
- `export fn pjsNumWidth(n: usize): usize;`
- `export fn pjsWriteUsize(w: &mut PjsWriter, n: usize);`
- `export fn pjsWriteSpaces(w: &mut PjsWriter, count: usize);`
- `export fn pjsFindLineSpan(input: string, target_line: usize): PjsLineSpan;`
- `export fn pjsWriteFrameLine(w: &mut PjsWriter, input: string, line_no: usize, span: PjsLineSpan, gutter_w: usize, is_err: bool);`
- `export fn formatJsonError(input: string, diag: JsonDiagResult, filename: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`parse-json@1.0.0`)，许可证：MIT
