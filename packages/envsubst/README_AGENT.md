# envsubst — AI Agent Quick Ref

Zero-allocation POSIX / Docker / K8s environment variable template expansion engine (dotenv-expand / envsub / Rust shellexpand parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ExpandError,
    EnvMap,
    ExpandResult,
    isVarStart,
    isVarCont,
    expandImpl,
    expandEnv,
    expandEnvStr,
} from "envsubst";
```

## Core Types & Constants

```cp
enum ExpandError { None, UnclosedBrace, MissingRequired, BufferOverflow }
class EnvMap { keys: [MAX_ENV_VARS]string, vals: [MAX_ENV_VARS]string, len: usize } // methods: init(...) +3
class ExpandResult { ok: bool, err: ExpandError, len: usize, missing_var: string }
```

## API Signatures

```cp
fn isVarStart(c: u8): bool;
fn isVarCont(c: u8): bool;
fn expandImpl(tpl: string, env: EnvMap, out: []u8, start_out: usize, depth: u8): ExpandResult;
fn expandEnv(tpl: string, env: EnvMap, out: []u8): ExpandResult;
fn expandEnvStr(tpl: string, env: EnvMap, out: []u8): string;
fn hasEnvVars(tpl: string): bool;
```

## Minimal Usage

```cp
import { expandEnv } from "envsubst";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = expandEnv("example", /* env: EnvMap */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"envsubst"`; install via `cap add envsubst`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
