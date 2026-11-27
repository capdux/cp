# vte@1.0.0 (Agent Quickref)

> Zero-allocation ANSI/VT100 terminal escape sequence state machine parser (Paul Williams model) in pure CP | License: Apache-2.0
> Import: `import { ... } from "vte";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum VteActionKind`
- `export enum VteState`

## Functions
- `export fn vtepZeroParams(): [8]u16;`
- `export fn vtepZeroOsc(): [64]u8;`
- `export fn vtepIsExecute(b: u8): bool;`
- `export fn vtepAddDigit(cur: u16, digit: u8): u16;`
- `export fn vtepParseOscCmd(buf: &[]u8, len: usize): u16;`
- `export fn vtepHasOscCmd(buf: &[]u8, len: usize): bool;`
- `export fn vtepMakeEvent(kind: VteActionKind, ch: u8, final_byte: u8, param_count: usize, params: [8]u16, intermediate: u8): VteEvent;`
