# @cap/jsonflat

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:flat` ([https://github.com/hughsk/flat](https://github.com/hughsk/flat))（原库名 `flat` → CP 包名 `@cap/jsonflat`）

Zero-allocation JSON flattening (`a.b.0.c`), `dot-prop` path lookup & flat JSON serialization for CP — clean-room MIT parity with npm `flat` and `dot-prop`.

## Install

```bash
cap add @cap/jsonflat
```

## Features

- **Recursive JSON Flattening**: `flattenJson(json)` & `flattenJsonWith(json, '_')` flatten nested objects and arrays into stack-allocated `FlatResult` entries with zero heap allocations.
- **Dot-Prop Lookup**: `getByPath(json, "user.roles.0", out)` & `hasPath(json, "meta.version")`.
- **Depth Guard**: `MAX_JSON_DEPTH = 24` prevents stack exhaustion on malformed inputs.

## Usage

```cp
import { flattenJson, getByPath } from "@cap/jsonflat";

const src = "{\"user\":{\"name\":\"alice\",\"roles\":[\"admin\",\"ops\"]}}";
const flat = flattenJson(src);
// flat.get("user.roles.0") == "admin"
```
