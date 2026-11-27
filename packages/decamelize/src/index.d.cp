// Type definitions for CP

export struct DecamelizeOptions {
    separator: string,
    preserve_consecutive_uppercase: bool,
    fn defaults(): DecamelizeOptions;
}

export fn decamelizedLen(input: string, separator: string, preserve_consecutive_uppercase: bool): usize;
export fn decamelizeWithOptions(input: string, separator: string, preserve_consecutive_uppercase: bool, out: []u8): usize;
export fn decamelizeOpts(input: string, opts: DecamelizeOptions, out: []u8): usize;
export fn decamelize(input: string, separator: string, out: []u8): usize;
export fn decamelizeDefault(input: string, out: []u8): usize;
export fn toSnakeCase(input: string, out: []u8): usize;
export fn toKebabCase(input: string, out: []u8): usize;
export fn toSpaceCase(input: string, out: []u8): usize;
export fn toConstantCase(input: string, out: []u8): usize;
