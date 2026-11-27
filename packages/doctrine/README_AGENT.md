# doctrine@1.0.0 (Agent Quickref)

> Zero-allocation JSDoc comment parser extracting description and @param/@returns/@type/@deprecated/@throws/@template tags (npm doctrine parity) for CP | License: Apache-2.0
> Import: `import { ... } from "doctrine";` | Full types: `src/index.d.cp`

## Functions
- `export fn docEmptyTag(): JsDocTag;`
- `export fn docIsWs(ch: u8): bool;`
- `export fn docIsSpace(ch: u8): bool;`
- `export fn docStrEq(a: string, b: string): bool;`
- `export fn docTrim(s: string): string;`
- `export fn docStripLinePrefix(line: string): string;`
- `export fn docTagHasName(title: string): bool;`
- `export fn docParseTagLine(line: string): JsDocTag;`
- `export fn parseJsDoc(comment: string): JsDocComment;`
