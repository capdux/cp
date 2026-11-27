# http-digest-client@1.0.0 (Agent Quickref)

> Zero-allocation RFC 7616 / RFC 2617 HTTP Digest Authentication challenge & authorization parser and formatter in pure CP | License: MIT
> Import: `import { ... } from "http-digest-client";` | Full types: `src/index.d.cp`

## Functions
- `export fn digIsSpace(b: u8): bool;`
- `export fn trimSlice(s: string): string;`
- `export fn stripDigestPrefix(header: string): string;`
- `export fn unquoteVal(v: string): string;`
- `export fn parseChallenge(header: string): DigestChallenge;`
- `export fn parseAuthorization(header: string): DigestResponse;`
- `export fn supportsQop(qop_list: string, target: string): bool;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn pushQuotedField(dst: []u8, pos: usize, first: bool, key: string, val: string): i64;`
- `export fn pushTokenField(dst: []u8, pos: usize, first: bool, key: string, val: string): i64;`
- `export fn formatAuthorization(resp: DigestResponse, out: []u8): usize;`
- `export fn formatChallenge(ch: DigestChallenge, out: []u8): usize;`
