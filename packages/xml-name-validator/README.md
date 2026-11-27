[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/xml-name-validator

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:xml-name-validator` ([https://github.com/jsdom/xml-name-validator](https://github.com/jsdom/xml-name-validator), `Apache-2.0`)，同步覆盖 `npm:xmlchars` 字符类判定规则。

Zero-heap-allocation W3C XML 1.0 (5th Edition) and Namespaces in XML 1.0 character & identifier validator in pure CP.

## Features

- `isXmlChar(cp: u32): bool` — W3C XML 1.0 §2.2 `Char` production (`0x09`/`0x0A`/`0x0D`, `0x20..0xD7FF`, `0xE000..0xFFFD`, `0x10000..0x10FFFF`)
- `isNameStartChar(cp: u32): bool` — W3C XML 1.0 §2.3 `NameStartChar` production
- `isNameChar(cp: u32): bool` — W3C XML 1.0 §2.3 `NameChar` production
- `isValidXmlName(s: string): bool` — strict UTF-8 decoded XML `Name` validation
- `isValidNCName(s: string): bool` — Namespaces in XML 1.0 `NCName` (colon-free `Name`) validation
- `isValidQName(s: string): bool` — Namespaces in XML 1.0 `QName` (`NCName` or `NCName:NCName`) validation

## License

Apache-2.0
