# @cap/dotenv-expand

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:dotenv-expand` ([https://github.com/motdotla/dotenv-expand](https://github.com/motdotla/dotenv-expand))

Clean-room MIT, zero-heap `.env` parser and POSIX variable expansion engine for CP — modeled on npm's `dotenv` + `dotenv-expand`.

## API
- `EnvMap` (`new`, `len`, `set`, `get`, `has`)
- `parseDotenv(src): EnvMap` — parses `.env` lines, `export` prefixes, `#` comments, single/double quotes
- `expandEnvValue(template, env, out): usize` — expands `$VAR`, `${VAR}`, `${VAR:-default}`, `${VAR-default}`, and escaped `\$`
