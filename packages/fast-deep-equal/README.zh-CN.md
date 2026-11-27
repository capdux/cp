[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fast-deep-equal

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 fast-deep-equal

## 安装与导入

```bash
cap add fast-deep-equal
```

```cp
import { isWs } from "fast-deep-equal";
```

## 核心导出 API

- `export fn isWs(ch: u8): bool;`
- `export fn skipWs(s: string, pos: usize): usize;`
- `export fn matchLit(s: string, pos: usize, lit: string): bool;`
- `export fn hexDigit(ch: u8): i32;`
- `export fn invalidSpan(): ValueSpan;`
- `export fn scanString(s: string, pos: usize): ValueSpan;`
- `export fn scanNumber(s: string, pos: usize): ValueSpan;`
- `export fn parseValueSpan(s: string, pos: usize, depth: u32): ValueSpan;`
- `export fn kindOfJson(s: string): u8;`
- `export fn isPlainObjectJson(s: string): bool;`
- `export fn parseJsonNumberF64(s: string): f64;`
- `export fn nextDecodedChar(s: string, pos: usize): DecChar;`
- `export fn jsonStringsEqual(a: string, b: string): bool;`
- `export fn countObjectMembers(obj: string): usize;`
- `export fn arraysDeepEqual(a: string, b: string, depth: u32): bool;`
- `export fn objectsDeepEqual(a: string, b: string, depth: u32): bool;`
- `export fn valuesDeepEqual(a: string, b: string, kind: u8, depth: u32): bool;`
- `export fn jsonDeepEqual(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`fast-deep-equal@1.0.0`)，许可证：MIT
