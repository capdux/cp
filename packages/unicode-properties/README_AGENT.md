# unicode-properties@1.0.0 (Agent Quickref)

> Zero-allocation Unicode character properties, UAX #31 identifier validation (XID_Start/XID_Continue), White_Space, Default_Ignorable, and UTS #51 Emoji classification in pure CP. | License: MIT
> Import: `import { ... } from "unicode-properties";` | Full types: `src/index.d.cp`

## Functions
- `export fn decodeUtf8Strict(s: string, pos: usize, next_pos: &mut usize): u32;`
- `export fn isXidStart(cp: u32): bool;`
- `export fn isXidContinue(cp: u32): bool;`
- `export fn isUnicodeWhitespace(cp: u32): bool;`
- `export fn isEmojiPresentation(cp: u32): bool;`
- `export fn isEmoji(cp: u32): bool;`
- `export fn isDefaultIgnorable(cp: u32): bool;`
- `export fn isValidIdentifier(s: string): bool;`
