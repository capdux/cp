[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zerocopy

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap zero-copy prefix/suffix byte slice cursor readers & writers with layout validation (crates.io zerocopy equivalent, reuses @cap/bytemuck-crate)

## 安装与导入

```bash
cap add zerocopy
```

```cp
import { isAlignedSlice } from "zerocopy";
```

## 核心导出 API

- `export fn isAlignedSlice(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`zerocopy@1.0.0`)，许可证：BSD-2-Clause OR Apache-2.0 OR MIT
