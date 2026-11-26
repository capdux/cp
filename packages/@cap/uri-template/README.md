# @cap/uri-template

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:url-template` ([https://github.com/bramstein/url-template](https://github.com/bramstein/url-template))（原库名 `url-template` → CP 包名 `@cap/uri-template`）

Zero-allocation RFC 6570 URI Template (Levels 1-4) expander, validator, and variable inspector for CP.

## Features

- **Zero heap allocation**: expands directly into a caller-provided `[]u8` buffer.
- **RFC 6570 Levels 1-4**:
  - Level 1 simple expansion: `{var}` with RFC 6570 percent-encoding.
  - Level 2 reserved `{+path}` and fragment `{#frag}` expansion.
  - Level 3 multi-variable `{x,y}`, label `{.ext}`, path `{/list}`, path params `{;x,y}`, query `{?x,y}`, and query continuation `{&x,y}`.
  - Level 4 UTF-8 code-point prefix truncation `{var:3}`.
- **Inspection & validation**: `isValid`, `countVars`, `extractVarName`, and `pctEncode`.

## Usage

```cp
import { TemplateVar, expand, isValid } from "@cap/uri-template";

let vars: [2]TemplateVar = undefined;
vars[0] = TemplateVar{ name: "owner", value: "cap-lang" };
vars[1] = TemplateVar{ name: "repo", value: "cp" };

let buf: [128]u8 = undefined;
const n = expand("https://api.example.com/repos/{owner}/{repo}", vars[0..2], buf[0..128]);
// buf[0..n] == "https://api.example.com/repos/cap-lang/cp"
```
