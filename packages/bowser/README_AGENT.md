# bowser@1.0.0 (Agent Quickref)

> Zero-allocation User-Agent parser for browser, OS, engine, and platform detection (npm bowser parity) in pure CP | License: MIT
> Import: `import { ... } from "bowser";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum PlatformType`

## Functions
- `export fn bwsToLower(c: u8): u8;`
- `export fn bwsIsVersionChar(c: u8): bool;`
- `export fn bwsFindSubstr(haystack: string, needle: string): usize?;`
- `export fn bwsFindSubstrCI(haystack: string, needle: string): usize?;`
- `export fn bwsContains(haystack: string, needle: string): bool;`
- `export fn bwsContainsCI(haystack: string, needle: string): bool;`
- `export fn bwsExtractVersionAt(ua: string, start: usize): string;`
- `export fn bwsExtractAfter(ua: string, token: string): string;`
- `export fn bwsDetectBrowser(ua: string): BwsPair;`
- `export fn bwsDetectOS(ua: string): BwsPair;`
- `export fn bwsDetectEngine(ua: string): string;`
- `export fn bwsDetectPlatform(ua: string, os_name: string): PlatformType;`
- `export fn parseUserAgent(ua: string): ParsedUA;`
