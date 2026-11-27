# colorette@1.0.0 (Agent Quickref)

> Zero-heap pure CP terminal ANSI color formatter with nested close-code recovery (clean-room MIT implementation of npm colorette v2) | License: MIT
> Import: `import { ... } from "colorette";` | Full types: `src/index.d.cp`
> Deps: `ansi-styles`

## Functions
- `export fn isColorSupported(no_color: string, force_color: string, term: string, is_tty: bool): bool;`
- `export fn createColors(use_color: bool): Colorette;`
- `export fn createColorsWithOptions(opts: ColoretteOptions): Colorette;`
