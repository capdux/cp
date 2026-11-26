# `@cap/mediatype`

Zero-heap structured MIME `MediaType` parser, parameter store, `+suffix` inspector, and RFC 6838 formatter — clean-room CP equivalent of crates.io `mediatype`.

Reuses `@cap/media-typer` and `@cap/unicase` as independent packages.

## Features
- `ParsedMediaType.parse(raw)` — parses `type/subtype+suffix; param=val; ...`
- `getParam(key)` (case-insensitive via `@cap/unicase`), `hasSuffix(suf)`, `isJson()`, `isText()`
- `formatEssence(out)` & `formatFull(out)`
- `essenceEq(a, b)` — compares media type essences ignoring parameters and case.
