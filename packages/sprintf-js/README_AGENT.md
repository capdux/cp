# sprintf-js@1.0.0 (Agent Quickref)

> Zero-allocation C-style sprintf formatting engine supporting %s/%d/%i/%u/%x/%X/%o/%b/%f/%c with flags, width, and precision in pure CP | License: BSD-3-Clause
> Import: `import { ... } from "sprintf-js";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum FmtArgKind`

## Functions
- `export fn spfPutByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn spfPutRepeat(out: []u8, pos: usize, b: u8, count: usize): usize;`
- `export fn spfEmitPadded(out: []u8, pos: usize, sign: u8, body: &[]u8, width: usize, left_align: bool, zero_pad: bool): usize;`
- `export fn spfFormatUintRadix(val: u64, base: u64, upper: bool, buf: []u8): usize;`
- `export fn spfArgAsSigned(arg: FmtArg): i64;`
- `export fn spfArgAsUnsigned(arg: FmtArg): u64;`
- `export fn spfArgAsFloat(arg: FmtArg): f64;`
- `export fn spfFormatFloat(val: f64, prec: usize, plus_sign: bool, width: usize, left_align: bool, zero_pad: bool, out: []u8, pos: usize): usize;`
- `export fn sprintf(fmt: string, args: &[]FmtArg, out: []u8): usize;`
