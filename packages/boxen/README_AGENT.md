# boxen@1.0.0 (Agent Quickref)

> Zero-allocation CLI terminal box renderer with 6 Unicode/ASCII border styles, embedded titles, alignment, CJK double-width & ANSI escape awareness (boxen parity) for CP | License: MIT
> Import: `import { ... } from "boxen";` | Full types: `src/index.d.cp`
> Deps: `unicode-width`

## Types & Constants
- `export enum BorderStyle`
- `export enum TextAlign`

## Functions
- `export fn borderForStyle(style: BorderStyle): BorderChars;`
- `export fn decodeUtf8At(s: string, i: usize): Utf8Step;`
- `export fn codepointWidth(cp: u32): usize;`
- `export fn displayWidth(s: string): usize;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushStr(out: []u8, pos: usize, s: string): usize;`
- `export fn pushRepeat(out: []u8, pos: usize, s: string, count: usize): usize;`
- `export fn pushSpaces(out: []u8, pos: usize, count: usize): usize;`
- `export fn renderBox(text: string, opts: BoxOptions, out: []u8): string;`
- `export fn box(text: string, out: []u8): string;`
- `export fn titledBox(title: string, text: string, style: BorderStyle, out: []u8): string;`
