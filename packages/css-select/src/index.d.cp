// Type definitions for CP

import { isSpace, isAlpha, isDigit, strEq, trim } from "cap:ascii";
import { eqAscii } from "../../unicase/src/index.cp";
export fn csssIsCombinator(c: u8): bool;
export fn csssIsIdentStart(c: u8): bool;
export fn csssIsIdentChar(c: u8): bool;
export fn csssEqMaybeCi(a: string, b: string, ci: bool): bool;
export fn csssSkipIdent(s: string, pos: usize): usize;
export fn csssFindBracketEnd(s: string, pos: usize): usize;
export fn csssSkipParen(s: string, pos: usize): usize;
export fn csssHasToken(list: string, target: string, ci: bool): bool;
export fn csssContains(haystack: string, needle: string, ci: bool): bool;
export fn csssIsLegacyPseudoElement(name: string): bool;
export class Specificity {
    ids: u32,
    classes: u32,
    tags: u32,
    fn new(ids: u32, classes: u32, tags: u32): Specificity;
    fn cmp(other: Specificity): i32;
    fn toScore(): u64;
}

export fn csssComputeSingleSpecificity(s: string, depth: u32): Specificity;
export fn csssComputeListSpecificity(selector: string, depth: u32): Specificity;
export fn computeSpecificity(selector: string): Specificity;
export class CssElement {
    tag: string,
    id: string,
    class_list: string,
    attr_name: string,
    attr_val: string,
    fn new(tag: string, id: string, class_list: string, attr_name: string, attr_val: string): CssElement;
}

export fn csssMatchAttr(raw_inner: string, el: CssElement): bool;
export fn matchesCompoundSelector(selector: string, el: CssElement): bool;
export fn matchesSelectorList(selector_list: string, el: CssElement): bool;
export fn countSelectorParts(selector: string): usize;
