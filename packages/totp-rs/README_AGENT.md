# totp-rs@1.0.0 (Agent Quickref)

> Zero-allocation Google Authenticator otpauth:// TOTP/HOTP Key URI parser, Base32 secret validator, and builder for CP. | License: MIT
> Import: `import { ... } from "totp-rs";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidBase32Secret(secret: string): bool;`
- `export fn parseOtpAuthUri(uri: string): OtpAuthUri;`
- `export fn formatOtpAuthUri(cfg: OtpAuthUri, out: []u8): usize;`
- `export fn splitLabel(label: string): LabelParts;`
