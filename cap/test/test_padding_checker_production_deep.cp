import { test, expect_eq } from "cap:test";
import {
    paddingBytes,
    hasPadding,
    hasPointers,
    isSafeBinarySerializable,
    assertNoUninitializedPadding,
} from "cap:padding_checker";

@c struct CleanRecord {
    a: u8 = 0,
    _pad: [7]u8 = [0, 0, 0, 0, 0, 0, 0],
    b: u64 = 0,
}

@c struct PaddedRecord {
    a: u8 = 0,
    b: u64 = 0,
}

@c struct TrailingPadRecord {
    b: u64 = 0,
    a: u8 = 0,
}

@c struct NestedClean {
    hdr: CleanRecord = CleanRecord{},
    tail: u64 = 0,
}

@c struct NestedPadded {
    hdr: PaddedRecord = PaddedRecord{},
    tail: u64 = 0,
}

struct WithPointer {
    ptr: (*u8!)? = null,
    len: usize = 0,
}

test("1. Clean explicitly-padded extern struct passes all checks", () => {
    assertNoUninitializedPadding(CleanRecord);
    expect_eq(paddingBytes(CleanRecord), usize(0));
    expect_eq(hasPadding(CleanRecord), false);
    expect_eq(hasPointers(CleanRecord), false);
    expect_eq(isSafeBinarySerializable(CleanRecord), true);
});

test("2. Detects internal implicit padding between u8 and u64", () => {
    expect_eq(paddingBytes(PaddedRecord), usize(7));
    expect_eq(hasPadding(PaddedRecord), true);
    expect_eq(hasPointers(PaddedRecord), false);
    expect_eq(isSafeBinarySerializable(PaddedRecord), false);
});

test("3. Detects trailing padding at end of extern struct", () => {
    expect_eq(paddingBytes(TrailingPadRecord), usize(7));
    expect_eq(hasPadding(TrailingPadRecord), true);
    expect_eq(isSafeBinarySerializable(TrailingPadRecord), false);
});

test("4. Recursive check catches nested struct padding", () => {
    assertNoUninitializedPadding(NestedClean);
    expect_eq(hasPadding(NestedClean), false);
    expect_eq(isSafeBinarySerializable(NestedClean), true);

    expect_eq(hasPadding(NestedPadded), true);
    expect_eq(isSafeBinarySerializable(NestedPadded), false);
});

test("5. Detects pointers and scalars cleanly", () => {
    expect_eq(hasPointers(WithPointer), true);
    expect_eq(isSafeBinarySerializable(WithPointer), false);

    assertNoUninitializedPadding(u64);
    expect_eq(paddingBytes(u64), usize(0));
    expect_eq(isSafeBinarySerializable(u64), true);
});
