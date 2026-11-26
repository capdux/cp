# @cap/json-repair

Zero-allocation malformed & LLM-generated JSON repair engine for CP — clean-room MIT parity with npm `jsonrepair` / `dirty-json` and Rust `llm_json`.

## Install

```bash
cap add @cap/json-repair
```

## Repairs Supported

- Strips Markdown ```` ```json ... ``` ```` code fences from LLM responses
- Strips `//` line comments and `/* ... */` block comments
- Converts single-quoted strings (`'hello'`) to valid double-quoted JSON strings
- Quotes unquoted object keys (`{name: "alice"}` → `{"name":"alice"}`)
- Removes trailing commas (`[1, 2,]` → `[1,2]`)
- Normalizes Python/JS literals (`True`/`False`/`None`/`undefined` → `true`/`false`/`null`)
- Auto-closes truncated brackets/braces from interrupted LLM streams

## Usage

```cp
import { repairJson } from "@cap/json-repair";

let out: [512]u8 = undefined;
const fixed = repairJson("```json\n{user: 'alice', active: True, tags: ['ai',]}\n```", out[0..512]);
// {"user":"alice","active":true,"tags":["ai"]}
```
