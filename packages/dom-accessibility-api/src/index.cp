// @cap/dom-accessibility-name — Zero-heap W3C Accessible Name & Description (clean-room CP)

import { isSpace, eqIgnoreCase } from "cap:ascii";

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

fn a11yUsesAlt(tag: string): bool {
    return eqIgnoreCase(tag, "img") || eqIgnoreCase(tag, "area");
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
