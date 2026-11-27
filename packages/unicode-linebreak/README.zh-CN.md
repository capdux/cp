[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# unicode-linebreak

> 版本：`1.0.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation pure CP UAX #14 Unicode line break opportunity detector (unicode-linebreak parity)

## 安装与导入

```bash
cap add unicode-linebreak
```

```cp
import { ulbrDecodeUtf8 } from "unicode-linebreak";
```

## 核心导出 API

- `export enum BreakKind`
- `export fn ulbrDecodeUtf8(s: string, pos: usize): UlbrDecoded;`
- `export fn ulbrClassify(cp: u32): u8;`
- `export fn ulbrEvalBreak(prev: u8, prevNonSp: u8, curr: u8): u8;`
- `export fn findLineBreaks(utf8: string, out: []LineBreak): usize;`
- `export fn countLineBreaks(utf8: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`unicode-linebreak@1.0.0`)，许可证：Apache-2.0
