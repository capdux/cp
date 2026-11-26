# `@cap/dotenv-expand`

Zero-heap-allocation `.env` environment variable interpolation engine in pure CP.

## Supported Syntax

- `$VAR` and `${VAR}` — Basic variable substitution
- `${VAR:-default}` — Fallback when `VAR` is unset or empty
- `${VAR-default}` — Fallback only when `VAR` is unset (preserves empty string)
- `${VAR:+alt}` — Alternate value when `VAR` is set and non-empty
- Nested fallbacks — e.g. `${A:-${B:-fallback}}`
- `\$` — Escaped literal `$`
- Bounded recursive expansion (`MAX_DEPTH = 8`) with cycle protection

## API

```cp
import { MAX_DEPTH, EnvVar, hasVar, lookupVar, expandValue } from "@cap/dotenv-expand";
```

## License

MIT
