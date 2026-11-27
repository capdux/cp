[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# jsbarcode

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 1D barcode encoder & SVG renderer (JsBarcode / barcoders parity: EAN-13, UPC-A, Code 128B, Code 39 with checksum verification and crisp SVG output) for CP

## 安装与导入

```bash
cap add jsbarcode
```

```cp
import { ean13Checksum } from "jsbarcode";
```

## 核心导出 API

- `export fn ean13Checksum(digits12: string): i8;`
- `export fn isValidEan13(code: string): bool;`
- `export fn upcAChecksum(digits11: string): i8;`
- `export fn isValidUpcA(code: string): bool;`
- `export fn encodeEan13(code: string): Barcode;`
- `export fn encodeUpcA(code: string): Barcode;`
- `export fn encodeCode128B(text: string): Barcode;`
- `export fn encodeCode39(text: string): Barcode;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`jsbarcode@1.0.0`)，许可证：MIT
