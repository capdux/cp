[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/expat

> 版本：`2.6.4` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Stream-oriented XML 1.0 SAX parser (libexpat 2.6.4) native C binding for CP

## 安装与导入

```bash
cap add @cap/expat
```

```cp
import { cp_expat_version } from "@cap/expat";
```

## 核心导出 API

- `export fn cp_expat_version(): [*:0]&u8!;`
- `export fn cp_expat_count_elements(xml: [*]&u8!?, len: usize, out_elem_count: *usize!): i32;`
- `export fn versionString(): string;`
- `export fn countElements(xml: &[]u8): !usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`libexpat@2.6.4`)，许可证：MIT
