[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @opentelemetry/core

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation W3C Baggage (OpenTelemetry) HTTP header parser, percent-codec, and serializer in pure CP

## 安装与导入

```bash
cap add @opentelemetry/core
```

```cp
import { isSpace } from "@opentelemetry/core";
```

## 核心导出 API

- `export fn isSpace(b: u8): bool;`
- `export fn isTchar(b: u8): bool;`
- `export fn isValidKey(key: string): bool;`
- `export fn trimSlice(s: string): string;`
- `export fn strEq(a: string, b: string): bool;`
- `export fn parseSingleMember(seg: string, out: &mut BaggageMember): bool;`
- `export fn parseBaggage(header: string, out: []BaggageMember): usize;`
- `export fn countMembers(header: string): usize;`
- `export fn hexVal(b: u8): i32;`
- `export fn percentDecode(src: string, out: []u8, out_len: &mut usize): bool;`
- `export fn getDecodedValue(header: string, target_key: string, out: []u8, out_len: &mut usize): bool;`
- `export fn hexUpper(nibble: u8): u8;`
- `export fn needsPercentEncode(b: u8): bool;`
- `export fn formatBaggage(members: &[]BaggageMember, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`@opentelemetry/core@1.0.0`)，许可证：Apache-2.0
