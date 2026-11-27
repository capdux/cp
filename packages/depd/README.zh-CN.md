[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# depd

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap deprecation notice formatter and deduplicated warning tracker matching npm depd (150M+/wk) in pure CP

## 安装与导入

```bash
cap add depd
```

```cp
import { depdHashKey } from "depd";
```

## 核心导出 API

- `export fn depdHashKey(ns: string, msg: string): u64;`
- `export fn copySlice(dst: []u8, pos: usize, src: string): usize;`
- `export fn formatDeprecation(namespace: string, msg: string, site: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`depd@1.0.0`)，许可证：MIT
