# @cap/basic-auth

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:basic-auth` ([https://github.com/jshttp/basic-auth](https://github.com/jshttp/basic-auth)) / `crates.io:http-auth-basic` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

Zero-heap RFC 7617 HTTP Basic Authentication parser, encoder, constant-time verifier, and `WWW-Authenticate` challenge formatter in pure CP.

## Features

- `decodeHeader(header, out_user, out_pass)`: case-insensitive `Basic` scheme parsing with built-in zero-heap RFC 4648 Base64 decoder
- `encodeHeader(user, pass, out)`: builds `Basic <base64(user:pass)>` while rejecting colons in `user`
- `constantTimeEq(a, b)` & `verifyHeader(header, expected_user, expected_pass)`: constant-time credential verification
- `formatChallenge(realm, utf8, out)`: formats `Basic realm="...", charset="UTF-8"` with `"` and `\` escaping

## Usage

```cp
import { verifyHeader, formatChallenge } from "@cap/basic-auth";

const ok = verifyHeader("Basic dXNlcjpwYXNz", "user", "pass"); // true
```
