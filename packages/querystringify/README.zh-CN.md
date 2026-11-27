[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# querystringify

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation WHATWG application/x-www-form-urlencoded parser, serializer, and percent-encoding codec for CP

## 安装与导入

```bash
cap add querystringify
```

```cp
import { isFormSafe } from "querystringify";
```

## 核心导出 API

- `export fn isFormSafe(b: u8): bool;`
- `export fn upperHex(v: u8): u8;`
- `export fn hexVal(c: u8): i32;`
- `export fn encodedSize(input: string, space_as_plus: bool): usize;`
- `export fn percentEncode(input: string, space_as_plus: bool, out: []u8): usize;`
- `export fn decodedSize(input: string): usize;`
- `export fn percentDecode(input: string, plus_as_space: bool, out: []u8): usize;`
- `export fn parseFormPairs(query: string, out: []FormPair): usize;`
- `export fn decodedMatches(raw: string, plus_as_space: bool, target: string): bool;`
- `export fn getParamDecoded(query: string, target_key: string, out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`querystringify@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`urlencoding@1.0.0`)，许可证：MIT
