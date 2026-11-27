[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# file-entry-cache

> 版本：`10.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap incremental build and lint file metadata cache in pure CP (covers flat-cache)

## 安装与导入

```bash
cap add file-entry-cache
```

```cp
import { fecStrEq } from "file-entry-cache";
```

## 核心导出 API

- `export fn fecStrEq(a: string, b: string): bool;`
- `export fn fecEmptyEntry(): FileCacheEntry;`
- `export fn fecPathExists(path: string, existing_paths: &[]string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`file-entry-cache@10.0.0`)（https://github.com/jaredwray/cacheable），许可证：MIT
