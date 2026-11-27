[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `chalk`

Zero-allocation terminal string styling library for CP — clean-room parity with **npm `chalk` v5**, featuring automatic terminal capability detection (`supports-color`), Truecolor (16M) / ANSI 256 / ANSI 16 / plain-text color downsampling, and composable foreground, background, and modifier styles.

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registry** | `npm:chalk` ([chalk/chalk](https://github.com/chalk/chalk), v5 parity) |
| **CP Package** | `chalk@1.0.0` (`MIT`) |
| **Dependencies** | `ansi-styles@1.0.0`, `supports-color@1.0.0` |

Keep the exact original package name — no renaming required:

```ts
// TypeScript (npm chalk)
import chalk from "chalk";
console.log(chalk.red("Error!") + " " + chalk.hex("#FF8800")("Warn"));
```

```cp
// CP — identical package name, zero-heap caller buffer
import { Chalk } from "chalk";

const c = Chalk.new(3); // Level 3 = Truecolor (16M)
let buf: [128]u8 = undefined;
const n1 = c.red("Error!", buf[0..]);
const n2 = c.hex("#FF8800", "Warn", buf[0..]);
```

## Architecture & Zero-Heap Design

- **Zero heap allocations**: Every style method writes ANSI SGR escape sequences directly into caller-owned `out: []u8` and returns exact bytes written.
- **Automatic color-level downsampling**:
  - Level `0` (`COLOR_LEVEL_NONE`): plain text passthrough (zero ANSI bytes emitted)
  - Level `1` (`COLOR_LEVEL_BASIC`): 16-color SGR codes (RGB/hex/keyword downsampled via `ansi-styles`)
  - Level `2` (`COLOR_LEVEL_256`): 256-color SGR (`38;5;n` / `48;5;n`)
  - Level `3` (`COLOR_LEVEL_16M`): 24-bit Truecolor (`38;2;r;g;b` / `48;2;r;g;b`)

## Examples

### 1. Basic Colors, Backgrounds & Text Modifiers

```cp
import { Chalk } from "chalk";

fn demoBasic() {
    const c = Chalk.new(3); // Truecolor level
    let buf: [256]u8 = undefined;

    const n1 = c.green("PASS", buf[0..]);
    const n2 = c.bgRed("FAIL", buf[0..]);
    const n3 = c.bold("Heading", buf[0..]);
    const n4 = c.underline("https://cp-lang.org", buf[0..]);
    const n5 = c.dim("debug trace", buf[0..]);
    _ = n1; _ = n2; _ = n3; _ = n4; _ = n5;
}
```

### 2. Truecolor RGB, Hex, CSS Keywords & Auto Downsampling

```cp
import { Chalk } from "chalk";

fn demoColors() {
    let buf: [256]u8 = undefined;

    // Level 3 emits 24-bit Truecolor (\x1b[38;2;255;136;0m...)
    const tc = Chalk.new(3);
    const n_rgb = tc.rgb(255, 136, 0, "orange", buf[0..]);
    const n_hex = tc.hex("#00D4FF", "cyan-glow", buf[0..]);
    const n_kw = tc.keyword("rebeccapurple", "purple", buf[0..]);
    const n_256 = tc.ansi256(208, "xterm-208", buf[0..]);

    // Level 0 strips all styling cleanly (ideal for CI logs / piped stdout)
    const plain = Chalk.new(0);
    const n_plain = plain.hex("#00D4FF", "cyan-glow", buf[0..]); // "cyan-glow"
    _ = n_rgb; _ = n_hex; _ = n_kw; _ = n_256; _ = n_plain;
}
```

### 3. Environment Detection (`Chalk.fromEnv`) & Combined Styles (`styled`)

```cp
import { Chalk } from "chalk";
import { TerminalEnv } from "supports-color";
import { FG_CYAN } from "ansi-styles";

fn demoEnvAndStyled(env: TerminalEnv, argv: &[]string) {
    // Auto-detects NO_COLOR, FORCE_COLOR, COLORTERM, TERM, and --color / --no-color flags
    const c = Chalk.fromEnv(env, argv);

    // Combine bold + underline + foreground in a single zero-heap write
    let buf: [256]u8 = undefined;
    const n = c.styled("Deployment Ready", true, true, FG_CYAN, buf[0..]);
    _ = n;
}
```

## API Overview

- **Constructors**: `Chalk.new(level: u8)`, `Chalk.fromEnv(env: TerminalEnv, argv: &[]string)`
- **Foreground**: `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`, `gray`
- **Background**: `bgBlack`, `bgRed`, `bgGreen`, `bgYellow`, `bgBlue`, `bgMagenta`, `bgCyan`, `bgWhite`, `bgGray`
- **Modifiers**: `bold`, `dim`, `italic`, `underline`, `inverse`, `strikethrough`
- **Extended Colors**: `ansi256`, `bgAnsi256`, `rgb`, `bgRgb`, `hex`, `bgHex`, `keyword`, `bgKeyword`
- **Composite**: `styled(text, is_bold, is_underline, fg, out)`

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
