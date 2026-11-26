// Type definitions for CP

export class ParsedArg {
    kind: u8,
    name: string,
    value: string,
}

export fn strEq(a: string, b: string): bool;
export fn parseArgs(argv: &[]string, out: []ParsedArg): usize;
export fn hasFlag(argv: &[]string, long_name: string, short_char: u8): bool;
export fn getOption(argv: &[]string, long_name: string, short_char: u8): string;
export fn countPositionals(argv: &[]string): usize;
