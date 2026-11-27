[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# dotenv-expand

> **Source Attribution**: Clean-room MIT CP implementation of [`npm:dotenv-expand`](https://github.com/motdotla/dotenv-expand) (BSD-2-Clause) | CP Package Version: `1.0.0` | Zero external dependencies

Zero-heap `.env` file parser and POSIX variable expansion engine for CP. Supports `export KEY=VAL`, single/double-quoted values, `#` comments, `$VAR`, `${VAR}`, `${VAR:-default}`, `${VAR-default}`, and escaped `\$` literals.

## Zero-Cost Migration (TypeScript / JS → CP)

Keep the exact same package name (`"dotenv-expand"`) — no renaming required.

| TypeScript / Node.js (`npm:dotenv-expand`) | CP (`dotenv-expand`) |
| :--- | :--- |
| `import dotenv from "dotenv"; import { expand } from "dotenv-expand";` | `import { parseDotenv, expandEnvValue, EnvMap } from "dotenv-expand";` |
| `const parsed = dotenv.parse(src);` | `const env = parseDotenv(src);` |
| `expand({ parsed })` | `expandEnvValue(template, env, out)` |
| `parsed["API_URL"]` | `env.get("API_URL")` |

## API Reference

### Constants

- `DE_MAX_VARS: usize = 32`

### Types & Functions

| Symbol / Signature | Description |
| :--- | :--- |
| `EnvEntry { key: string, value: string }` | Key-value pair borrowing slices from source `.env` text. |
| `EnvMap.new(): EnvMap` | Create an empty 32-slot stack `EnvMap`. |
| `env.len(): usize` / `env.has(key: string): bool` | Query variable count or key existence. |
| `env.set(*_, key: string, val: string): bool` | Insert or update variable (`false` if full at 32 entries). |
| `env.get(key: string): string?` | Look up variable value (`null` if absent). |
| `parseDotenv(src: string): EnvMap` | Parse `.env` text (handles `export `, quotes, `#` comments, CRLF). |
| `expandEnvValue(template: string, env: EnvMap, out: []u8): usize` | Expand POSIX variables in `template` into `out`; returns bytes written. |

## Usage Examples

### Parsing `.env` & Expanding Variables

```cp
import { parseDotenv, expandEnvValue } from "dotenv-expand";

fn demoExpand() {
    const dotenv_src = "HOST=api.example.com\nexport PORT=8443\n# comment\nBASE=\"https://${HOST}:${PORT}\"\n";
    const env = parseDotenv(dotenv_src);

    const base_tpl = env.get("BASE").?;
    let out: [128]u8 = undefined;
    const n = expandEnvValue(base_tpl, env, out[0..]);
    // out[0..n] == "https://api.example.com:8443"
    * = n;
}
```

### Default Fallbacks & Escaped Dollar Signs

```cp
import { EnvMap, expandEnvValue } from "dotenv-expand";

fn demoDefaultsAndEscapes() {
    let env = EnvMap.new();
    * = env.set("EMPTY", "");
    * = env.set("USER", "cp");

    let out: [128]u8 = undefined;
    // ${VAR:-default} triggers when unset OR empty
    const n1 = expandEnvValue("${EMPTY:-fallback}", env, out[0..]);
    // out[0..n1] == "fallback"

    // ${VAR-default} triggers ONLY when unset
    const n2 = expandEnvValue("${EMPTY-fallback}", env, out[0..]);
    // out[0..n2] == ""

    // Escaped \$ emits literal '$'
    const n3 = expandEnvValue("price=\\$100 user=$USER", env, out[0..]);
    // out[0..n3] == "price=$100 user=cp"
    * = n1; * = n2; * = n3;
}
```

## Performance & Safety Highlights

- **Zero Heap Allocation**: `EnvMap` uses `entries: [32]EnvEntry = undefined` on the stack — zero heap allocation and zero init zeroing.
- **Fast `@memcpy` Expansion**: Literal segments and resolved values are copied with `@memcpy` directly into `out: []u8` with bounds clamping.
- **Zero-Copy Parsing**: `parseDotenv` slices keys and values directly from `src` without intermediate string copies.

## License

MIT
