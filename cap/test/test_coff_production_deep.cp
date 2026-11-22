// Source: cp/cap/test/test_coff_production_deep.cp (Production deep tests for cap:coff)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    COFF_HEADER_SIZE,
    COFF_SECTION_SIZE,
    COFF_SYMBOL_SIZE,
    COFF_RELOC_SIZE,
    MACHINE_UNKNOWN,
    MACHINE_I386,
    MACHINE_ARM,
    MACHINE_AMD64,
    MACHINE_ARM64,
    MACHINE_RISCV64,
    SCN_CNT_CODE,
    SCN_CNT_INITIALIZED_DATA,
    SCN_CNT_UNINITIALIZED_DATA,
    SCN_MEM_EXECUTE,
    SCN_MEM_READ,
    SCN_MEM_WRITE,
    SYM_CLASS_EXTERNAL,
    SYM_CLASS_STATIC,
    SYM_CLASS_FILE,
    SYM_CLASS_SECTION,
    SYM_UNDEFINED,
    SYM_ABSOLUTE,
    SYM_DEBUG,
    isKnownCoffMachine,
    CoffHeader,
    CoffSection,
    CoffSymbol,
    CoffReloc,
    CoffObject,
    parseCoff,
} from "cap:binfmt";

fn putU16Le(buf: []u8, off: usize, v: u16) {
    buf[off + 0] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putI16Le(buf: []u8, off: usize, v: i16) {
    const u: u16 = @bitCast(v);
    putU16Le(buf, off, u);
}

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off + 0] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putBytes(buf: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
}

fn slicesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

// Builds a complete synthetic AMD64 COFF .obj with:
// - 3 sections: ".text" (with 4B code + 2 relocs), ".rdata" (4B data), "/4" (long name ".debug_info")
// - 3 symbols: "main" (short external func), long symbol "cp_very_long_exported_symbol" (strtab off 16), "abs_sym" (SYM_ABSOLUTE)
// - String table: [len:4B] ".debug_info\0" (off 4, len 12) "cp_very_long_exported_symbol\0" (off 16, len 29) -> total 45B
fn buildSampleObj(machine: u16): [320]u8 {
    let buf: [320]u8 = std.mem.zeroes([320]u8);

    // Offsets:
    // 0..20:    CoffHeader (20B)
    // 20..140:  3 * 40B Sections (120B)
    // 140..144: .text raw data (4B: 0x48 0x89 0xE5 0xC3)
    // 144..164: .text 2 * 10B relocs (20B)
    // 164..168: .rdata raw data (4B: 0xDE 0xAD 0xBE 0xEF)
    // 168..222: 3 * 18B Symbols (54B)
    // 222..267: String Table (45B)
    putU16Le(buf[0..], 0, machine);
    putU16Le(buf[0..], 2, 3);           // num_sections
    putU32Le(buf[0..], 4, 1700000000);  // timestamp
    putU32Le(buf[0..], 8, 168);         // ptr_to_symbol_table
    putU32Le(buf[0..], 12, 3);          // num_symbols
    putU16Le(buf[0..], 16, 0);          // size_of_opt_header
    putU16Le(buf[0..], 18, 0x0000);     // characteristics

    // Section 0: ".text"
    const s0: usize = 20;
    putBytes(buf[0..], s0, ".text");
    putU32Le(buf[0..], s0 + 8, 4);      // virtual_size
    putU32Le(buf[0..], s0 + 16, 4);     // size_of_raw_data
    putU32Le(buf[0..], s0 + 20, 140);   // ptr_to_raw_data
    putU32Le(buf[0..], s0 + 24, 144);   // ptr_to_relocs
    putU16Le(buf[0..], s0 + 32, 2);     // num_relocs
    putU32Le(buf[0..], s0 + 36, SCN_CNT_CODE | SCN_MEM_EXECUTE | SCN_MEM_READ);

    // Section 1: ".rdata"
    const s1: usize = 60;
    putBytes(buf[0..], s1, ".rdata");
    putU32Le(buf[0..], s1 + 16, 4);
    putU32Le(buf[0..], s1 + 20, 164);
    putU32Le(buf[0..], s1 + 36, SCN_CNT_INITIALIZED_DATA | SCN_MEM_READ);

    // Section 2: "/4" -> ".debug_info"
    const s2: usize = 100;
    putBytes(buf[0..], s2, "/4");
    putU32Le(buf[0..], s2 + 16, 0);
    putU32Le(buf[0..], s2 + 36, SCN_CNT_INITIALIZED_DATA | SCN_MEM_READ | SCN_MEM_WRITE);

    // .text bytes
    buf[140] = 0x48;
    buf[141] = 0x89;
    buf[142] = 0xE5;
    buf[143] = 0xC3;

    // .text reloc 0 & 1
    putU32Le(buf[0..], 144, 0x0001); // virtual_addr
    putU32Le(buf[0..], 148, 1);      // symbol_idx
    putU16Le(buf[0..], 152, 4);      // IMAGE_REL_AMD64_REL32

    putU32Le(buf[0..], 154, 0x0003);
    putU32Le(buf[0..], 158, 0);
    putU16Le(buf[0..], 162, 1);      // IMAGE_REL_AMD64_ADDR64

    // .rdata bytes
    buf[164] = 0xDE;
    buf[165] = 0xAD;
    buf[166] = 0xBE;
    buf[167] = 0xEF;

    // Symbol 0: "main" (external function in section 1)
    const sym0: usize = 168;
    putBytes(buf[0..], sym0, "main");
    putU32Le(buf[0..], sym0 + 8, 0);
    putI16Le(buf[0..], sym0 + 12, 1);
    putU16Le(buf[0..], sym0 + 14, 0x20); // function
    buf[sym0 + 16] = SYM_CLASS_EXTERNAL;
    buf[sym0 + 17] = 0;

    // Symbol 1: long symbol at strtab offset 16 (undefined external)
    const sym1: usize = 186;
    putU32Le(buf[0..], sym1 + 0, 0);     // zeroes == 0
    putU32Le(buf[0..], sym1 + 4, 16);    // strtab offset
    putU32Le(buf[0..], sym1 + 8, 0);
    putI16Le(buf[0..], sym1 + 12, SYM_UNDEFINED);
    putU16Le(buf[0..], sym1 + 14, 0x20);
    buf[sym1 + 16] = SYM_CLASS_EXTERNAL;

    // Symbol 2: "abs_sym" (static absolute)
    const sym2: usize = 204;
    putBytes(buf[0..], sym2, "abs_sym");
    putU32Le(buf[0..], sym2 + 8, 0x1234);
    putI16Le(buf[0..], sym2 + 12, SYM_ABSOLUTE);
    putU16Le(buf[0..], sym2 + 14, 0);
    buf[sym2 + 16] = SYM_CLASS_STATIC;

    // String table at 222 (total size = 4 + 12 + 29 = 45)
    putU32Le(buf[0..], 222, 45);
    putBytes(buf[0..], 226, ".debug_info");
    buf[237] = 0;
    putBytes(buf[0..], 238, "cp_very_long_exported_symbol");
    buf[266] = 0;

    return buf;
}

test("1. AMD64 & ARM64 COFF header parsing and machine ID validation", () => {
    expect_eq(COFF_HEADER_SIZE, 20 as usize);
    expect_eq(COFF_SECTION_SIZE, 40 as usize);
    expect_eq(COFF_SYMBOL_SIZE, 18 as usize);
    expect_eq(COFF_RELOC_SIZE, 10 as usize);

    expect_eq(isKnownCoffMachine(MACHINE_AMD64), true);
    expect_eq(isKnownCoffMachine(MACHINE_ARM64), true);
    expect_eq(isKnownCoffMachine(MACHINE_I386), true);
    expect_eq(isKnownCoffMachine(MACHINE_ARM), true);
    expect_eq(isKnownCoffMachine(MACHINE_RISCV64), true);
    expect_eq(isKnownCoffMachine(MACHINE_UNKNOWN), true);
    expect_eq(isKnownCoffMachine(0xBEEF), false);

    const amd64_raw = buildSampleObj(MACHINE_AMD64);
    if (obj := parseCoff(amd64_raw[0..267])) {
        expect_eq(obj.header.machine, MACHINE_AMD64);
        expect_eq(obj.header.num_sections, 3 as u16);
        expect_eq(obj.header.timestamp, 1700000000 as u32);
        expect_eq(obj.header.ptr_to_symbol_table, 168 as u32);
        expect_eq(obj.header.num_symbols, 3 as u32);
    } else {
        expect_eq(true, false);
    }

    const arm64_raw = buildSampleObj(MACHINE_ARM64);
    if (obj := parseCoff(arm64_raw[0..267])) {
        expect_eq(obj.header.machine, MACHINE_ARM64);
    } else {
        expect_eq(true, false);
    }
});

test("2. Section headers: short names ('.text', '.rdata'), '/<offset>' long names ('.debug_info'), and flags", () => {
    const raw = buildSampleObj(MACHINE_AMD64);
    if (obj := parseCoff(raw[0..267])) {
        if (s0 := obj.sectionAt(0)) {
            expect_eq(slicesEq(s0.shortName(), ".text"), true);
            expect_eq(s0.isCode(), true);
            expect_eq(s0.isExecutable(), true);
            expect_eq(s0.isReadable(), true);
            expect_eq(s0.isWritable(), false);
            if (name := obj.sectionName(s0)) {
                expect_eq(slicesEq(name, ".text"), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        if (s1 := obj.sectionAt(1)) {
            expect_eq(slicesEq(s1.shortName(), ".rdata"), true);
            expect_eq(s1.isCode(), false);
            expect_eq(s1.isReadable(), true);
            expect_eq(s1.isWritable(), false);
        } else {
            expect_eq(true, false);
        }

        if (s2 := obj.sectionAt(2)) {
            expect_eq(slicesEq(s2.shortName(), "/4"), true);
            expect_eq(s2.isWritable(), true);
            if (long_name := obj.sectionName(s2)) {
                expect_eq(slicesEq(long_name, ".debug_info"), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        // Out-of-bounds section index
        if (_ := obj.sectionAt(3)) {
            expect_eq(true, false);
        } else {
            expect_eq(true, true);
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. Symbol table: short names, zeroes==0 long string-table names, and storage/section predicates", () => {
    const raw = buildSampleObj(MACHINE_AMD64);
    if (obj := parseCoff(raw[0..267])) {
        // Symbol 0: "main"
        if (sym0 := obj.symbolAt(0)) {
            expect_eq(sym0.isExternal(), true);
            expect_eq(sym0.isStatic(), false);
            expect_eq(sym0.isUndefined(), false);
            expect_eq(sym0.isFunction(), true);
            if (n0 := obj.symbolName(sym0)) {
                expect_eq(slicesEq(n0, "main"), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        // Symbol 1: long name from string table
        if (sym1 := obj.symbolAt(1)) {
            expect_eq(sym1.isExternal(), true);
            expect_eq(sym1.isUndefined(), true);
            expect_eq(sym1.isFunction(), true);
            if (off := sym1.str_table_offset) {
                expect_eq(off, 16 as u32);
            } else {
                expect_eq(true, false);
            }
            if (n1 := obj.symbolName(sym1)) {
                expect_eq(slicesEq(n1, "cp_very_long_exported_symbol"), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        // Symbol 2: static absolute
        if (sym2 := obj.symbolAt(2)) {
            expect_eq(sym2.isStatic(), true);
            expect_eq(sym2.isExternal(), false);
            expect_eq(sym2.isFunction(), false);
            expect_eq(sym2.section_num, SYM_ABSOLUTE);
            expect_eq(sym2.value, 0x1234 as u32);
            if (n2 := obj.symbolName(sym2)) {
                expect_eq(slicesEq(n2, "abs_sym"), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        expect_eq(SYM_CLASS_FILE, 103 as u8);
        expect_eq(SYM_CLASS_SECTION, 104 as u8);
        expect_eq(SYM_DEBUG, -2 as i16);
        expect_eq(SCN_CNT_UNINITIALIZED_DATA, 0x00000080 as u32);
    } else {
        expect_eq(true, false);
    }
});

test("4. Zero-copy sectionData slicing and 10B relocation table extraction", () => {
    const raw = buildSampleObj(MACHINE_AMD64);
    if (obj := parseCoff(raw[0..267])) {
        if (s0 := obj.sectionAt(0)) {
            if (code := obj.sectionData(s0)) {
                expect_eq(code.len, 4 as usize);
                expect_eq(code[0], 0x48 as u8);
                expect_eq(code[3], 0xC3 as u8);
            } else {
                expect_eq(true, false);
            }

            if (r0 := obj.relocAt(s0, 0)) {
                expect_eq(r0.virtual_addr, 0x0001 as u32);
                expect_eq(r0.symbol_idx, 1 as u32);
                expect_eq(r0.reloc_type, 4 as u16);
            } else {
                expect_eq(true, false);
            }

            if (r1 := obj.relocAt(s0, 1)) {
                expect_eq(r1.virtual_addr, 0x0003 as u32);
                expect_eq(r1.symbol_idx, 0 as u32);
                expect_eq(r1.reloc_type, 1 as u16);
            } else {
                expect_eq(true, false);
            }

            // Out-of-bounds reloc index
            if (_ := obj.relocAt(s0, 2)) {
                expect_eq(true, false);
            } else {
                expect_eq(true, true);
            }
        } else {
            expect_eq(true, false);
        }

        if (s1 := obj.sectionAt(1)) {
            if (rdata := obj.sectionData(s1)) {
                expect_eq(rdata.len, 4 as usize);
                expect_eq(rdata[0], 0xDE as u8);
                expect_eq(rdata[3], 0xEF as u8);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. Malformed COFF rejection: unknown machine, out-of-bounds section/symbol/string tables, and truncation", () => {
    // 5a. Unknown machine ID
    let bad_mach = buildSampleObj(0xDEAD);
    if (_ := parseCoff(bad_mach[0..267])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. Truncated header (< 20B)
    const ok_raw = buildSampleObj(MACHINE_AMD64);
    if (_ := parseCoff(ok_raw[0..19])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. Section table overflows buffer
    if (_ := parseCoff(ok_raw[0..100])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5d. Symbol table overflows buffer
    if (_ := parseCoff(ok_raw[0..200])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5e. String table length overflows buffer
    if (_ := parseCoff(ok_raw[0..250])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5f. ptr_to_symbol_table == 0 with non-zero num_symbols
    let bad_sym: [20]u8 = std.mem.zeroes([20]u8);
    putU16Le(bad_sym[0..], 0, MACHINE_AMD64);
    putU32Le(bad_sym[0..], 12, 5); // num_symbols = 5, ptr = 0
    if (_ := parseCoff(bad_sym[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});
