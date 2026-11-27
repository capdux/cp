[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/ldap-dn

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ldapjs` ([https://github.com/ldapjs/node-ldapjs](https://github.com/ldapjs/node-ldapjs)) / `crates.io:ldap3` ([https://github.com/inejge/ldap3](https://github.com/inejge/ldap3))（原库名 `ldapjs` → CP 包名 `@cap/ldap-dn`）

Zero-heap RFC 4514 LDAP Distinguished Name parser, value escaper/unescaper, validator, and case-insensitive attribute lookup in pure CP.

## Features

- `parseDn(dn, out)`: splits unescaped `,` RDNs respecting `\,`, `\=`, `\\`, and `\XX` escapes
- `escapeValue(value, out)` & `unescapeValue(raw, out)`: full RFC 4514 Section 2.4 escaping and hex/char unescaping
- `findAttr(dn, target_attr, out, &out_len)`: case-insensitive lookup with automatic unescaping
- `isValidDn(dn)`: strict syntax validation

## Usage

```cp
import { findAttr } from "@cap/ldap-dn";

let val: [64]u8 = undefined;
let len: usize = 0;
const ok = findAttr("CN=John\\, Doe,OU=Eng,DC=example,DC=com", "cn", val[0..64], &len);
// val[0..len] => "John, Doe"
```
