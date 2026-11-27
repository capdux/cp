[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# adler2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 1950 Adler-32 checksum with streaming, rolling window update, and combine in pure CP (modeled after Rust adler2)

## 安装与导入

```bash
cap add adler2
```

```cp
import { adler32 } from "adler2";
```

## 核心导出 API

- `export fn adler32(data: &[]u8): u32;`
- `export fn combineAdler32(adler1: u32, adler2: u32, len2: u64): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`adler2@1.0.0`)，许可证：MIT
