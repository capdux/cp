// Type definitions for CP

import { isDigit, toLowerByte } from "cap:ascii";
export fn compareImpl(a: string, b: string, ignore_case: bool): i32;
export fn compareNatural(a: string, b: string): i32;
export fn compareNaturalIgnoreCase(a: string, b: string): i32;
export fn isSortedNatural(items: &[]string): bool;
export fn sortNatural(items: []string);
