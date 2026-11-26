# @cap/redact

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fast-redact` ([https://github.com/davidmarkclements/fast-redact](https://github.com/davidmarkclements/fast-redact))（原库名 `fast-redact` → CP 包名 `@cap/redact`）

Zero-allocation PII & secret redaction/masking engine for CP — clean-room MIT parity with npm `fast-redact` / `maskdata` and Rust `secrecy`.

## Install

```bash
cap add @cap/redact
```

## Features

- **Single-Value Masking**: `maskEmail`, `maskPhone`, `maskCard`, `maskSecret`, `maskIp`
- **Free-Text Log Redaction**: `redactText(input, opts, out)` automatically detects and masks JWTs (`eyJ...`), API keys (`sk-`, `ghp_`, `cfut_`, `capx_`, `AKIA`), emails, Luhn-verified credit cards, E.164 / CN mobile phones, and IPv4 addresses.
- **Structured JSON Field Redaction**: `redactJsonFields(json, out)` masks `password`, `token`, `secret`, `api_key`, `authorization`, `private_key`, `credit_card`, `ssn`, `cookie` values to `"[REDACTED]"` while preserving valid JSON.
- **Fast Leak Detector**: `containsSensitive(text)` for CI/CD and AI agent output guardrails.

## Usage

```cp
import { RedactOptions, redactText, redactJsonFields } from "@cap/redact";

let out: [512]u8 = undefined;
const clean = redactText("User alice@capdux.org used sk-proj-1234567890abcdef", RedactOptions.defaultAll(), out[0..512]);
```
