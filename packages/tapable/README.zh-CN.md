[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# tapable

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation plugin hook engine (SyncHook, SyncBailHook, SyncWaterfallHook, SyncLoopHook) with stage ordering and before insertion (clean-room MIT implementation of npm tapable v2, #242 / 195M wk)

## 安装与导入

```bash
cap add tapable
```

```cp
import { tapEmptyEntry } from "tapable";
```

## 核心导出 API

- `export fn tapEmptyEntry(): TapEntry;`
- `export fn tapStrEq(a: string, b: string): bool;`
- `export fn tapInsertSorted(entries: []TapEntry, count: usize, entry: TapEntry): usize;`
- `export fn tapInsertBefore(entries: []TapEntry, count: usize, name: string, before_name: string, handler_id: u32): usize;`
- `export fn tapSetEnabled(entries: []TapEntry, count: usize, name: string, enabled: bool): bool;`
- `export fn tapCollectOrdered(entries: &[]TapEntry, count: usize, out: []u32): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`tapable@1.0.0`)，许可证：MIT
