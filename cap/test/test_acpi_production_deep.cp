// Source: cp/cap/test/test_acpi_production_deep.cp (Production deep tests for cap:acpi)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    RSDP_SIGNATURE,
    RSDP_V1_SIZE,
    RSDP_V2_SIZE,
    SDT_HEADER_SIZE,
    SIG_RSDT,
    SIG_XSDT,
    SIG_FACP,
    SIG_APIC,
    SIG_MCFG,
    SIG_HPET,
    SIG_DSDT,
    MADT_LOCAL_APIC,
    MADT_IO_APIC,
    MADT_INT_SRC_OVERRIDE,
    MADT_NMI_SOURCE,
    MADT_LOCAL_APIC_NMI,
    MADT_LOCAL_APIC_ADDR_OVERRIDE,
    MADT_LOCAL_X2APIC,
    acpiChecksum,
    verifyAcpiChecksum,
    AcpiRsdp,
    parseAcpiRsdp,
    AcpiSdtHeader,
    AcpiTable,
    parseAcpiTable,
    rsdtEntryCount,
    rsdtEntryAt,
    xsdtEntryCount,
    xsdtEntryAt,
    MadtEntry,
    MadtEntryIter,
    MadtTable,
    parseMadt,
    MadtLocalApic,
    parseMadtLocalApic,
    MadtIoApic,
    parseMadtIoApic,
    MadtIntOverride,
    parseMadtIntOverride,
    McfgEntry,
    McfgEntryIter,
    parseMcfgEntries,
} from "cap:sys";

fn putU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putU64Le(buf: []u8, off: usize, v: u64) {
    putU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    putU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn fixChecksumAt(buf: []u8, cksum_off: usize) {
    buf[cksum_off] = 0;
    const rem = acpiChecksum(buf);
    buf[cksum_off] = ((256 - (rem as u32)) & 0xFF) as u8;
}

fn initSdtHeader(buf: []u8, sig: []const u8, total_len: u32, rev: u8) {
    copyBytes(buf, 0, sig);
    putU32Le(buf, 4, total_len);
    buf[8] = rev;
    copyBytes(buf, 10, "CPARCH");
    copyBytes(buf, 16, "CPTABLE1");
    putU32Le(buf, 24, 1);
    copyBytes(buf, 28, "CPAC");
    putU32Le(buf, 32, 0x00010000);
}

test("1. RSDP v1 (20B) & v2 (36B) dual-checksum parsing", () => {
    expect_eq(RSDP_V1_SIZE, 20 as usize);
    expect_eq(RSDP_V2_SIZE, 36 as usize);
    expect_eq(SDT_HEADER_SIZE, 36 as usize);
    expect_eq(std.mem.eql(u8, SIG_FACP, "FACP"), true);
    expect_eq(std.mem.eql(u8, SIG_HPET, "HPET"), true);
    expect_eq(std.mem.eql(u8, SIG_DSDT, "DSDT"), true);
    expect_eq(MADT_NMI_SOURCE, 3 as u8);
    expect_eq(MADT_LOCAL_APIC_NMI, 4 as u8);
    expect_eq(MADT_LOCAL_APIC_ADDR_OVERRIDE, 5 as u8);
    expect_eq(MADT_LOCAL_X2APIC, 9 as u8);

    // RSDP v1 (revision = 0)
    let r1: [20]u8 = std.mem.zeroes([20]u8);
    copyBytes(r1[0..], 0, RSDP_SIGNATURE);
    copyBytes(r1[0..], 9, "BOCHS ");
    r1[15] = 0;
    putU32Le(r1[0..], 16, 0x7FFE0000);
    fixChecksumAt(r1[0..], 8);

    const p1 = parseAcpiRsdp(r1[0..]) ?? unreachable;
    expect_eq(p1.revision, 0 as u8);
    expect_eq(p1.is_v2, false);
    expect_eq(p1.rsdt_addr, 0x7FFE0000 as u32);
    expect_eq(p1.xsdt_addr, 0 as u64);
    expect_eq(std.mem.eql(u8, p1.oem_id[0..], "BOCHS "), true);

    // RSDP v2 (revision = 2) with both v1 and extended checksums
    let r2: [36]u8 = std.mem.zeroes([36]u8);
    copyBytes(r2[0..], 0, RSDP_SIGNATURE);
    copyBytes(r2[0..], 9, "CPFW  ");
    r2[15] = 2;
    putU32Le(r2[0..], 16, 0x7FFE1000);
    fixChecksumAt(r2[0..20], 8);
    putU32Le(r2[0..], 20, 36);
    putU64Le(r2[0..], 24, 0x00000001_80000000);
    fixChecksumAt(r2[0..], 32);

    const p2 = parseAcpiRsdp(r2[0..]) ?? unreachable;
    expect_eq(p2.revision, 2 as u8);
    expect_eq(p2.is_v2, true);
    expect_eq(p2.length, 36 as u32);
    expect_eq(p2.rsdt_addr, 0x7FFE1000 as u32);
    expect_eq(p2.xsdt_addr, 0x00000001_80000000 as u64);
});

test("2. RSDT 32-bit & XSDT 64-bit table pointer extraction", () => {
    // RSDT with 2 entries: 36 + 8 = 44B
    let rsdt_buf: [44]u8 = std.mem.zeroes([44]u8);
    initSdtHeader(rsdt_buf[0..], SIG_RSDT, 44, 1);
    putU32Le(rsdt_buf[0..], 36, 0x7FFE2000);
    putU32Le(rsdt_buf[0..], 40, 0x7FFE3000);
    fixChecksumAt(rsdt_buf[0..], 9);

    const rsdt = parseAcpiTable(rsdt_buf[0..]) ?? unreachable;
    expect_eq(rsdtEntryCount(rsdt), 2 as usize);
    expect_eq(rsdtEntryAt(rsdt, 0) ?? 0, 0x7FFE2000 as u32);
    expect_eq(rsdtEntryAt(rsdt, 1) ?? 0, 0x7FFE3000 as u32);
    expect_eq(rsdtEntryAt(rsdt, 2) == null, true);
    expect_eq(xsdtEntryCount(rsdt), 0 as usize);

    // XSDT with 2 entries: 36 + 16 = 52B
    let xsdt_buf: [52]u8 = std.mem.zeroes([52]u8);
    initSdtHeader(xsdt_buf[0..], SIG_XSDT, 52, 1);
    putU64Le(xsdt_buf[0..], 36, 0x00000002_00001000);
    putU64Le(xsdt_buf[0..], 44, 0x00000002_00002000);
    fixChecksumAt(xsdt_buf[0..], 9);

    const xsdt = parseAcpiTable(xsdt_buf[0..]) ?? unreachable;
    expect_eq(xsdtEntryCount(xsdt), 2 as usize);
    expect_eq(xsdtEntryAt(xsdt, 0) ?? 0, 0x00000002_00001000 as u64);
    expect_eq(xsdtEntryAt(xsdt, 1) ?? 0, 0x00000002_00002000 as u64);
    expect_eq(xsdtEntryAt(xsdt, 2) == null, true);
});

test("3. MADT (APIC) Local APIC, I/O APIC & Interrupt Source Override iteration", () => {
    // 36B SDT + 8B MADT hdr + 8B LocalAPIC + 12B IoApic + 10B IntOverride = 74B
    let buf: [74]u8 = std.mem.zeroes([74]u8);
    initSdtHeader(buf[0..], SIG_APIC, 74, 4);
    putU32Le(buf[0..], 36, 0xFEE00000); // local_apic_addr
    putU32Le(buf[0..], 40, 0x00000001); // PCAT_COMPAT (dual 8259)

    // Entry 0 [44..52]: Local APIC (type=0, len=8, uid=0, apic_id=1, flags=1 enabled)
    buf[44] = MADT_LOCAL_APIC;
    buf[45] = 8;
    buf[46] = 0;
    buf[47] = 1;
    putU32Le(buf[0..], 48, 0x00000001);

    // Entry 1 [52..64]: I/O APIC (type=1, len=12, id=2, addr=0xFEC00000, gsi_base=0)
    buf[52] = MADT_IO_APIC;
    buf[53] = 12;
    buf[54] = 2;
    buf[55] = 0;
    putU32Le(buf[0..], 56, 0xFEC00000);
    putU32Le(buf[0..], 60, 0);

    // Entry 2 [64..74]: Int Source Override (type=2, len=10, bus=0, irq=0, gsi=2, flags=0x0005)
    buf[64] = MADT_INT_SRC_OVERRIDE;
    buf[65] = 10;
    buf[66] = 0;
    buf[67] = 0;
    putU32Le(buf[0..], 68, 2);
    putU16Le(buf[0..], 72, 0x0005);

    fixChecksumAt(buf[0..], 9);

    const tbl = parseAcpiTable(buf[0..]) ?? unreachable;
    const madt = parseMadt(tbl) ?? unreachable;
    expect_eq(madt.local_apic_addr, 0xFEE00000 as u32);
    expect_eq(madt.hasDual8259(), true);

    let it = madt.entries();
    const e0 = it.next() ?? unreachable;
    const lapic = parseMadtLocalApic(e0) ?? unreachable;
    expect_eq(lapic.processor_uid, 0 as u8);
    expect_eq(lapic.apic_id, 1 as u8);
    expect_eq(lapic.isEnabled(), true);
    expect_eq(lapic.isOnlineCapable(), false);

    const e1 = it.next() ?? unreachable;
    const ioapic = parseMadtIoApic(e1) ?? unreachable;
    expect_eq(ioapic.io_apic_id, 2 as u8);
    expect_eq(ioapic.io_apic_addr, 0xFEC00000 as u32);
    expect_eq(ioapic.gsi_base, 0 as u32);

    const e2 = it.next() ?? unreachable;
    const ovr = parseMadtIntOverride(e2) ?? unreachable;
    expect_eq(ovr.bus, 0 as u8);
    expect_eq(ovr.irq_source, 0 as u8);
    expect_eq(ovr.gsi, 2 as u32);
    expect_eq(ovr.flags, 0x0005 as u16);

    expect_eq(it.next() == null, true);
    expect_eq(it.isMalformed(), false);
});

test("4. MCFG (PCIe ECAM) 16-byte entry iteration", () => {
    // 36B SDT + 8B reserved + 2 * 16B entries = 76B
    let buf: [76]u8 = std.mem.zeroes([76]u8);
    initSdtHeader(buf[0..], SIG_MCFG, 76, 1);

    // Entry 0 [44..60]: base=0xE0000000, seg=0, bus 0..255
    putU64Le(buf[0..], 44, 0x00000000_E0000000);
    putU16Le(buf[0..], 52, 0);
    buf[54] = 0;
    buf[55] = 255;

    // Entry 1 [60..76]: base=0xF0000000, seg=1, bus 0..63
    putU64Le(buf[0..], 60, 0x00000000_F0000000);
    putU16Le(buf[0..], 68, 1);
    buf[70] = 0;
    buf[71] = 63;

    fixChecksumAt(buf[0..], 9);

    const tbl = parseAcpiTable(buf[0..]) ?? unreachable;
    let it = parseMcfgEntries(tbl) ?? unreachable;

    const m0 = it.next() ?? unreachable;
    expect_eq(m0.base_addr, 0xE0000000 as u64);
    expect_eq(m0.segment_group, 0 as u16);
    expect_eq(m0.start_bus, 0 as u8);
    expect_eq(m0.end_bus, 255 as u8);

    const m1 = it.next() ?? unreachable;
    expect_eq(m1.base_addr, 0xF0000000 as u64);
    expect_eq(m1.segment_group, 1 as u16);
    expect_eq(m1.start_bus, 0 as u8);
    expect_eq(m1.end_bus, 63 as u8);

    expect_eq(it.next() == null, true);
});

test("5. Bad checksum, wrong RSDP signature, length < 36, and MADT entry length < 2 rejection", () => {
    // Bad RSDP signature
    let bad_rsdp: [20]u8 = std.mem.zeroes([20]u8);
    copyBytes(bad_rsdp[0..], 0, "BAD PTR ");
    fixChecksumAt(bad_rsdp[0..], 8);
    expect_eq(parseAcpiRsdp(bad_rsdp[0..]) == null, true);

    // Valid signature but corrupted checksum
    copyBytes(bad_rsdp[0..], 0, RSDP_SIGNATURE);
    fixChecksumAt(bad_rsdp[0..], 8);
    bad_rsdp[8] = bad_rsdp[8] ^ 0xFF;
    expect_eq(parseAcpiRsdp(bad_rsdp[0..]) == null, true);

    // SDT length < 36
    let bad_sdt: [40]u8 = std.mem.zeroes([40]u8);
    initSdtHeader(bad_sdt[0..], SIG_RSDT, 20, 1);
    fixChecksumAt(bad_sdt[0..], 9);
    expect_eq(parseAcpiTable(bad_sdt[0..]) == null, true);

    // SDT length > buffer.len
    initSdtHeader(bad_sdt[0..], SIG_RSDT, 100, 1);
    expect_eq(parseAcpiTable(bad_sdt[0..]) == null, true);

    // MADT entry with length < 2 must stop safely & flag malformed
    const bad_madt_entries: [4]u8 = [MADT_LOCAL_APIC, 0, 0, 0];
    let mit = MadtEntryIter.init(bad_madt_entries[0..]);
    expect_eq(mit.next() == null, true);
    expect_eq(mit.isMalformed(), true);
});
