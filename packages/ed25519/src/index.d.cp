// Type definitions for CP

export fn ed25Zero32(): [32]u8;
export fn ed25Zero64(): [64]u8;
export fn ed25GroupOrderL(): [32]u8;
export fn ed25SpkiPrefix(): [12]u8;
export fn ed25Pkcs8Prefix(): [16]u8;
export fn ed25HexNibble(ch: u8): i32;
export fn ed25HexDigit(n: u8): u8;
export fn ed25EncodeHex(src: &[]u8, out: []u8): usize;
export class Ed25519Signature {
    bytes: [64]u8,
    fn fromBytes(b: [64]u8): Ed25519Signature;
    fn fromSlice(s: &[]u8): Ed25519Signature?;
    fn fromComponents(r: [32]u8, s: [32]u8): Ed25519Signature;
    fn rBytes(): [32]u8;
    fn sBytes(): [32]u8;
    fn toBytes(): [64]u8;
    fn isCanonicalS(): bool;
    fn fromHex(hex: string): Ed25519Signature?;
    fn toHex(out: []u8): usize;
    fn eql(other: Ed25519Signature): bool;
}

export class Ed25519PublicKey {
    bytes: [32]u8,
    fn fromBytes(b: [32]u8): Ed25519PublicKey;
    fn fromSlice(s: &[]u8): Ed25519PublicKey?;
    fn toBytes(): [32]u8;
    fn toSpkiDer(out: []u8): usize;
    fn fromSpkiDer(der: &[]u8): Ed25519PublicKey?;
    fn fromHex(hex: string): Ed25519PublicKey?;
    fn toHex(out: []u8): usize;
    fn eql(other: Ed25519PublicKey): bool;
}

export class Ed25519SecretKey {
    bytes: [32]u8,
    fn fromBytes(b: [32]u8): Ed25519SecretKey;
    fn fromSlice(s: &[]u8): Ed25519SecretKey?;
    fn toBytes(): [32]u8;
    fn toPkcs8Der(out: []u8): usize;
    fn fromPkcs8Der(der: &[]u8): Ed25519SecretKey?;
    fn fromHex(hex: string): Ed25519SecretKey?;
    fn toHex(out: []u8): usize;
    fn eql(other: Ed25519SecretKey): bool;
}

