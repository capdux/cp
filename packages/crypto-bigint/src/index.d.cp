// Type definitions for CP

export fn cbiMask64(): u128;
export fn cbiAllOnes64(): u64;
export fn cbiCtMask(cond: bool): u64;
export fn cbiReadBeU64(b: &[]u8, off: usize): u64;
export fn cbiReadLeU64(b: &[]u8, off: usize): u64;
export fn cbiWriteBeU64(v: u64, out: []u8, off: usize);
export fn cbiWriteLeU64(v: u64, out: []u8, off: usize);
export fn cbiHexNibble(ch: u8): i32;
export fn cbiHexChar(nibble: u8): u8;
export fn cbiBitLen64(v_in: u64): usize;
export class CbiAddRes {
    val: U256,
    carry: u64,
}

export class CbiSubRes {
    val: U256,
    borrow: u64,
}

export fn cbiAddWithCarry(a: U256, b: U256): CbiAddRes;
export fn cbiSubWithBorrow(a: U256, b: U256): CbiSubRes;
export class U256 {
    limbs: [4]u64,
    fn fromU64(v: u64): U256;
    fn fromBeBytes(b: [32]u8): U256;
    fn fromLeBytes(b: [32]u8): U256;
    fn toBeBytes(): [32]u8;
    fn toLeBytes(): [32]u8;
    fn fromBeHex(hex: string): U256?;
    fn toHex(out: []u8): usize;
    fn wrappingAdd(other: U256): U256;
    fn wrappingSub(other: U256): U256;
    fn wrappingMul(other: U256): U256;
    fn shl1(): U256;
    fn shr1(): U256;
    fn bitLen(): usize;
    fn ctEq(other: U256): bool;
    fn ctCmp(other: U256): i32;
    fn ctSelect(cond: bool, a: U256, b: U256): U256;
    fn addMod(a: U256, b: U256, m: U256): U256;
    fn subMod(a: U256, b: U256, m: U256): U256;
}

export fn addMod(a: U256, b: U256, m: U256): U256;
export fn subMod(a: U256, b: U256, m: U256): U256;
