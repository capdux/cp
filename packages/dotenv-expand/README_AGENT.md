# dotenv-expand@1.0.0 — AI Agent Quick Ref

- **Source**: `npm:dotenv-expand` (`motdotla/dotenv-expand`, BSD-2-Clause) | **Deps**: none
- **Import**: `import { EnvMap, EnvEntry, parseDotenv, expandEnvValue, DE_MAX_VARS } from "dotenv-expand";`
- **Full Types**: `packages/dotenv-expand/src/index.d.cp`

## Constants
```cp
DE_MAX_VARS: usize = 32
```

## Core Types & Functions
```cp
class EnvEntry { key: string, value: string }
class EnvMap {
    entries: [32]EnvEntry, count: usize,
    fn new(): EnvMap
    fn len(): usize
    fn set(_: auto, key: string, val: string): bool
    fn get(key: string): string?
    fn has(key: string): bool
}

fn parseDotenv(src: string): EnvMap
fn expandEnvValue(template: string, env: EnvMap, out: []u8): usize
```

## Minimal Pattern
```cp
import { parseDotenv, expandEnvValue } from "dotenv-expand";

const env = parseDotenv("HOST=localhost\nURL=http://${HOST}:${PORT:-8080}\n");
let out: [128]u8 = undefined;
const n = expandEnvValue(env.get("URL").?, env, out[0..]); // "http://localhost:8080"
```

## Pitfalls & Notes
1. **Zero-Copy Lifetime**: `EnvEntry.key` and `EnvEntry.value` borrow from `src` passed to `parseDotenv`; keep `src` alive while reading `env`.
2. **Capacity Cap**: `EnvMap` stores at most `DE_MAX_VARS` (32) variables; `set()` updates existing keys in place and returns `false` if inserting a 33rd unique key.
3. **Single-Pass Expansion**: `expandEnvValue` performs single-pass expansion (no recursive re-expansion of expanded values), preventing circular reference loops.
4. **Syntax Support**: Supports `$VAR`, `${VAR}`, `${VAR:-default}` (unset or empty), `${VAR-default}` (unset only), and `\$` (literal `$`).
5. **Output Truncation**: If `out` is smaller than the expanded string, output is safely clamped to `out.len` without panic.
