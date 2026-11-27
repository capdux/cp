[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# string_decoder

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation streaming multi-byte UTF-8/UTF-16/Latin1/Base64 string decoder preserving incomplete boundary bytes across chunks (clean-room MIT implementation of npm string_decoder, #62 / 378M wk)

## 安装与导入

```bash
cap add string_decoder
```

```cp
import { example } from "string_decoder";
```

## 核心导出 API

- `export enum DecoderEncoding`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`string_decoder@1.0.0`)，许可证：MIT
