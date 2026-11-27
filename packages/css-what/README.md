[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/css-what`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:css-what` ([https://github.com/fb55/css-what](https://github.com/fb55/css-what))

Zero-heap-allocation CSS selector parser and serializer (Cheerio / `css-select` core) in pure CP.

## Features

- **Complete CSS Selector Lexing**: Tags, universal `*`, `#id`, `.class`, `[attr op val flags]`, `:pseudo(args)`, `::pseudo-element`, and combinators (` `, `>`, `+`, `~`, `,`).
- **Attribute Operators & Case Flags**: Supports exists, `=`, `^=`, `$=`, `*=`, `!=`, `~=`, `|=`, quoted/unquoted values, and `i`/`s` flags.
- **Zero Heap Allocation**: Slices directly into the input selector string and writes into caller-provided token/byte buffers.

## API

```cp
import {
    CssTokenKind,
    AttrAction,
    CssSelectorToken,
    parseCssSelector,
    stringifyCssSelector,
} from "@cap/css-what";
```

## License

BSD-2-Clause
