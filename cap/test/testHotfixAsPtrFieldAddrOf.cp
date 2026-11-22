import { describe, test, expect_eq } from "cap:test";

struct PtrHolder {
    p: [*]u8!,
    cp: [*]const u8!,
    raw: *anyopaque!,
    opt: ([*]u8!)?,

    pub fn castU64(): *u64! {
        return unsafe { _.p as *u64! };
    }

    pub fn castConstU64(): *const u64! {
        return unsafe { _.cp as *const u64! };
    }

    pub fn castConstU32(): *const u32! {
        return unsafe { _.p as *const u32! };
    }

    pub fn castRawU64(): *u64! {
        return unsafe { _.raw as *u64! };
    }

    pub fn castOptU64(): *u64! {
        return unsafe { _.opt.? as *u64! };
    }
}

fn getManyPtr(p: [*]u8!): [*]u8! {
    return p;
}

describe("Hotfix: as *T on pointer fields must not take address-of pointer variable", () => {
    test("struct [*]u8 / [*]const u8 / *anyopaque / ([*]u8)? field casts read and write target memory", () => {
        let val: u64 = 0x1122334455667788;
        let many_mut: [*]u8! = unsafe { (*val as *u64!) as [*]u8! };
        let many_const: [*]const u8! = unsafe { (&val as *const u64!) as [*]const u8! };
        let raw_ptr: *anyopaque! = unsafe { *val as *anyopaque! };

        let holder = PtrHolder {
            p: many_mut,
            cp: many_const,
            raw: raw_ptr,
            opt: many_mut,
        };

        // 1. _.p as *u64! (reads target memory, NOT &self.p on stack)
        let p_u64 = holder.castU64();
        expect_eq(unsafe { p_u64.* }, 0x1122334455667788);

        // 2. _.cp as *const u64!
        let cp_u64 = holder.castConstU64();
        expect_eq(unsafe { cp_u64.* }, 0x1122334455667788);

        // 3. _.p as *const u32! (low 32 bits on little-endian)
        let cp_u32 = holder.castConstU32();
        expect_eq(unsafe { cp_u32.* }, 0x55667788);

        // 4. _.raw as *u64! and _.opt.? as *u64!
        expect_eq(unsafe { holder.castRawU64().* }, 0x1122334455667788);
        expect_eq(unsafe { holder.castOptU64().* }, 0x1122334455667788);

        // 5. Write through _.p as *u64! mutates `val`, NOT stack slot of holder.p
        unsafe {
            p_u64.* = 0xAABBCCDDEEFF0011;
        }
        expect_eq(val, 0xAABBCCDDEEFF0011);

        // 6. External member access: obj.p as *u32!
        let obj_u32: *u32! = unsafe { holder.p as *u32! };
        expect_eq(unsafe { obj_u32.* }, 0xEEFF0011);
        unsafe {
            obj_u32.* = 0x12345678;
        }
        expect_eq(val, 0xAABBCCDD12345678);
    });

    test("local [*]u8, array of pointers arr[i], and getPtr() as *u64", () => {
        let a: u64 = 0x0102030405060708;
        let b: u64 = 0x090A0B0C0D0E0F10;
        let pa: [*]u8! = unsafe { (*a as *u64!) as [*]u8! };
        let pb: [*]u8! = unsafe { (*b as *u64!) as [*]u8! };

        // Local [*]u8 as *u64
        let la: *u64! = unsafe { pa as *u64! };
        expect_eq(unsafe { la.* }, 0x0102030405060708);

        // Array of pointers: arr[i] as *u64
        let ptrs: [2][*]u8! = [pa, pb];
        let p0: *u64! = unsafe { ptrs[0] as *u64! };
        let p1: *u64! = unsafe { ptrs[1] as *u64! };
        expect_eq(unsafe { p0.* }, 0x0102030405060708);
        expect_eq(unsafe { p1.* }, 0x090A0B0C0D0E0F10);

        // Function returning [*]u8: getPtr() as *u64
        let from_fn: *u64! = unsafe { getManyPtr(pb) as *u64! };
        expect_eq(unsafe { from_fn.* }, 0x090A0B0C0D0E0F10);

        // Fixed array regression check: [4]u8 as [*]u8! still takes address of array
        let arr: [4]u8 = [10, 20, 30, 40];
        let arr_ptr: [*]u8! = unsafe { arr as [*]u8! };
        expect_eq(unsafe { arr_ptr[0] }, 10);
        expect_eq(unsafe { arr_ptr[3] }, 40);
    });
});
