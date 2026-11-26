// Type definitions for CP

export class BomInfo {
    kind: u8,
    byte_len: usize,
}

export fn detectBom(data: []u8): BomInfo;
export fn hasUtf8Bom(s: string): bool;
export fn stripUtf8Bom(s: string): string;
export fn stripAnyBomOffset(data: []u8): usize;
export fn prependUtf8Bom(s: string, out: []u8): usize;
