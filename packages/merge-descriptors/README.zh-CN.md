[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# merge-descriptors

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation property descriptor table and safe descriptor merge engine for CP (clean-room parity with npm merge-descriptors v2)

## 安装与导入

```bash
cap add merge-descriptors
```

```cp
import { mergeDescriptors } from "merge-descriptors";
```

## 核心导出 API

- `export fn mergeDescriptors(dst: &mut DescriptorTable, src: DescriptorTable, redefine: bool): usize;`
- `export fn countEnumerable(tbl: DescriptorTable): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`merge-descriptors@1.0.0`)，许可证：MIT
