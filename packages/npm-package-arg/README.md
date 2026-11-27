<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`npm-package-arg`](https://github.com/npm/npm-package-arg)（`ISC`）（原库名 `npm-package-arg`，CP 包名 `@cap/npm-spec`）。

# @cap/npm-spec

Zero-heap npm package name validator and package specifier (`@scope/name@^1.2.3`) parser and formatter in pure CP.

## Features

- `isValidPackageName(name)`: strict npm rules (1..214 chars, lowercase URL-safe, no leading `.` or `_`, valid `@scope/pkg`)
- `parsePackageSpec(input)`: separates scoped leading `@` from version `@` cleanly
- `formatPackageSpec(spec, out)`: serializes `<full_name>[@<version_spec>]`

## Usage

```cp
import { parsePackageSpec } from "@cap/npm-spec";

const s = parsePackageSpec("@cap/http@^1.0.0");
// s.scope => "cap", s.name => "http", s.version_spec => "^1.0.0"
```
