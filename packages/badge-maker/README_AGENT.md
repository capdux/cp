# badge-maker@1.0.0 (Agent Quickref)

> Zero-heap Shields.io-style SVG status badge generator (flat, flat-square, plastic) with XML escaping and named colors | License: MIT
> Import: `import { ... } from "badge-maker";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum BadgeStyle`

## Functions
- `export fn defaultBadge(label: string, message: string, color: string): BadgeOptions;`
- `export fn resolveBadgeColor(name: string): string;`
- `export fn estimateTextWidth(s: string): usize;`
- `export fn renderBadge(opts: BadgeOptions, out: []u8): string;`
- `export fn makeBadge(label: string, message: string, color: string, out: []u8): string;`
