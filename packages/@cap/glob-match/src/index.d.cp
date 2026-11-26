// Type definitions for CP

export fn gmIsSpace(c: u8): bool;
export fn gmTrim(s: &[]u8): &[]u8;
export fn gmContainsByte(s: &[]u8, target: u8): bool;
export fn gmMatchCharClass(pattern: &[]u8, pi: usize, ch: u8, next_pi: &mut usize): i32;
export fn isMatch(pattern: string, path: string): bool;
export fn isMatchAny(patterns: &[]string, path: string): bool;
export fn gmBasename(path: &[]u8): &[]u8;
export fn gmRuleMatchesPath(rule: &[]u8, path: &[]u8): bool;
export fn isIgnored(rules: &[]string, path: string): bool;
export fn hasGlobMagic(pattern: string): bool;
