// Type definitions for CP

import { decamelize } from "../../decamelize/src/index.cp";

export class HumanizeOptions {
    capitalize_first: bool,
    preserve_acronyms: bool,
    static fn defaults(): HumanizeOptions;
}

export fn humanizeString(input: string, out: []u8): usize;
export fn humanizeStringWithOptions(input: string, opts: HumanizeOptions, out: []u8): usize;
export fn titleizeString(input: string, out: []u8): usize;
export fn humanizeLength(input: string): usize;
