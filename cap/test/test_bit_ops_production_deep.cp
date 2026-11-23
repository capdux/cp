import { test, expect } from "cap:test";
import {
    testBit64,
    setBit64,
    clearBit64,
    toggleBit64,
    intersects64,
    containsAll64,
    maskOut64,
    extractBits64,
    insertBits64,
    popcount64,
    clz64,
    ctz64,
    isPowerOfTwo,
    nextPowerOfTwo,
    alignUp,
    alignDown,
    clearLowestSetBit,
    isolateLowestSetBit,
    BitSetIter64,
} from "cap:math";

test("bit_ops: bit masks, bitfields, popcount, and power-of-two alignment", () => {
    let m: u64 = 0;
    m = setBit64(m, 3);
    m = setBit64(m, 10);
    expect(testBit64(m, 3) && testBit64(m, 10) && !testBit64(m, 4));
    expect(popcount64(m) == 2);
    expect(ctz64(m) == 3);
    expect(clz64(m) == 53);

    expect(intersects64(m, setBit64(0, 10)));
    expect(containsAll64(m, setBit64(0, 3)));
    expect(maskOut64(m, setBit64(0, 3)) == setBit64(0, 10));
    expect(clearBit64(m, 10) == setBit64(0, 3));
    expect(toggleBit64(m, 3) == setBit64(0, 10));

    // Bitfield insert & extract
    const packed = insertBits64(0xFFFF_0000, 0xAB, 8, 8);
    expect(extractBits64(packed, 8, 8) == 0xAB);
    expect(extractBits64(packed, 16, 16) == 0xFFFF);

    expect(isPowerOfTwo(64) && !isPowerOfTwo(0) && !isPowerOfTwo(63));
    expect(nextPowerOfTwo(0) == 1);
    expect(nextPowerOfTwo(17) == 32);
    expect(nextPowerOfTwo(32) == 32);

    expect(alignUp(13, 8) == 16);
    expect(alignDown(13, 8) == 8);
    expect(isolateLowestSetBit(0b101100) == 0b000100);
    expect(clearLowestSetBit(0b101100) == 0b101000);
});

test("bit_ops: BitSetIter64 ascending iteration", () => {
    const mask = setBit64(setBit64(setBit64(0, 1), 7), 63);
    let iter = BitSetIter64.init(mask);
    expect(iter.next().? == 1);
    expect(iter.next().? == 7);
    expect(iter.next().? == 63);
    expect(iter.next() == null);
});
