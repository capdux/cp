# vte — AI Agent Quick Ref

Zero-allocation ANSI/VT100 terminal escape sequence state machine parser (Paul Williams model) in pure CP (`v1.0.0`, `Apache-2.0`)

## Import

```cp
import {
    VteActionKind,
    VteState,
    VteEvent,
    VteParser,
    vtepZeroParams,
    vtepZeroOsc,
    vtepIsExecute,
    vtepAddDigit,
} from "vte";
```

## Core Types & Constants

```cp
enum VteActionKind { Print, Execute, CsiDispatch, OscDispatch, EscDispatch }
enum VteState { Ground, Escape, CsiEntry, CsiParam, OscString, CsiIgnore, ... }
class VteEvent { kind: VteActionKind, ch: u8, final_byte: u8, param_count: usize, /* +2 fields */ }
class VteParser { state: VteState, params: [8]u16, param_count: usize, cur_param: u16, /* +4 fields */ } // methods: new(...) +6
```

## API Signatures

```cp
fn vtepZeroParams(): [8]u16;
fn vtepZeroOsc(): [64]u8;
fn vtepIsExecute(b: u8): bool;
fn vtepAddDigit(cur: u16, digit: u8): u16;
fn vtepParseOscCmd(buf: &[]u8, len: usize): u16;
fn vtepHasOscCmd(buf: &[]u8, len: usize): bool;
fn vtepMakeEvent(kind: VteActionKind, ch: u8, final_byte: u8, param_count: usize, params: [8]u16, intermediate: u8): VteEvent;
```

## Minimal Usage

```cp
import { vtepZeroParams } from "vte";

pub fn main() {
    let result = vtepZeroParams();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"vte"`; install via `cap add vte`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
