# git-url-parse@1.0.0 (Agent Quickref)

> Zero-allocation Git repository URL parser and normalizer (SCP SSH, HTTPS, SSH+port, hosted shorthands, GitLab nested groups) for CP | License: MIT
> Import: `import { ... } from "git-url-parse";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum GitProtocol`
- `export enum GitHostKind`

## Functions
- `export fn toLowerAscii(c: u8): u8;`
- `export fn isWs(c: u8): bool;`
- `export fn trimWs(s: string): string;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn startsWithIgnoreCase(s: string, prefix: string): bool;`
- `export fn indexOfChar(s: string, ch: u8): usize?;`
- `export fn lastIndexOfChar(s: string, ch: u8): usize?;`
- `export fn indexOfSubstr(s: string, sub: string): usize?;`
- `export fn hasWhitespace(s: string): bool;`
- `export fn parsePort(s: string): u16?;`
- `export fn detectHostKind(host: string): GitHostKind;`
- `export fn stripDotGit(s: string): string;`
- `export fn isValidSegment(s: string): bool;`
- `export fn parseGitUrl(raw: string): ParsedGitUrl;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn appendU16(out: []u8, pos: usize, val: u16): usize;`
- `export fn toHttpsUrl(parsed: ParsedGitUrl, out: []u8): string;`
- `export fn toSshUrl(parsed: ParsedGitUrl, out: []u8): string;`
- `export fn toBrowseUrl(parsed: ParsedGitUrl, out: []u8): string;`
