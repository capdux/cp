[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ksuid

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Segment KSUID (20-byte K-Sortable Unique IDentifier) 27-character Base62 encoder, decoder, validator, and comparator.

## 安装与导入

```bash
cap add ksuid
```

```cp
import { b62Char } from "ksuid";
```

## 核心导出 API

- `export fn b62Char(v: u8): u8;`
- `export fn b62Val(c: u8): i32;`
- `export fn fromParts(unix_secs: u64, payload: [16]u8): Ksuid;`
- `export fn timestampSecs(k: Ksuid): u64;`
- `export fn encode(k: Ksuid, out: []u8): usize;`
- `export fn decode(s: string, out: &mut Ksuid): bool;`
- `export fn isValidKsuid(s: string): bool;`
- `export fn compare(a: Ksuid, b: Ksuid): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ksuid@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`svix-ksuid@1.0.0`)，许可证：MIT
