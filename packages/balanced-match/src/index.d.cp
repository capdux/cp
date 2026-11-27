// Type definitions for CP

import { strEq } from "cap:ascii";
import { indexOfBytes } from "cap:bin";
export class BalancedResult {
    found: bool,
    start: usize,
    end: usize,
    pre: string,
    body: string,
    post: string,
}

export fn bmMakeRange(s: usize, e: usize): [usize, usize];
export fn bmEmptyResult(): BalancedResult;
export fn balancedRange(a: string, b: string, input: string): [usize, usize]?;
export fn balanced(a: string, b: string, input: string): BalancedResult;
export fn balancedAll(a: string, b: string, input: string, out: []BalancedResult): usize;
