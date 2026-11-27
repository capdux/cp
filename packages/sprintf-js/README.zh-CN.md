[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sprintf-js

> 版本：`1.0.0` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation C-style sprintf formatting engine supporting %s/%d/%i/%u/%x/%X/%o/%b/%f/%c with flags, width, and precision in pure CP

## 安装与导入

```bash
cap add sprintf-js
```

```cp
import { spfPutByte } from "sprintf-js";
```

## 核心导出 API

- `export enum FmtArgKind`
- `export fn spfPutByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn spfPutBytes(out: []u8, pos: usize, s: &[]u8): usize;`
- `export fn spfPutRepeat(out: []u8, pos: usize, b: u8, count: usize): usize;`
- `export fn spfEmitPadded(out: []u8, pos: usize, sign: u8, body: &[]u8, width: usize, left_align: bool, zero_pad: bool): usize;`
- `export fn spfFormatUintRadix(val: u64, base: u64, upper: bool, buf: []u8): usize;`
- `export fn spfArgAsSigned(arg: FmtArg): i64;`
- `export fn spfArgAsUnsigned(arg: FmtArg): u64;`
- `export fn spfArgAsFloat(arg: FmtArg): f64;`
- `export fn spfFormatFloat(val: f64, prec: usize, plus_sign: bool, width: usize, left_align: bool, zero_pad: bool, out: []u8, pos: usize): usize;`
- `export fn sprintf(fmt: string, args: &[]FmtArg, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`sprintf-js@1.0.0`)，许可证：BSD-3-Clause
