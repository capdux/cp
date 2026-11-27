[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# shortid

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

URL-friendly non-sequential short unique ID generator and validator (clean-room CP implementation of npm shortid)

## 安装与导入

```bash
cap add shortid
```

```cp
import { sidU64 } from "shortid";
```

## 核心导出 API

- `export fn sidU64(hi: u32, lo: u32): u64;`
- `export fn sidCharIndex(c: u8, alphabet: string): i32;`
- `export fn isValidShortId(id: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`shortid@1.0.0`)，许可证：MIT
