import { test, expect } from "cap:test";
import {
    isByteString,
    isSlice,
    isSingleItemPtr,
    isConstPtr,
    isOptional,
    isInteger,
    isFloat,
    isNumber,
    isStruct,
    isEnum,
    isUnion,
    isContainer,
    isExternContainer,
    isPackedContainer,
    isIndexable,
    PointerKind,
    PointerParseOptions,
    PointerMeta,
} from "cap:type_traits";

struct PlainStruct {
    a: u32,
    b: u64,
}

@c struct ExternStruct {
    x: u32,
    y: u32,
}

@packed struct PackedFlags {
    a: u4,
    b: u4,
}

enum Color: u8 {
    red = 0,
    green = 1,
}

type MutU32Ptr = *u32!;
type ConstU32Ptr = *const u32!;
type ConstU8ArrPtr = *const [4]u8!;
type ConstU8Ptr = *const u8!;
type OptConstStructPtr = (*const PlainStruct!)?;

test("type_traits: numeric, optional, and container predicates", () => {
    expect(comptime isInteger(i32)).toBeTrue();
    expect(comptime isInteger(u64)).toBeTrue();
    expect(comptime isInteger(f64)).toBeFalse();
    expect(comptime isFloat(f32)).toBeTrue();
    expect(comptime isFloat(f64)).toBeTrue();
    expect(comptime isNumber(u16)).toBeTrue();
    expect(comptime isNumber(f64)).toBeTrue();
    expect(comptime isNumber(bool)).toBeFalse();

    expect(comptime isOptional(u32?)).toBeTrue();
    expect(comptime isOptional(u32)).toBeFalse();

    expect(comptime isStruct(PlainStruct)).toBeTrue();
    expect(comptime isEnum(Color)).toBeTrue();
    expect(comptime isContainer(PlainStruct)).toBeTrue();
    expect(comptime isContainer(Color)).toBeTrue();
    expect(comptime isContainer(u32)).toBeFalse();
    expect(comptime isExternContainer(ExternStruct)).toBeTrue();
    expect(comptime isExternContainer(PlainStruct)).toBeFalse();
    expect(comptime isPackedContainer(PackedFlags)).toBeTrue();
    expect(comptime isPackedContainer(PlainStruct)).toBeFalse();
});

test("type_traits: string, slice, pointer, and indexable predicates", () => {
    expect(comptime isByteString([]const u8)).toBeTrue();
    expect(comptime isByteString([]u8)).toBeTrue();
    expect(comptime isByteString(ConstU8ArrPtr)).toBeTrue();
    expect(comptime isByteString([]const u16)).toBeFalse();
    expect(comptime isByteString(u8)).toBeFalse();
    expect(comptime isByteString(([]const u8)?)).toBeFalse();

    expect(comptime isSlice([]const u8)).toBeTrue();
    expect(comptime isSlice([]u32)).toBeTrue();
    expect(comptime isSlice(MutU32Ptr)).toBeFalse();

    expect(comptime isSingleItemPtr(MutU32Ptr)).toBeTrue();
    expect(comptime isSingleItemPtr(ConstU32Ptr)).toBeTrue();
    expect(comptime isSingleItemPtr([]u32)).toBeFalse();

    expect(comptime isConstPtr(ConstU32Ptr)).toBeTrue();
    expect(comptime isConstPtr([]const u8)).toBeTrue();
    expect(comptime isConstPtr(MutU32Ptr)).toBeFalse();
    expect(comptime isConstPtr([]u8)).toBeFalse();

    expect(comptime isIndexable([4]u8)).toBeTrue();
    expect(comptime isIndexable([]const u8)).toBeTrue();
    expect(comptime isIndexable(ConstU8ArrPtr)).toBeTrue();
    expect(comptime isIndexable(ConstU8Ptr)).toBeFalse();
    expect(comptime isIndexable(u32)).toBeFalse();
});

test("type_traits: PointerMeta.inspect on single/slice/optional/const pointers", () => {
    const m1 = comptime PointerMeta.inspect(MutU32Ptr, PointerParseOptions{});
    expect(m1.kind == .single).toBeTrue();
    expect(m1.is_optional).toBeFalse();
    expect(m1.is_const).toBeFalse();
    expect(m1.child_size).toBe(4);
    expect(m1.alignment).toBe(4);

    const m2 = comptime PointerMeta.inspect(([]const u64)?, PointerParseOptions{});
    expect(m2.kind == .slice).toBeTrue();
    expect(m2.is_optional).toBeTrue();
    expect(m2.is_const).toBeTrue();
    expect(m2.child_size).toBe(8);
    expect(m2.alignment).toBe(8);

    const m3 = comptime PointerMeta.inspect(OptConstStructPtr, PointerParseOptions{ allow_slices: false });
    expect(m3.kind == .single).toBeTrue();
    expect(m3.is_optional).toBeTrue();
    expect(m3.is_const).toBeTrue();
    expect(m3.child_size).toBe(@sizeOf(PlainStruct));
});
