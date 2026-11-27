// @cap/dom-accessibility-name — Zero-heap W3C Accessible Name & Description (clean-room CP)
// Modeled on npm `dom-accessibility-api` (W3C AccName 1.2).

import { isSpace, eqIgnoreCase } from "cap:ascii";

pub const NAME_SOURCE_NONE: u8 = 0;
pub const NAME_SOURCE_ARIA_LABELLEDBY: u8 = 1;
pub const NAME_SOURCE_ARIA_LABEL: u8 = 2;
pub const NAME_SOURCE_ALT: u8 = 3;
pub const NAME_SOURCE_HOST_LABEL: u8 = 4;
pub const NAME_SOURCE_VALUE: u8 = 5;
pub const NAME_SOURCE_CONTENT: u8 = 6;
pub const NAME_SOURCE_PLACEHOLDER: u8 = 7;
pub const NAME_SOURCE_TITLE: u8 = 8;

pub struct A11yNodeInput {
    tag: string,
    aria_label: string,
    labelled_by_text: string,
    inner_text: string,
    alt: string,
    title: string,
    placeholder: string,
    aria_hidden: bool,
}

/// Extended node input covering full W3C AccName 1.2 host-language & ARIA semantics.
pub struct A11yNodeEx {
    pub tag: string,
    pub role: string,
    pub input_type: string,
    pub aria_label: string,
    pub labelled_by_text: string,
    pub described_by_text: string,
    pub aria_description: string,
    pub inner_text: string,
    pub alt: string,
    pub title: string,
    pub placeholder: string,
    pub value: string,
    pub label_text: string,
    pub legend_text: string,
    pub caption_text: string,
    pub figcaption_text: string,
    pub aria_hidden: bool,
    pub is_hidden: bool,
}

/// Options modeled on `dom-accessibility-api` `ComputeTextAlternativeOptions`.
pub struct ComputeNameOptions {
    pub include_hidden: bool,
    pub compute_from_content: bool,
    pub fallback_to_placeholder: bool,
    pub fallback_to_title: bool,
}

/// Detailed result from `computeAccessibleNameWithOptions`.
pub struct NameResult {
    pub len: usize,
    pub source: u8,
    pub used_title: bool,
    pub used_placeholder: bool,
}

pub fn defaultComputeOptions(): ComputeNameOptions {
    return ComputeNameOptions{
        include_hidden: false,
        compute_from_content: true,
        fallback_to_placeholder: true,
        fallback_to_title: true,
    };
}

pub fn defaultA11yNodeEx(tag: string): A11yNodeEx {
    return A11yNodeEx{
        tag: tag,
        role: "",
        input_type: "",
        aria_label: "",
        labelled_by_text: "",
        described_by_text: "",
        aria_description: "",
        inner_text: "",
        alt: "",
        title: "",
        placeholder: "",
        value: "",
        label_text: "",
        legend_text: "",
        caption_text: "",
        figcaption_text: "",
        aria_hidden: false,
        is_hidden: false,
    };
}

fn a11yUsesAlt(tag: string): bool {
    return eqIgnoreCase(tag, "img") || eqIgnoreCase(tag, "area");
}

fn a11yIsInputButtonType(input_type: string): bool {
    return eqIgnoreCase(input_type, "button")
        || eqIgnoreCase(input_type, "submit")
        || eqIgnoreCase(input_type, "reset");
}

fn a11yIsFormControl(tag: string): bool {
    return eqIgnoreCase(tag, "input")
        || eqIgnoreCase(tag, "textarea")
        || eqIgnoreCase(tag, "select")
        || eqIgnoreCase(tag, "meter")
        || eqIgnoreCase(tag, "output")
        || eqIgnoreCase(tag, "progress");
}

/// Returns `true` if the ARIA role is `presentation` or `none`.
pub fn isPresentationRole(role: string): bool {
    return eqIgnoreCase(role, "presentation") || eqIgnoreCase(role, "none");
}

/// Returns `true` if a node is hidden from the accessibility tree under the given options.
pub fn isHiddenElement(aria_hidden: bool, is_hidden: bool, include_hidden: bool): bool {
    if (include_hidden) { return false; }
    return aria_hidden || is_hidden;
}

/// Returns `true` if W3C AccName 1.2 permits naming from descendant text content for `(tag, role)`.
pub fn allowsNameFromContent(tag: string, role: string): bool {
    if (isPresentationRole(role)) { return false; }
    if (role.len > 0) {
        return eqIgnoreCase(role, "button")
            || eqIgnoreCase(role, "link")
            || eqIgnoreCase(role, "heading")
            || eqIgnoreCase(role, "checkbox")
            || eqIgnoreCase(role, "radio")
            || eqIgnoreCase(role, "switch")
            || eqIgnoreCase(role, "tab")
            || eqIgnoreCase(role, "menuitem")
            || eqIgnoreCase(role, "menuitemcheckbox")
            || eqIgnoreCase(role, "menuitemradio")
            || eqIgnoreCase(role, "option")
            || eqIgnoreCase(role, "treeitem")
            || eqIgnoreCase(role, "cell")
            || eqIgnoreCase(role, "gridcell")
            || eqIgnoreCase(role, "columnheader")
            || eqIgnoreCase(role, "rowheader")
            || eqIgnoreCase(role, "row")
            || eqIgnoreCase(role, "tooltip");
    }
    return eqIgnoreCase(tag, "button")
        || eqIgnoreCase(tag, "a")
        || eqIgnoreCase(tag, "h1")
        || eqIgnoreCase(tag, "h2")
        || eqIgnoreCase(tag, "h3")
        || eqIgnoreCase(tag, "h4")
        || eqIgnoreCase(tag, "h5")
        || eqIgnoreCase(tag, "h6")
        || eqIgnoreCase(tag, "li")
        || eqIgnoreCase(tag, "td")
        || eqIgnoreCase(tag, "th")
        || eqIgnoreCase(tag, "label")
        || eqIgnoreCase(tag, "option")
        || eqIgnoreCase(tag, "summary")
        || eqIgnoreCase(tag, "legend")
        || eqIgnoreCase(tag, "caption")
        || eqIgnoreCase(tag, "figcaption")
        || eqIgnoreCase(tag, "span")
        || eqIgnoreCase(tag, "div")
        || eqIgnoreCase(tag, "p");
}

pub fn collapseWhitespace(input: string, out: []u8): usize {
    let pos: usize = 0;
    let in_space = false;
    let started = false;
    let i: usize = 0;
    while (i < input.len) : (i += 1) {
        const c = input[i];
        if (isSpace(c)) {
            if (started) {
                in_space = true;
            }
        } else {
            if (in_space && pos < out.len) {
                out[pos] = ' ';
                pos += 1;
            }
            in_space = false;
            started = true;
            if (pos < out.len) {
                out[pos] = c;
                pos += 1;
            }
        }
    }
    return pos;
}

pub fn computeAccessibleName(node: A11yNodeInput, out: []u8): usize {
    if (node.aria_hidden) {
        return 0;
    }
    let n = collapseWhitespace(node.labelled_by_text, out);
    if (n > 0) return n;

    n = collapseWhitespace(node.aria_label, out);
    if (n > 0) return n;

    if (a11yUsesAlt(node.tag)) {
        n = collapseWhitespace(node.alt, out);
        if (n > 0) return n;
    }

    n = collapseWhitespace(node.inner_text, out);
    if (n > 0) return n;

    n = collapseWhitespace(node.placeholder, out);
    if (n > 0) return n;

    n = collapseWhitespace(node.title, out);
    return n;
}

/// Full W3C AccName 1.2 accessible name computation with options and source attribution.
pub fn computeAccessibleNameWithOptions(
    node: A11yNodeEx,
    opts: ComputeNameOptions,
    out: []u8,
): NameResult {
    const empty = NameResult{ len: 0, source: NAME_SOURCE_NONE, used_title: false, used_placeholder: false };
    if (isHiddenElement(node.aria_hidden, node.is_hidden, opts.include_hidden)) {
        return empty;
    }

    // Step 2B: aria-labelledby
    let n = collapseWhitespace(node.labelled_by_text, out);
    if (n > 0) {
        return NameResult{ len: n, source: NAME_SOURCE_ARIA_LABELLEDBY, used_title: false, used_placeholder: false };
    }

    // Step 2C: aria-label
    n = collapseWhitespace(node.aria_label, out);
    if (n > 0) {
        return NameResult{ len: n, source: NAME_SOURCE_ARIA_LABEL, used_title: false, used_placeholder: false };
    }

    // Step 2D: Host-language native labeling
    if (!isPresentationRole(node.role)) {
        // img, area, input[type="image"] -> alt
        if (a11yUsesAlt(node.tag) || (eqIgnoreCase(node.tag, "input") && eqIgnoreCase(node.input_type, "image"))) {
            n = collapseWhitespace(node.alt, out);
            if (n > 0) {
                return NameResult{ len: n, source: NAME_SOURCE_ALT, used_title: false, used_placeholder: false };
            }
        }

        // Form controls -> associated <label>
        if (a11yIsFormControl(node.tag)) {
            n = collapseWhitespace(node.label_text, out);
            if (n > 0) {
                return NameResult{ len: n, source: NAME_SOURCE_HOST_LABEL, used_title: false, used_placeholder: false };
            }
        }

        // input[type=button|submit|reset] -> value or default label
        if (eqIgnoreCase(node.tag, "input") && a11yIsInputButtonType(node.input_type)) {
            n = collapseWhitespace(node.value, out);
            if (n > 0) {
                return NameResult{ len: n, source: NAME_SOURCE_VALUE, used_title: false, used_placeholder: false };
            }
            if (eqIgnoreCase(node.input_type, "submit")) {
                n = collapseWhitespace("Submit", out);
                return NameResult{ len: n, source: NAME_SOURCE_VALUE, used_title: false, used_placeholder: false };
            }
            if (eqIgnoreCase(node.input_type, "reset")) {
                n = collapseWhitespace("Reset", out);
                return NameResult{ len: n, source: NAME_SOURCE_VALUE, used_title: false, used_placeholder: false };
            }
        }

        // fieldset -> legend, table -> caption, figure -> figcaption
        if (eqIgnoreCase(node.tag, "fieldset")) {
            n = collapseWhitespace(node.legend_text, out);
            if (n > 0) {
                return NameResult{ len: n, source: NAME_SOURCE_HOST_LABEL, used_title: false, used_placeholder: false };
            }
        }
        if (eqIgnoreCase(node.tag, "table")) {
            n = collapseWhitespace(node.caption_text, out);
            if (n > 0) {
                return NameResult{ len: n, source: NAME_SOURCE_HOST_LABEL, used_title: false, used_placeholder: false };
            }
        }
        if (eqIgnoreCase(node.tag, "figure")) {
            n = collapseWhitespace(node.figcaption_text, out);
            if (n > 0) {
                return NameResult{ len: n, source: NAME_SOURCE_HOST_LABEL, used_title: false, used_placeholder: false };
            }
        }
    }

    // Step 2F: Name from content
    if (opts.compute_from_content && allowsNameFromContent(node.tag, node.role)) {
        n = collapseWhitespace(node.inner_text, out);
        if (n > 0) {
            return NameResult{ len: n, source: NAME_SOURCE_CONTENT, used_title: false, used_placeholder: false };
        }
    }

    // Step 2D/2I: placeholder fallback for text inputs
    if (opts.fallback_to_placeholder && (eqIgnoreCase(node.tag, "input") || eqIgnoreCase(node.tag, "textarea"))) {
        n = collapseWhitespace(node.placeholder, out);
        if (n > 0) {
            return NameResult{ len: n, source: NAME_SOURCE_PLACEHOLDER, used_title: false, used_placeholder: true };
        }
    }

    // Step 2I: Tooltip (title) fallback
    if (opts.fallback_to_title) {
        n = collapseWhitespace(node.title, out);
        if (n > 0) {
            return NameResult{ len: n, source: NAME_SOURCE_TITLE, used_title: true, used_placeholder: false };
        }
    }

    return empty;
}

pub fn computeAccessibleDescription(
    aria_desc: string,
    title: string,
    used_title_as_name: bool,
    out: []u8,
): usize {
    let n = collapseWhitespace(aria_desc, out);
    if (n > 0) return n;

    if (!used_title_as_name) {
        return collapseWhitespace(title, out);
    }
    return 0;
}

/// Full W3C AccName 1.2 accessible description computation for `A11yNodeEx`.
/// Priority: `aria-describedby` -> `aria-description` -> `title` (unless used as name) -> `placeholder` (unless used as name).
pub fn computeAccessibleDescriptionForNode(
    node: A11yNodeEx,
    opts: ComputeNameOptions,
    used_title_as_name: bool,
    used_placeholder_as_name: bool,
    out: []u8,
): usize {
    if (isHiddenElement(node.aria_hidden, node.is_hidden, opts.include_hidden)) {
        return 0;
    }

    let n = collapseWhitespace(node.described_by_text, out);
    if (n > 0) return n;

    n = collapseWhitespace(node.aria_description, out);
    if (n > 0) return n;

    if (!used_title_as_name) {
        n = collapseWhitespace(node.title, out);
        if (n > 0) return n;
    }

    if (!used_placeholder_as_name && (eqIgnoreCase(node.tag, "input") || eqIgnoreCase(node.tag, "textarea"))) {
        n = collapseWhitespace(node.placeholder, out);
        if (n > 0) return n;
    }

    return 0;
}
