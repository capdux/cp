[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# base64ct

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation constant-time (table-free) Base64 encoder and strict canonical decoder (RustCrypto base64ct parity)

## 安装与导入

```bash
cap add base64ct
```

```cp
import { encodedLen } from "base64ct";
```

## 核心导出 API

- `export fn encodedLen(n: usize, padded: bool): usize;`
- `export fn encodeStd(src: []u8, out: []u8): usize;`
- `export fn encodeStdUnpadded(src: []u8, out: []u8): usize;`
- `export fn encodeUrl(src: []u8, out: []u8): usize;`
- `export fn encodeUrlUnpadded(src: []u8, out: []u8): usize;`
- `export fn decodeStd(src: string, out: []u8): DecodeResult;`
- `export fn decodeUrlUnpadded(src: string, out: []u8): DecodeResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`base64ct@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`base64-arraybuffer@1.0.0`)，许可证：MIT
