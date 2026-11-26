// Type definitions for CP

export class OptionSpec {
    long_name: string,
    short_alias: string,
    value_type: string,
    description: string,
    default_val: string,
    required: bool,
}

export fn optnStrEq(a: string, b: string): bool;
export fn optnWriteStr(out: []u8, pos: usize, s: string): usize;
export fn optnWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn optnWriteSpaces(out: []u8, pos: usize, count: usize): usize;
export fn optnLeftColLen(spec: OptionSpec): usize;
export fn formatOptionRow(spec: OptionSpec, col_width: usize, out: []u8): usize;
export fn generateHelp(program: string, summary: string, specs: &[]OptionSpec, out: []u8): usize;
export fn findOptionSpec(specs: &[]OptionSpec, flag: string): OptionSpec?;
