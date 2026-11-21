# `cap:editorconfig` — `.editorconfig` Parser & Property Resolver

Zero-allocation `.editorconfig` parser supporting `root = true`, section glob patterns (`*`, `**`, `?`, `{a,b,c}` brace alternation), cascading overrides, and `unset` clearing.

## Exports

- `matchSectionPattern(pattern, rel_path): bool`
- `IndentStyle` (`unset` | `space` | `tab`), `EndOfLine` (`unset` | `lf` | `crlf` | `cr`), `Charset` (`unset` | `latin1` | `utf8` | `utf8_bom` | `utf16be` | `utf16le`)
- `EditorConfigProps` (`empty()`, `effectiveTabWidth()`)
- `resolveEditorConfig(content, rel_path): EditorConfigProps`
