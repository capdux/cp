# fast-redact@1.0.0 (Agent Quickref)

> Zero-allocation PII & secret redaction/masking engine (fast-redact / maskdata / secrecy parity: emails, Luhn credit cards, JWT/API tokens, phones, IPv4, and sensitive JSON fields) for CP | License: MIT
> Import: `import { ... } from "fast-redact";` | Full types: `src/index.d.cp`

## Functions
- `export fn maskEmail(email: string, out: []u8): string;`
- `export fn maskPhone(phone: string, out: []u8): string;`
- `export fn maskCard(card: string, out: []u8): string;`
- `export fn maskSecret(secret: string, out: []u8): string;`
- `export fn maskIp(ip: string, out: []u8): string;`
- `export fn redactText(input: string, opts: RedactOptions, out: []u8): string;`
- `export fn containsSensitive(input: string): bool;`
- `export fn redactJsonFields(json_str: string, out: []u8): string;`
