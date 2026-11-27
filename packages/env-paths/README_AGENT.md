# env-paths@1.0.0 (Agent Quickref)

> Zero-allocation cross-platform application directory resolver (Linux XDG, macOS Library, Windows AppData) in pure CP | License: MIT
> Import: `import { ... } from "env-paths";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum EnvPlatform`

## Functions
- `export fn evpTrimTrailingSep(s: string, sep: u8): string;`
- `export fn evpAppend(out: []u8, pos: usize, s: string): usize;`
- `export fn evpAppendByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn evpAppendAppName(out: []u8, pos: usize, name: string, suffix: string): usize;`
- `export fn evpJoinUnix2(out: []u8, base: string, name: string, suffix: string): usize;`
- `export fn evpJoinUnix3(out: []u8, base: string, mid: string, name: string, suffix: string): usize;`
- `export fn evpWinAppBase(out: []u8, explicit: string, home: string, fallback_sub: string): usize;`
- `export fn evpWinDir(out: []u8, explicit: string, home: string, fallback_sub: string, name: string, suffix: string, leaf: string): usize;`
- `export fn envDataDir(name: string, inp: EnvPathsInput, out: []u8): usize;`
- `export fn envConfigDir(name: string, inp: EnvPathsInput, out: []u8): usize;`
- `export fn envCacheDir(name: string, inp: EnvPathsInput, out: []u8): usize;`
- `export fn envLogDir(name: string, inp: EnvPathsInput, out: []u8): usize;`
- `export fn envTempDir(name: string, inp: EnvPathsInput, out: []u8): usize;`
