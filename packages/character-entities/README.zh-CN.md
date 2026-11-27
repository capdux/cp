[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# character-entities

> 版本：`2.0.2` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation HTML5 named and numeric character reference decoder and encoder (covers decode-named-character-reference, parse-entities, stringify-entities)

## 安装与导入

```bash
cap add character-entities
```

```cp
import { mapInvalidCharRef } from "character-entities";
```

## 核心导出 API

- `export fn mapInvalidCharRef(cp: u32): u32;`
- `export fn decodeNamedEntity(name: string): u32?;`
- `export fn decodeNumericEntity(body: string): u32?;`
- `export fn decodeEntities(input: string, out: []u8): usize;`
- `export fn encodeEntities(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`character-entities@2.0.2`)（https://github.com/wooorm/character-entities），许可证：MIT
