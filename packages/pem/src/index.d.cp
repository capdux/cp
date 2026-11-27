// Type definitions for CP

import { findByte } from "cap:ascii";
import { indexOfBytes } from "cap:bin";
import { writeStr } from "cap:fmt";
export class PemBlock {
    valid: bool,
    label: string,
    raw_body: string,
    start_offset: usize,
    end_offset: usize,
}

export class PemBundle {
    blocks: [16]PemBlock,
    count: usize,
}

export fn isValidLabel(label: string): bool;
export fn parsePemFrom(input: string, from_offset: usize): PemBlock;
export fn parsePem(input: string): PemBlock;
export fn parsePemBundle(input: string): PemBundle;
export fn extractBase64(blk: PemBlock, out: []u8): string;
export fn encodePem(label: string, base64_payload: string, out: []u8): string;
