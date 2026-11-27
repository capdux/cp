# ua-parser-js@1.0.0 (Agent Quickref)

> Zero-allocation User-Agent parser with Browser/OS/Device/Engine detection, Search & AI Bot classification (GPTBot/ClaudeBot/PerplexityBot/Bytespider), and CLI runtime identification for CP | License: MIT
> Import: `import { ... } from "ua-parser-js";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum DeviceKind`
- `export enum BrowserKind`
- `export enum OsKind`

## Functions
- `export fn toLowerAscii(c: u8): u8;`
- `export fn findSubstr(haystack: string, needle: string): usize?;`
- `export fn findSubstrCI(haystack: string, needle: string): usize?;`
- `export fn contains(haystack: string, needle: string): bool;`
- `export fn containsCI(haystack: string, needle: string): bool;`
- `export fn isVersionChar(c: u8): bool;`
- `export fn extractVersionAfter(ua: string, prefix: string): string;`
- `export fn extractVersionAfterCI(ua: string, prefix: string): string;`
- `export fn matchAiBot(ua: string): BotMatch;`
- `export fn matchAnyBot(ua: string): BotMatch;`
- `export fn parseOs(ua: string, out: &mut UserAgent);`
- `export fn isGenericCli(ua: string): bool;`
- `export fn parseBrowserAndDevice(ua: string, out: &mut UserAgent);`
- `export fn parseUserAgent(ua: string): UserAgent;`
- `export fn isBot(ua: string): bool;`
- `export fn isAiBot(ua: string): bool;`
- `export fn isMobile(ua: string): bool;`
