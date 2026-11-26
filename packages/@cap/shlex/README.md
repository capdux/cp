# @cap/shlex

Zero-allocation POSIX shell quoting, command-line lexical splitting & injection guard for CP — clean-room MIT parity with npm `shell-quote` and Rust `shlex`.

## Install

```bash
cap add @cap/shlex
```

## Features

- **Injection-Safe Quoting**: `quoteArg(arg, out)` & `joinArgs(args, out)` safely quote arbitrary strings (including spaces, `$()`, backticks, `;`, `|`, and embedded single quotes via `'\''`).
- **POSIX Lexical Splitter**: `splitShell(cmd)` handles single quotes, double quotes with POSIX backslash rules, unquoted escapes, `#` comments, and reports structured `ShlexError` for unclosed quotes or trailing backslashes.
- **Metacharacter Guard**: `hasShellMetachars(s)` & `needsQuote(s)` for AI agent command validation.

## Usage

```cp
import { quoteArg, joinArgs, splitShell } from "@cap/shlex";

let out: [256]u8 = undefined;
const safe = quoteArg("it's $(rm -rf /)", out[0..256]);
// "'it'\\''s $(rm -rf /)'"

const parsed = splitShell("cap deploy --name 'my agent' \"--tag=v1.0\"");
// parsed.count == 4
```
