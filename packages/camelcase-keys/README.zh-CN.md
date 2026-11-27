[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# camelcase-keys

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 camelcase-keys

## 安装与导入

```bash
cap add camelcase-keys
```

```cp
import { ckIsSpace } from "camelcase-keys";
```

## 核心导出 API

- `export fn ckIsSpace(ch: u8): bool;`
- `export fn ckWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn ckWriteSlice(out: []u8, pos: usize, src: &[]u8): usize;`
- `export fn ckConvertJsonKeys(json: string, mode: u8, out: []u8): usize;`
- `export fn convertJsonKeysToCamel(json: string, out: []u8): usize;`
- `export fn convertJsonKeysToSnake(json: string, out: []u8): usize;`
- `export fn convertJsonKeysToPascal(json: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`camelcase-keys@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`decamelize-keys@1.0.0`)，许可证：MIT
