// Type definitions for CP

export class ParseU64Result {
    value: u64,
    used: usize,
    ok: bool,
}

export class ParseI64Result {
    value: i64,
    used: usize,
    ok: bool,
}

export fn digitVal(ch: u8, radix: u32): i32;
export fn fromRadixU64(bytes: string, radix: u32): ParseU64Result;
export fn fromRadix10U64(bytes: string): ParseU64Result;
export fn fromRadix16U64(bytes: string): ParseU64Result;
export fn fromRadixI64(bytes: string, radix: u32): ParseI64Result;
export fn fromRadix10I64(bytes: string): ParseI64Result;
export fn atoiU64(s: string): ParseU64Result;
export fn atoiI64(s: string): ParseI64Result;
