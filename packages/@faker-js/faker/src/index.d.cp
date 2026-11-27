// Type definitions for CP

export fn splitmixStep(state: &mut u64): u64;
export fn rotl7(x: u64): u64;
export fn rotl45(x: u64): u64;
export fn appendByte(out: []u8, pos: &mut usize, b: u8);
export fn appendBytes(out: []u8, pos: &mut usize, data: &[]u8);
export fn appendLowerAscii(out: []u8, pos: &mut usize, data: &[]u8);
export fn appendCapitalized(out: []u8, pos: &mut usize, data: &[]u8);
export fn appendU64Dec(out: []u8, pos: &mut usize, val: u64);
export fn appendDecPadded(out: []u8, pos: &mut usize, val: u64, width: usize);
export fn appendHexByte(out: []u8, pos: &mut usize, b: u8);
export fn isLeapYear(year: u64): bool;
export fn daysInMonth(year: u64, month: u64): u64;
export fn isValidLuhn(digits: &[]u8): bool;
export class Faker {
    s0: u64,
    s1: u64,
    s2: u64,
    s3: u64,
    fn init(seed: u64): Faker;
    fn reseed(_: auto, seed: u64);
    fn nextU64(_: auto): u64;
    fn nextInt(_: auto, min: i64, max: i64): i64;
    fn nextBool(_: auto, prob_pct: u8): bool;
    fn pickStr(_: auto, items: &[]&[]u8): &[]u8;
    fn nextRangeU64(_: auto, lo: u64, hi: u64): u64;
    fn firstName(_: auto): &[]u8;
    fn lastName(_: auto): &[]u8;
    fn fullName(_: auto, out: []u8): &[]u8;
    fn username(_: auto, out: []u8): &[]u8;
    fn email(_: auto, out: []u8): &[]u8;
    fn phone(_: auto, out: []u8): &[]u8;
    fn ipv4(_: auto, out: []u8): &[]u8;
    fn ipv6(_: auto, out: []u8): &[]u8;
    fn domain(_: auto, out: []u8): &[]u8;
    fn url(_: auto, out: []u8): &[]u8;
    fn uuidV4(_: auto, out: []u8): &[]u8;
    fn hexColor(_: auto, out: []u8): &[]u8;
    fn word(_: auto): &[]u8;
    fn sentence(_: auto, word_count: usize, out: []u8): &[]u8;
    fn paragraph(_: auto, sentence_count: usize, out: []u8): &[]u8;
    fn company(_: auto, out: []u8): &[]u8;
    fn city(_: auto): &[]u8;
    fn country(_: auto): &[]u8;
    fn isoDate(_: auto, out: []u8): &[]u8;
    fn creditCardNumber(_: auto, out: []u8): &[]u8;
    fn shuffleU64(_: auto, items: []u64);
    fn sampleIndices(_: auto, population_len: usize, out: []usize): usize;
}

