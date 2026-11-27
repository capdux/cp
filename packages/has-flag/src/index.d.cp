// Type definitions for CP

export fn hfEq(a: string, b: string): bool;
export fn hfStartsWith(s: string, prefix: string): bool;
export fn hfMatchesFlag(arg: string, flag: string, prefix_len: usize): bool;
export fn hfComputePrefixLen(flag: string): usize;
export fn findFlagIndex(flag: string, argv: &[]string): i64;
export fn hasFlag(flag: string, argv: &[]string): bool;
export fn getFlagValue(flag: string, argv: &[]string, out: []u8): i64;
export fn isFlagEnabled(flag: string, argv: &[]string, default_val: bool): bool;
