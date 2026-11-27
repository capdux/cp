[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `y18n`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:y18n` ([https://github.com/yargs/y18n](https://github.com/yargs/y18n))

Zero-heap-allocation internationalization (i18n) catalog and printf-style formatter in pure CP (clean-room MIT implementation inspired by npm `y18n`).

## Features

- **`Y18nCatalog`** — Fixed-capacity (64 entries) translation catalog supporting `new(locale)`, `setLocale` / `getLocale`, `setEntry`, `setPluralEntry`, `hasEntry`, and `clear`.
- **`t(key, args, out)` (`__`)** — Looks up translation by key (falling back to `key` if missing) and substitutes `%s` arguments with `%%` literal escape support.
- **`tn(singular, plural, count, args, out)` (`__n`)** — Selects singular template when `count == 1` and plural otherwise, injecting `%d` (supports zero, positive, and negative integers) and `%s` positional arguments.
- **`formatPrintf(tmpl, count, has_count, args, out)`** — Standalone zero-allocation formatter with strict output buffer bounds protection.

## Usage

```cp
import { Y18nCatalog, formatPrintf } from "y18n";
```

## License

MIT
