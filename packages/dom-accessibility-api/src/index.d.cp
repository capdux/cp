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

export class A11yNodeEx {
    tag: string,
    role: string,
    input_type: string,
    aria_label: string,
    labelled_by_text: string,
    described_by_text: string,
    aria_description: string,
    inner_text: string,
    alt: string,
    title: string,
    placeholder: string,
    value: string,
    label_text: string,
    legend_text: string,
    caption_text: string,
    figcaption_text: string,
    aria_hidden: bool,
    is_hidden: bool,
}

export class ComputeNameOptions {
    include_hidden: bool,
    compute_from_content: bool,
    fallback_to_placeholder: bool,
    fallback_to_title: bool,
}

export class NameResult {
    len: usize,
    source: u8,
    used_title: bool,
    used_placeholder: bool,
}

export fn defaultComputeOptions(): ComputeNameOptions;
export fn defaultA11yNodeEx(tag: string): A11yNodeEx;
export fn a11yUsesAlt(tag: string): bool;
export fn a11yIsInputButtonType(input_type: string): bool;
export fn a11yIsFormControl(tag: string): bool;
export fn isPresentationRole(role: string): bool;
export fn isHiddenElement(aria_hidden: bool, is_hidden: bool, include_hidden: bool): bool;
export fn allowsNameFromContent(tag: string, role: string): bool;
export fn collapseWhitespace(input: string, out: []u8): usize;
export fn computeAccessibleName(node: A11yNodeInput, out: []u8): usize;
export fn computeAccessibleNameWithOptions(node: A11yNodeEx, opts: ComputeNameOptions, out: []u8): NameResult;
export fn computeAccessibleDescription(aria_desc: string, title: string, used_title_as_name: bool, out: []u8): usize;
export fn computeAccessibleDescriptionForNode(node: A11yNodeEx, opts: ComputeNameOptions, used_title_as_name: bool, used_placeholder_as_name: bool, out: []u8): usize;
