// Type definitions for CP

export class DerTlv {
    valid: bool,
    tag: u8,
    value: string,
    total_len: usize,
    fn invalid(): DerTlv;
}

export fn parseDerTlv(input: string): DerTlv;
export fn decodeDerBool(tlv: DerTlv): bool?;
export fn decodeDerU64(tlv: DerTlv): u64?;
export fn derLengthSize(val_len: usize): usize;
export fn writeDerLength(out: []u8, pos: usize, val_len: usize): usize;
export fn encodeDerTlv(tag: u8, value: string, out: []u8): string?;
export fn encodeDerBool(v: bool, out: []u8): string?;
export fn encodeDerNull(out: []u8): string?;
export fn encodeDerU64(v: u64, out: []u8): string?;
export fn encodeDerOctetString(data: string, out: []u8): string?;
export fn encodeDerSequence(inner_bytes: string, out: []u8): string?;
