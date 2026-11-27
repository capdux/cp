[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# simdutf8

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap strict RFC 3629 UTF-8 validator with ASCII fast-path, basic/compat validation, valid_up_to & error_len reporting (crates.io simdutf8 equivalent)

## 安装与导入

```bash
cap add simdutf8
```

```cp
import { sutfIsCont } from "simdutf8";
```

## 核心导出 API

- `export fn sutfIsCont(b: u8): bool;`
- `export fn isAscii(buf: []u8): bool;`
- `export fn validateUtf8Compat(buf: []u8): Utf8CompatResult;`
- `export fn basicIsValidUtf8(buf: []u8): bool;`
- `export fn countCodepoints(buf: []u8): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`simdutf8@1.0.0`)，许可证：MIT OR Apache-2.0
