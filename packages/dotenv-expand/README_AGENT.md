# dotenv-expand@1.0.0 (Agent Quickref)

> Zero-heap .env parser and POSIX variable expansion engine (${VAR}, ${VAR:-default}, $VAR, escaped \$) (clean-room port of npm dotenv + dotenv-expand) | License: MIT
> Import: `import { ... } from "dotenv-expand";` | Full types: `src/index.d.cp`

## Functions
- `export fn deIsVarChar(b: u8): bool;`
- `export fn parseDotenv(src: string): EnvMap;`
- `export fn expandEnvValue(template: string, env: EnvMap, out: []u8): usize;`
