[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# dom-accessibility-api

Zero-heap W3C Accessible Name and Description Computation (AccName 1.2) library for CP, modeled on npm [`dom-accessibility-api`](https://github.com/eps1lon/dom-accessibility-api) v0.6.3 (MIT, CP v0.6.3).

Computes accessible names and descriptions following W3C AccName precedence (`aria-labelledby` → `aria-label` → host-language labels / `alt` / `value` / `legend` / `caption` / `figcaption` → content → `placeholder` → `title`) with whitespace collapsing, presentation-role handling, and hidden-element rules—completely without heap allocations.

## Installation & Import

```bash
cap add dom-accessibility-api
```

```cp
import {
    A11yNodeInput,
    A11yNodeEx,
    ComputeNameOptions,
    NameResult,
    defaultComputeOptions,
    defaultA11yNodeEx,
    computeAccessibleName,
    computeAccessibleNameWithOptions,
    computeAccessibleDescription,
    computeAccessibleDescriptionForNode,
} from "dom-accessibility-api";
```

## Core API Reference

### Constants (`NameResult.source`)

| Constant | Value | Meaning |
| :--- | :--- | :--- |
| `NAME_SOURCE_NONE` | `0` | No accessible name resolved. |
| `NAME_SOURCE_ARIA_LABELLEDBY` | `1` | Resolved from `labelled_by_text`. |
| `NAME_SOURCE_ARIA_LABEL` | `2` | Resolved from `aria_label`. |
| `NAME_SOURCE_ALT` | `3` | Resolved from `alt` (`img`, `area`, `input`). |
| `NAME_SOURCE_HOST_LABEL` | `4` | Resolved from `<label>`, `<legend>`, `<caption>`, or `<figcaption>`. |
| `NAME_SOURCE_VALUE` | `5` | Resolved from input button `value`. |
| `NAME_SOURCE_CONTENT` | `6` | Resolved from `inner_text` (when `allowsNameFromContent` is true). |
| `NAME_SOURCE_PLACEHOLDER` | `7` | Resolved from `placeholder` fallback. |
| `NAME_SOURCE_TITLE` | `8` | Resolved from `title` fallback. |

### Data Structures

| Type | Key Fields | Description |
| :--- | :--- | :--- |
| `A11yNodeInput` | `tag`, `aria_label`, `labelled_by_text`, `inner_text`, `alt`, `title`, `placeholder`, `aria_hidden` | Compact node input for quick `computeAccessibleName`. |
| `A11yNodeEx` | All `A11yNodeInput` fields + `role`, `input_type`, `described_by_text`, `aria_description`, `value`, `label_text`, `legend_text`, `caption_text`, `figcaption_text`, `is_hidden` | Full W3C AccName 1.2 element descriptor. |
| `ComputeNameOptions` | `include_hidden`, `compute_from_content`, `fallback_to_placeholder`, `fallback_to_title` | Fine-grained AccName computation options. |
| `NameResult` | `len: usize`, `source: u8`, `used_title: bool`, `used_placeholder: bool` | Written byte length plus provenance flags for description deduplication. |

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `defaultComputeOptions` | `(): ComputeNameOptions` | Returns standard AccName options (`include_hidden: false`, content & fallbacks enabled). |
| `defaultA11yNodeEx` | `(tag: string): A11yNodeEx` | Creates a zeroed `A11yNodeEx` pre-populated with `tag`. |
| `computeAccessibleName` | `(node: A11yNodeInput, out: []u8): usize` | Computes normalized accessible name into `out`, returning bytes written. |
| `computeAccessibleNameWithOptions` | `(node: A11yNodeEx, opts: ComputeNameOptions, out: []u8): NameResult` | Full AccName 1.2 computation returning length, source code, and fallback flags. |
| `computeAccessibleDescription` | `(aria_desc: string, title: string, used_title_as_name: bool, out: []u8): usize` | Computes accessible description avoiding duplicate `title` reuse. |
| `computeAccessibleDescriptionForNode` | `(node: A11yNodeEx, opts: ComputeNameOptions, used_title_as_name: bool, used_placeholder_as_name: bool, out: []u8): usize` | Full AccName description computation (`described_by_text` → `aria_description` → `title` / `placeholder`). |
| `collapseWhitespace` | `(input: string, out: []u8): usize` | Trims leading/trailing ASCII whitespace and collapses internal runs to single spaces. |
| `isPresentationRole` / `isHiddenElement` / `allowsNameFromContent` | `(...): bool` | Spec predicate helpers for ARIA roles, visibility, and name-from-content eligibility. |
| `a11yUsesAlt` / `a11yIsInputButtonType` / `a11yIsFormControl` | `(string): bool` | HTML host-language element classification helpers. |

## Usage Examples

### 1. Full AccName Name + Description Computation

```cp
import {
    defaultA11yNodeEx,
    defaultComputeOptions,
    computeAccessibleNameWithOptions,
    computeAccessibleDescriptionForNode,
} from "dom-accessibility-api";

fn inspectInput() {
    let mut node = defaultA11yNodeEx("input");
    node.input_type = "text";
    node.label_text = "  Email   Address ";
    node.title = "We will never share your email";

    let opts = defaultComputeOptions();
    let name_buf: [128]u8 = undefined;
    let res = computeAccessibleNameWithOptions(node, opts, name_buf[0..128]);
    // name_buf[0..res.len] == "Email Address", res.used_title == false

    let desc_buf: [128]u8 = undefined;
    let dlen = computeAccessibleDescriptionForNode(
        node, opts, res.used_title, res.used_placeholder, desc_buf[0..128],
    );
    // desc_buf[0..dlen] == "We will never share your email"
}
```

### 2. Quick Name Lookup (`A11yNodeInput`)

```cp
import { A11yNodeInput, computeAccessibleName } from "dom-accessibility-api";

fn quickButtonName(): usize {
    let btn = A11yNodeInput {
        tag: "button",
        aria_label: "",
        labelled_by_text: "",
        inner_text: "  Save \n Changes  ",
        alt: "",
        title: "Save",
        placeholder: "",
        aria_hidden: false,
    };
    let out: [64]u8 = undefined;
    return computeAccessibleName(btn, out[0..64]); // writes "Save Changes" (12)
}
```

## Differences from Upstream & Zero-Copy Highlights

- **DOM-Agnostic Value Structs**: Instead of walking live browser DOM nodes, callers populate stack-allocated `A11yNodeInput` or `A11yNodeEx` descriptors—making the library usable in headless SSR, parsers, and test harnesses.
- **Zero Heap Allocation**: All whitespace normalization and name/description formatting write directly into caller-provided `[]u8` buffers.
- **Explicit Provenance Tracking**: `NameResult` reports `source`, `used_title`, and `used_placeholder` so description computation never duplicates fallback attributes.
