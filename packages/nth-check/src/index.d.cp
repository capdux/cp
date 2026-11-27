// Type definitions for CP

import { isSpace, isDigit, trim, eqIgnoreCase } from "cap:ascii";
export class NthRule {
    a: i32,
    b: i32,
    valid: bool,
    fn matches(index: usize): bool;
}

export fn parseNth(formula: string): NthRule;
export fn nthCheck(formula: string, index: usize): bool;
