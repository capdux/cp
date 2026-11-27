[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cssesc

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CSSOM-compliant CSS identifier and string escaper in pure CP

## 安装与导入

```bash
cap add cssesc
```

```cp
import { defaultCssEscOptions } from "cssesc";
```

## 核心导出 API

- `export enum CssQuoteStyle`
- `export fn defaultCssEscOptions(): CssEscOptions;`
- `export fn cescDecodeUtf8(s: string, start: usize): CescUtf8Res;`
- `export fn cescIsAsciiAlnumOrDash(cp: u32): bool;`
- `export fn cssescBuf(input: string, opts: CssEscOptions, out: []u8): usize;`
- `export fn cssesc(input: string, opts: CssEscOptions): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cssesc@1.0.0`)，许可证：MIT
