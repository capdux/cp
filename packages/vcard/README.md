[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/vcard

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:vcard` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

Zero-allocation RFC 6350 vCard 4.0 (`.vcf`) contact formatter and parser for CP.

## Features

- **Zero heap allocation**: formats directly into a caller-supplied `[]u8` buffer and parses into zero-copy byte spans (`ParsedVCard`) referencing the original `.vcf` slice.
- **RFC 6350 compliant formatting**: emits `BEGIN:VCARD\r\nVERSION:4.0\r\n...\r\nEND:VCARD\r\n` with CRLF line endings, automatic escaping of `\`, `,`, `;`, `\n`, and omission of empty optional fields.
- **Flexible parsing**: handles parameterized properties (`TEL;TYPE=work,voice:...`, `EMAIL;TYPE=internet:...`), property group prefixes (`item1.EMAIL:...`), and both CRLF and LF line endings.

## Usage

```cp
import { VCardData, formatVCard, parseVCard } from "@cap/vcard";

const card = VCardData{
    full_name: "Ada Lovelace",
    org: "Analytical Engine Labs",
    title: "Chief Programmer",
    email: "ada@example.com",
    tel: "+1-555-0100",
    url: "https://example.com/ada",
};

let buf: [512]u8 = undefined;
const n = formatVCard(card, buf[0..512]);
const parsed = parseVCard(buf[0..(n as usize)]);
```
