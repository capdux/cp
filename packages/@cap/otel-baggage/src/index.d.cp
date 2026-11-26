// Type definitions for CP

export class BaggageMember {
    key: string,
    value: string,
    properties: string,
}

export fn isSpace(b: u8): bool;
export fn isTchar(b: u8): bool;
export fn isValidKey(key: string): bool;
export fn trimSlice(s: string): string;
export fn strEq(a: string, b: string): bool;
export fn parseSingleMember(seg: string, out: &mut BaggageMember): bool;
export fn parseBaggage(header: string, out: []BaggageMember): usize;
export fn countMembers(header: string): usize;
export fn hexVal(b: u8): i32;
export fn percentDecode(src: string, out: []u8, out_len: &mut usize): bool;
export fn getDecodedValue(header: string, target_key: string, out: []u8, out_len: &mut usize): bool;
export fn hexUpper(nibble: u8): u8;
export fn needsPercentEncode(b: u8): bool;
export fn formatBaggage(members: &[]BaggageMember, out: []u8): usize;
