import { test, expect_eq } from "cap:test";

@c pub struct PackedHeader {
    tag: u8 align(1),
    val: u32 align(1),
}

@c pub struct DefaultAlignedHeader {
    tag: u8 align(1) = 0,
    val: u32 align(1) = 123,
}

fn main() {
    test("@c pub struct field align(N) parsing and layout", () => {
        const h = PackedHeader{ tag: 7, val: 0x12345678 };
        expect_eq(h.tag, 7);
        expect_eq(h.val, 0x12345678);
        expect_eq(@sizeOf(PackedHeader), 5);
        expect_eq(@alignOf(PackedHeader), 1);

        const d = DefaultAlignedHeader{};
        expect_eq(d.tag, 0);
        expect_eq(d.val, 123);
        expect_eq(@sizeOf(DefaultAlignedHeader), 5);
    });
}
