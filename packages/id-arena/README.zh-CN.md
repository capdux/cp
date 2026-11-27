[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# id-arena

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation typed handle arena allocator with arena_id cross-arena misuse protection in pure CP

## 安装与导入

```bash
cap add id-arena
```

```cp
import { idaIsValid } from "id-arena";
```

## 核心导出 API

- `export fn idaIsValid(id: ArenaId, expected_arena_id: u32, count: usize): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`id-arena@1.0.0`)，许可证：MIT OR Apache-2.0
