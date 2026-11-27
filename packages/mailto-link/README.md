[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `mailto-link`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mailto-link` ([https://github.com/sindresorhus/mailto-link](https://github.com/sindresorhus/mailto-link)) / `crates.io:mailto` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `mailto-link` → CP 包名 `mailto-link`）

Zero-heap-allocation RFC 6068 `mailto:` URI parser, recipient counter, email validator, and percent-encoding builder in pure CP.

## Features

- **`parseMailto`**: Case-insensitive `mailto:` parser extracting `to`, `cc`, `bcc`, `subject`, and `body` (including `?to=` fallback).
- **`countRecipients` & `isBasicEmail`**: Zero-copy comma-separated recipient counting and lightweight email syntax validation.
- **`formatMailto`**: Serializes `mailto:<to>?subject=...&body=...&cc=...` with RFC 3986 `%20` percent-encoding.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    MailtoUri,
    parseMailto,
    countRecipients,
    formatMailto,
    isBasicEmail,
} from "mailto-link";
```

## License

MIT
