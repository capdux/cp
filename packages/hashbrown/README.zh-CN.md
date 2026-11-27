[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hashbrown

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap SwissTable-style control-byte hash map and hash set (clean-room CP implementation of crates.io #1 hashbrown)

## 安装与导入

```bash
cap add hashbrown
```

```cp
import { hbHashU64 } from "hashbrown";
```

## 核心导出 API

- `export fn hbHashU64(key: u64): u64;`
- `export fn hbH2(h: u64): u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`hashbrown@1.0.0`)，许可证：MIT
