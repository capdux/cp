# type-is — AI Agent Quick Ref

Zero-heap HTTP `Content-Type` matcher supporting shorthands, wildcards (`text/*`), structured suffixes (`+json`), and body checks (npm `type-is` parity).

## Import

```cp
import {
    RequestHeaders,
    is, typeIs, typeis, typeisCsv,
    hasBody, requestHasBody,
    normalize, normalizeTypePattern, extractEssence,
    isValidContentType, typeMatches, matchType,
    isJson, isText, isForm, isMultipart,
} from "type-is";
```

## Core API Signatures

```cp
class RequestHeaders { content_type: string, transfer_encoding: string, content_length: string }

normalize(pattern: string): string?                // strict: null on empty/unknown
normalizeTypePattern(pattern: string): string      // lenient fallback
extractEssence(content_type: string): string?      // strips "; charset=..."
isValidContentType(content_type: string): bool
typeMatches(content_type: string, pattern: string): bool
matchType(expected: string, actual: string): bool

is(content_type: string, types: &[]string): string?
typeIs(content_type: string, candidates_csv: string): string?
hasBody(transfer_encoding: string, content_length: string): bool
requestHasBody(req: RequestHeaders): bool
typeis(req: RequestHeaders, types: &[]string): string?
typeisCsv(req: RequestHeaders, candidates_csv: string): string?

isJson(content_type: string): bool
isText(content_type: string): bool
isForm(content_type: string): bool
isMultipart(content_type: string): bool
```

## Minimal Example

```cp
import { RequestHeaders, is, typeis } from "type-is";

fn demo(): bool {
    let patterns = ["json", "+json", "text/*"];
    let a = is("application/vnd.api+json; charset=utf-8", &patterns); // "+json"
    let req = RequestHeaders {
        content_type: "application/json",
        transfer_encoding: "",
        content_length: "42",
    };
    let b = typeis(req, &patterns); // "json"
    return a != null && b != null;
}
```

## Gotchas & Borrow Rules

- **Body guard in `typeis`/`typeisCsv`**: Returns `null` when both `transfer_encoding` and `content_length` are empty, even if `content_type` matches. Use `is` / `typeIs` to match header strings directly.
- **Read-only slice borrow**: Pass `&patterns` (`&[]string`) to `is` and `typeis`.
- **Return semantics**: Returns `string?` (`null` on mismatch/no body), never `false`.
- **Shorthands**: `"urlencoded"` -> `"application/x-www-form-urlencoded"`, `"multipart"` -> `"multipart/*"`, `"*"` -> `"*/*"`, extensions delegate to `mime-types.lookup`.
