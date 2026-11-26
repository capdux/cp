# @cap/csp

Zero-allocation W3C Content-Security-Policy 3 header builder and parser for CP.

## Features

- **Zero heap allocation**: builds directly into a caller-supplied `[]u8` buffer and parses into fixed-size `[32]CspDirectiveSpan` slices referencing the original header.
- **Header injection protection**: `CspBuilder.addDirective` rejects directive names or source values containing `;`, `,`, whitespace, or CR/LF control characters.
- **W3C CSP3 compliant parsing**: case-insensitive directive lookup (`getDirective`, `hasDirective`), first-occurrence precedence for duplicate directives, valueless directives (`upgrade-insecure-requests`), and exact token detection for `'unsafe-inline'` (`allowsUnsafeInline`).

## Usage

```cp
import { CspBuilder, CspPolicy } from "@cap/csp";

let buf: [256]u8 = undefined;
let builder = CspBuilder.init(buf[0..256]);
const selfSrc: [1][]const u8 = ["'self'"];
* = builder.addDirective("default-src", selfSrc[0..1]);
const empty: [0][]const u8 = [];
* = builder.addDirective("upgrade-insecure-requests", empty[0..0]);

const header = builder.written();
const policy = CspPolicy.parse(header);
const defaultSrc = policy.getDirective(header, "DEFAULT-SRC"); // "'self'"
```
