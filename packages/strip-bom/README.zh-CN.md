[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# strip-bom

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation UTF-8/UTF-16/UTF-32 BOM detection, zero-copy stripping and idempotent prepending (npm strip-bom parity)

## 安装与导入

```bash
cap add strip-bom
```

```cp
import { detectBom } from "strip-bom";
```

## 核心导出 API

- `export fn detectBom(data: []u8): BomInfo;`
- `export fn hasUtf8Bom(s: string): bool;`
- `export fn stripUtf8Bom(s: string): string;`
- `export fn stripAnyBomOffset(data: []u8): usize;`
- `export fn prependUtf8Bom(s: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`strip-bom@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`strip-bom-buf@1.0.0`)，许可证：MIT
