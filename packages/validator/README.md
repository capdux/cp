[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`validator`](https://github.com/validatorjs/validator.js)（`MIT`）（包名 `validator`）。

# validator

Zero-allocation schema & format validation engine for CP — clean-room MIT parity with npm `zod` / `validator` / `ajv` and Rust `validator`.

## Install

```bash
cap add validator
```

## Features

- **Identity & Network**: `isEmail`, `isUrl` (`http`/`https`/`ws`/`wss` + IPv4/IPv6/port), `isUuid`, `isUlid`, `isIpv4`, `isIpv6`, `isCidr`, `isE164Phone`
- **Formats & Dates**: `isSemver` (SemVer 2.0), `isIsoDate` (leap-year aware), `isIsoDateTime` (RFC 3339 / ISO-8601), `isLuhn` (credit cards & IMEI)
- **Strings & Numbers**: `isAscii`, `isAlphanumeric`, `isNumeric`, `isHex`, `isBase64`, `isSlug`, `isOneOf`, `startsWith`, `endsWith`, `containsStr`, `isPort`, `inRangeI64`, `inRangeF64`, `isMultipleOf`
- **Structured Multi-Error Collector**: stack-allocated `Validator` collecting up to 32 `ValidationError{ field, code, message }` entries with zero heap allocations.

## Usage

```cp
import { Validator, isEmail, isSemver } from "validator";

let v = Validator.init();
v.required("username", "alice");
v.email("email", "alice@capdux.org");
v.url("homepage", "https://capdux.dev");
v.semver("version", "1.2.3-rc.1");
v.port("port", 8080);

if (!v.isValid()) {
    // inspect v.errorAt(0).field / .code / .message
}
```
