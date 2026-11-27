// Type definitions for CP

export fn isUpper(c: u8): bool;
export fn isLower(c: u8): bool;
export fn isDigit(c: u8): bool;
export fn isAlnum(c: u8): bool;
export fn toLower(c: u8): u8;
export fn toUpper(c: u8): u8;
export fn nextWord(input: string, pos: usize, out_start: &mut usize, out_end: &mut usize): bool;
export fn convertJoined(input: string, out: []u8, sep: u8, has_sep: bool, upper_all: bool, cap_first_word: bool, cap_rest_words: bool): usize;
export fn toCamelCase(input: string, out: []u8): usize;
export fn toPascalCase(input: string, out: []u8): usize;
export fn toSnakeCase(input: string, out: []u8): usize;
export fn toKebabCase(input: string, out: []u8): usize;
export fn toScreamingSnakeCase(input: string, out: []u8): usize;
