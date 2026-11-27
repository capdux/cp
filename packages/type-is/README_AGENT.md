# type-is — AI Agent Quick Ref

- **Import**: `import { RequestHeaders, typeis, typeisCsv, is, typeIs, hasBody, requestHasBody, ... } from "type-is";`
- **Origin**: npm `type-is` (v1.0.0, MIT, zero-heap, depends on `mime-types`, `unicase`)
- **Full Types**: `src/index.d.cp`

## Core Type
```cp
class RequestHeaders {
    content_type: string,
    transfer_encoding: string,
    content_length: string,
}
```

## Functions
- **Request-Level (checks `requestHasBody` first)**:
  - `hasBody(transfer_encoding: string, content_length: string): bool`
  - `requestHasBody(req: RequestHeaders): bool`
  - `typeis(req: RequestHeaders, types: &[]string): string?`
  - `typeisCsv(req: RequestHeaders, candidates_csv: string): string?`
- **Header-Level Matching**:
  - `is(content_type: string, types: &[]string): string?`
  - `typeIs(content_type: string, candidates_csv: string): string?`
  - `typeMatches(content_type: string, pattern: string): bool`
  - `matchType(expected: string, actual: string): bool`
- **Parsing & Normalization**:
  - `extractEssence(content_type: string): string?`
  - `isValidContentType(content_type: string): bool`
  - `normalize(pattern: string): string?`
  - `normalizeTypePattern(pattern: string): string`
- **Quick Predicates**:
  - `isJson(content_type: string): bool` | `isText(content_type: string): bool`
  - `isForm(content_type: string): bool` | `isMultipart(content_type: string): bool`

## Minimal Pattern
```cp
let req = RequestHeaders { content_type: "application/json", transfer_encoding: "", content_length: "16" };
let types: [2]string = ["json", "urlencoded"];
if (typeis(req, &types)) |hit| { /* hit == "json" */ }
```

## Gotchas
1. `typeis` / `typeisCsv` return `null` when the request has **no body** (both `transfer_encoding` and `content_length` empty/zero); use `is` / `typeIs` to match a raw `Content-Type` string directly.
2. Note parameter order: `typeMatches(content_type, pattern)` vs `matchType(expected, actual)` (npm `typeis.match(expected, actual)` convention).
3. Matched result is the **winning candidate string** from `types`/`candidates_csv` (e.g., `"json"` or `"+json"`), or `null` if none matched.
