[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# atoi

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap overflow-checked integer parsing directly from byte slices (base 10, 16, 2..36) matching Rust atoi (262M+ downloads) in pure CP

## 安装与导入

```bash
cap add atoi
```

```cp
import { digitVal } from "atoi";
```

## 核心导出 API

- `export fn digitVal(ch: u8, radix: u32): i32;`
- `export fn fromRadixU64(bytes: string, radix: u32): ParseU64Result;`
- `export fn fromRadix10U64(bytes: string): ParseU64Result;`
- `export fn fromRadix16U64(bytes: string): ParseU64Result;`
- `export fn fromRadixI64(bytes: string, radix: u32): ParseI64Result;`
- `export fn fromRadix10I64(bytes: string): ParseI64Result;`
- `export fn atoiU64(s: string): ParseU64Result;`
- `export fn atoiI64(s: string): ParseI64Result;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`atoi@1.0.0`)，许可证：MIT
