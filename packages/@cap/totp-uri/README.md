# @cap/totp-uri

Zero-allocation Google Authenticator Key URI Format (`otpauth://totp/...` and `otpauth://hotp/...`) parser, Base32 secret validator, and URI builder for CP.

## Features

- **Zero heap allocation**: zero-copy URI parsing and caller-supplied buffer formatting.
- **TOTP & HOTP support**: full parsing of `secret`, `issuer`, `algorithm` (`SHA1`/`SHA256`/`SHA512`), `digits` (`6`/`8`), `period` (default `30`), and `counter` (required for HOTP).
- **Base32 & Label helpers**: `isValidBase32Secret`, `splitLabel`, and `formatOtpAuthUri` with automatic percent-encoding.

## Usage

```cp
import { parseOtpAuthUri, formatOtpAuthUri, splitLabel } from "@cap/totp-uri";

const parsed = parseOtpAuthUri("otpauth://totp/Acme:alice@example.com?secret=JBSWY3DPEHPK3PXP&issuer=Acme");
const parts = splitLabel(parsed.label); // parts.issuer == "Acme", parts.account == "alice@example.com"
```
