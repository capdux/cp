// Type definitions for CP

import { strEq } from "cap:ascii";
import { BufWriter } from "cap:fmt";
export class OptionSpec {
    long_name: string,
    short_alias: string,
    value_type: string,
    description: string,
    default_val: string,
    required: bool,
}

export fn optnLeftColLen(spec: OptionSpec): usize;
export fn formatOptionRow(spec: OptionSpec, col_width: usize, out: []u8): usize;
export fn generateHelp(program: string, summary: string, specs: &[]OptionSpec, out: []u8): usize;
export fn findOptionSpec(specs: &[]OptionSpec, flag: string): OptionSpec?;
