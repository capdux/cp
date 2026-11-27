[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cesu8

> 版本：`1.1.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap CESU-8 and Java Modified UTF-8 (MUTF-8) to/from standard UTF-8 transcoder

## 安装与导入

```bash
cap add cesu8
```

```cp
import { toCesu8 } from "cesu8";
```

## 核心导出 API

- `export fn toCesu8(utf8: &[]u8, modified_nul: bool, out: []u8): i64;`
- `export fn fromCesu8(cesu: &[]u8, modified_nul: bool, out: []u8): i64;`
- `export fn isValidCesu8AsciiOrBmp(utf8: &[]u8, modified_nul: bool): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`cesu8@1.1.0`)（https://github.com/emk/cesu8-rs），许可证：MIT
