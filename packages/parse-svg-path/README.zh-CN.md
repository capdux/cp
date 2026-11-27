[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# parse-svg-path

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation SVG path `d` attribute parser, bounding box calculator, and canonical formatter for CP

## 安装与导入

```bash
cap add parse-svg-path
```

```cp
import { isWs } from "parse-svg-path";
```

## 核心导出 API

- `export fn isWs(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn skipWs(s: string, start: usize): usize;`
- `export fn skipWsComma(s: string, start: usize): usize;`
- `export fn expectedArgCount(code: u8): u8?;`
- `export fn isLowerCmd(code: u8): bool;`
- `export fn isNumberStart(c: u8): bool;`
- `export fn pow10(exp: i32): f64;`
- `export fn parseSvgNumber(s: string, start: usize): ParsedNum;`
- `export fn parseSvgFlag(s: string, start: usize): ParsedNum;`
- `export fn parseSvgPath(d: string): ParsedPath;`
- `export fn includePoint(acc: BoundsAcc, x: f64, y: f64): BoundsAcc;`
- `export fn computeBounds(p: ParsedPath): PathBounds;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn appendFloatCompact(out: []u8, start_pos: usize, val: f64): usize;`
- `export fn formatSvgPath(p: ParsedPath, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`parse-svg-path@1.0.0`)，许可证：MIT
