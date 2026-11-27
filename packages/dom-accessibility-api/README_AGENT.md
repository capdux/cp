# dom-accessibility-api — AI Agent Quick Ref

Zero-heap W3C AccName 1.2 accessible name & description computation (npm `dom-accessibility-api` v0.6.3 parity).

## Import

```cp
import {
    A11yNodeInput, A11yNodeEx, ComputeNameOptions, NameResult,
    defaultComputeOptions, defaultA11yNodeEx,
    computeAccessibleName, computeAccessibleNameWithOptions,
    computeAccessibleDescription, computeAccessibleDescriptionForNode,
    collapseWhitespace, allowsNameFromContent, isHiddenElement, isPresentationRole,
} from "dom-accessibility-api";
```

## Core API Signatures

```cp
// Source codes (NameResult.source):
// NAME_SOURCE_NONE=0, ARIA_LABELLEDBY=1, ARIA_LABEL=2, ALT=3,
// HOST_LABEL=4, VALUE=5, CONTENT=6, PLACEHOLDER=7, TITLE=8

class A11yNodeInput { tag, aria_label, labelled_by_text, inner_text, alt, title, placeholder: string, aria_hidden: bool }
class A11yNodeEx {
    tag, role, input_type, aria_label, labelled_by_text, described_by_text,
    aria_description, inner_text, alt, title, placeholder, value,
    label_text, legend_text, caption_text, figcaption_text: string,
    aria_hidden: bool, is_hidden: bool,
}
class ComputeNameOptions { include_hidden, compute_from_content, fallback_to_placeholder, fallback_to_title: bool }
class NameResult { len: usize, source: u8, used_title: bool, used_placeholder: bool }

defaultComputeOptions(): ComputeNameOptions
defaultA11yNodeEx(tag: string): A11yNodeEx
computeAccessibleName(node: A11yNodeInput, out: []u8): usize
computeAccessibleNameWithOptions(node: A11yNodeEx, opts: ComputeNameOptions, out: []u8): NameResult
computeAccessibleDescription(aria_desc: string, title: string, used_title_as_name: bool, out: []u8): usize
computeAccessibleDescriptionForNode(node: A11yNodeEx, opts: ComputeNameOptions, used_title_as_name: bool, used_placeholder_as_name: bool, out: []u8): usize
collapseWhitespace(input: string, out: []u8): usize
allowsNameFromContent(tag: string, role: string): bool
isPresentationRole(role: string): bool
isHiddenElement(aria_hidden: bool, is_hidden: bool, include_hidden: bool): bool
a11yUsesAlt(tag: string): bool; a11yIsInputButtonType(input_type: string): bool; a11yIsFormControl(tag: string): bool
```

## Minimal Example

```cp
import { defaultA11yNodeEx, defaultComputeOptions, computeAccessibleNameWithOptions } from "dom-accessibility-api";

fn demo(): usize {
    let mut node = defaultA11yNodeEx("button");
    node.inner_text = "  Submit  Form ";
    let out: [64]u8 = undefined;
    let res = computeAccessibleNameWithOptions(node, defaultComputeOptions(), out[0..64]);
    return res.len; // 11 ("Submit Form")
}
```

## Gotchas & Borrow Rules

- **Writable output buffer**: `out: []u8` receives the normalized text; slice `out[0..res.len]` to read the result.
- **Deduplicate description**: Always pass `res.used_title` and `res.used_placeholder` from `computeAccessibleNameWithOptions` into `computeAccessibleDescriptionForNode`.
- **Hidden nodes**: When `aria_hidden` or `is_hidden` is `true` and `opts.include_hidden` is `false`, returns empty (`len == 0`).
- **Builder helper**: Prefer `defaultA11yNodeEx("tag")` and override only needed fields instead of manually filling all 18 fields.
