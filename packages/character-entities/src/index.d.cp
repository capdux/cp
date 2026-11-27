// Type definitions for CP

export fn cenStrEq(a: string, b: string): bool;
export fn cenWriteUtf8(cp: u32, out: []u8, pos: usize): usize;
export fn mapInvalidCharRef(cp: u32): u32;
export fn decodeNamedEntity(name: string): u32?;
export fn decodeNumericEntity(body: string): u32?;
export fn decodeEntities(input: string, out: []u8): usize;
export fn cenWriteStr(s: string, out: []u8, pos: usize): usize;
export fn encodeEntities(input: string, out: []u8): usize;
