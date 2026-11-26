# @cap/ini

Zero-heap INI / `.gitconfig` / `.npmrc` / `.editorconfig` parser and writer in pure CP (npm `ini` / Rust `rust-ini` parity).

## Features

- Zero-copy `IniDoc.parse(src)` supporting up to `MAX_ENTRIES = 128` entries
- Supports top-level keys, `[section]` headers, `;`/`#` comments, CRLF/LF line endings, and single/double-quoted values
- Typed getters `get`, `has`, `getBool` (`true`/`yes`/`on`/`1`, `false`/`no`/`off`/`0`), and `getInt` with fallback defaults
- Zero-heap `IniWriter` with automatic quoting for values containing `;`, `#`, or surrounding whitespace

## Usage

```cp
import { IniDoc, IniWriter } from "@cap/ini";

const cfg = "[server]\nport = 8080\ntls = true\n";
const doc = IniDoc.parse(cfg);
const port = doc.getInt(cfg, "server", "port", 3000); // 8080
const tls = doc.getBool(cfg, "server", "tls", false); // true
```
