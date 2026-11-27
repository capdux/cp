# postcss-value-parser@1.0.0 (Agent Quickref)

> Zero-allocation CSS dimension, length, angle, time, and resolution parser and unit converter for CP | License: MIT
> Import: `import { ... } from "postcss-value-parser";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum CssUnitKind`

## Functions
- `export fn parseCssDimension(raw: string): CssDimension;`
- `export fn toPixels(dim: CssDimension, ctx: CssContext): f64?;`
- `export fn toDegrees(dim: CssDimension): f64?;`
- `export fn toMilliseconds(dim: CssDimension): f64?;`
- `export fn unitCategory(u: CssUnitKind): string;`
