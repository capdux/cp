# `cap:mustache` — Zero-Allocation Mustache Template Engine

Zero-allocation, pure-CP Mustache template tokenizer, syntax validator, and HTML-escaping variable interpolation renderer.

## Exports

### Types

- `MustacheTokenKind` (`enum`): `text`, `escaped_var`, `raw_var`, `comment`
- `MustacheToken`: `kind: MustacheTokenKind`, `content: []const u8`
- `MustacheVar`: `key: []const u8`, `value: []const u8`
- `MustacheIter`
  - `MustacheIter.init(template: []const u8): MustacheIter`
  - `next(*_): MustacheToken?` — Yields `text`, `escaped_var` (`{{ var }}`), `raw_var` (`{{{ raw }}}` / `{{& raw }}`), and `comment` (`{{! comment }}`) tokens without heap allocation. Sets `has_error = true` if an unclosed or empty tag is encountered.

### Functions

- `validateMustache(template: []const u8): bool` — Verifies that every `{{...}}` and `{{{...}}}` tag is properly closed and non-empty.
- `renderMustache(template: []const u8, vars: []const MustacheVar, out: []u8): ([]const u8)?` — Renders `template` into caller-provided `out`:
  - `{{var}}`: HTML-escapes `&`, `<`, `>`, `"`, `'` (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`)
  - `{{{raw}}}` / `{{& raw}}`: writes raw unescaped value
  - `{{!comment}}`: omitted from output
  - Missing variables render as `""`
  - Returns `null` on syntax errors or insufficient `out` buffer space

## Example

```typescript
import { MustacheVar, renderMustache, validateMustache } from "cap:mustache";

const tpl = "<h1>{{ title }}</h1>{{! hidden }}<div>{{{ html }}}</div>";
const vars: [2]MustacheVar = [
    MustacheVar { key: "title", value: "A & B <C>" },
    MustacheVar { key: "html", value: "<b>ok</b>" },
];
let buf: [128]u8 = undefined;
if (rendered := renderMustache(tpl, vars[0..], buf[0..])) {
    // rendered == "<h1>A &amp; B &lt;C&gt;</h1><div><b>ok</b></div>"
}
```
