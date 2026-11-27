[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# chardet

> 版本：`2.1.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation character encoding detector (BOM, UTF-8, UTF-16/32, ASCII, Shift-JIS, GBK, Windows-1252, ISO-8859-1)

## 安装与导入

```bash
cap add chardet
```

```cp
import { charsetName } from "chardet";
```

## 核心导出 API

- `export enum DetectedCharset`
- `export fn charsetName(cs: DetectedCharset): string;`
- `export fn detect(data: &[]u8): CharsetMatch;`
- `export fn detectName(data: &[]u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`chardet@2.1.0`)（https://github.com/runk/node-chardet），许可证：MIT
