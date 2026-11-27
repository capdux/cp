// Type definitions for CP

import { isSpace, eqIgnoreCase } from "cap:ascii";
export class A11yNodeInput {
    tag: string,
    aria_label: string,
    labelled_by_text: string,
    inner_text: string,
    alt: string,
    title: string,
    placeholder: string,
    aria_hidden: bool,
}

export fn a11yUsesAlt(tag: string): bool;
export fn collapseWhitespace(input: string, out: []u8): usize;
export fn computeAccessibleName(node: A11yNodeInput, out: []u8): usize;
export fn computeAccessibleDescription(aria_desc: string, title: string, used_title_as_name: bool, out: []u8): usize;
