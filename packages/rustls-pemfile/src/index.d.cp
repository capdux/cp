// Type definitions for CP

import { PEM_OK, PEM_ERR_MISSING_BEGIN, locatePem, decodePem, decodeLabel, isValidLabel } from "../../pem-rfc7468/src/index.cp";
export fn rpfStrEq(a: string, b: string): bool;
export fn isPrivateKeyKind(kind: u8): bool;
export fn classifyPemLabel(label: string): u8;
export class PemItem {
    kind: u8,
    der_len: usize,
    ok: bool,
    eof: bool,
    fn makeEof(): PemItem;
    fn makeErr(): PemItem;
    fn isCert(): bool;
    fn isPrivateKey(): bool;
}

export class PemBundleReader {
    input: string,
    offset: usize,
    fn new(input: string): PemBundleReader;
    fn remaining(): usize;
    fn isEof(): bool;
    fn reset(_: auto);
    fn peekNextKind(): u8;
    fn nextItem(_: auto, der_out: []u8): PemItem;
    fn countItems(): usize;
    fn countCerts(): usize;
}

export fn readFirstPrivateKey(input: string, der_out: []u8): PemItem;
