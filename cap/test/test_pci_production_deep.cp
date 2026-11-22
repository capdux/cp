// Source: cp/cap/test/test_pci_production_deep.cp (Production deep tests for cap:pci)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PCI_CONFIG_SIZE,
    PCIE_EXT_CONFIG_SIZE,
    PCI_HEADER_TYPE_ENDPOINT,
    PCI_HEADER_TYPE_BRIDGE,
    PCI_HEADER_MULTIFUNC_BIT,
    PCI_STATUS_CAP_LIST,
    CAP_PM,
    CAP_MSI,
    CAP_VENDOR,
    CAP_PCIE,
    CAP_MSIX,
    EXT_CAP_AER,
    EXT_CAP_ACS,
    EXT_CAP_SRIOV,
    EXT_CAP_RESIZABLE_BAR,
    CLASS_MASS_STORAGE,
    CLASS_NETWORK,
    CLASS_DISPLAY,
    CLASS_BRIDGE,
    CLASS_SERIAL_BUS,
    PciHeader,
    parsePciHeader,
    PciBar,
    decodePciBar,
    PciCap,
    PciCapIter,
    iterPciCaps,
    findPciCap,
    PcieExtCap,
    PcieExtCapIter,
    iterPcieExtCaps,
    findPcieExtCap,
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

test("1. Type 0 Endpoint and Type 1 Bridge PCI configuration header parsing", () => {
    expect_eq(PCI_CONFIG_SIZE, 256 as usize);
    expect_eq(PCIE_EXT_CONFIG_SIZE, 4096 as usize);

    // Type 0 Multi-function Network Endpoint (Intel 0x8086:0x1572)
    let ep: [256]u8 = std.mem.zeroes([256]u8);
    putU16Le(ep[0..], 0x00, 0x8086);
    putU16Le(ep[0..], 0x02, 0x1572);
    putU16Le(ep[0..], 0x04, 0x0006); // Mem + BusMaster
    putU16Le(ep[0..], 0x06, PCI_STATUS_CAP_LIST);
    ep[0x08] = 0x01; // revision
    ep[0x0B] = CLASS_NETWORK;
    ep[0x0E] = PCI_HEADER_TYPE_ENDPOINT | PCI_HEADER_MULTIFUNC_BIT;
    putU16Le(ep[0..], 0x2C, 0x1028); // subsys_vendor
    putU16Le(ep[0..], 0x2E, 0x1F9C); // subsys_id
    ep[0x34] = 0x40;
    ep[0x3C] = 11;
    ep[0x3D] = 1;

    const h0 = parsePciHeader(ep[0..]) ?? unreachable;
    expect_eq(h0.isPresent(), true);
    expect_eq(h0.isEndpoint(), true);
    expect_eq(h0.isBridge(), false);
    expect_eq(h0.isMultiFunction(), true);
    expect_eq(h0.hasCapabilities(), true);
    expect_eq(h0.vendor_id, 0x8086 as u16);
    expect_eq(h0.device_id, 0x1572 as u16);
    expect_eq(h0.class_code, CLASS_NETWORK);
    expect_eq(h0.subsys_vendor_id, 0x1028 as u16);
    expect_eq(h0.subsys_id, 0x1F9C as u16);
    expect_eq(h0.cap_ptr, 0x40 as u8);
    expect_eq(h0.irq_line, 11 as u8);
    expect_eq(h0.irq_pin, 1 as u8);

    // Type 1 PCI-to-PCI Bridge
    let br: [64]u8 = std.mem.zeroes([64]u8);
    putU16Le(br[0..], 0x00, 0x1022);
    putU16Le(br[0..], 0x02, 0x1483);
    br[0x0B] = CLASS_BRIDGE;
    br[0x0E] = PCI_HEADER_TYPE_BRIDGE;

    const h1 = parsePciHeader(br[0..]) ?? unreachable;
    expect_eq(h1.isBridge(), true);
    expect_eq(h1.isEndpoint(), false);
    expect_eq(h1.isMultiFunction(), false);
    expect_eq(h1.subsys_vendor_id, 0 as u16);
});

test("2. BAR decoding: 32-bit Mem, 64-bit Prefetchable Mem, and I/O Space BAR", () => {
    let cfg: [256]u8 = std.mem.zeroes([256]u8);
    putU16Le(cfg[0..], 0x00, 0x10DE);
    putU16Le(cfg[0..], 0x02, 0x2684);
    cfg[0x0E] = PCI_HEADER_TYPE_ENDPOINT;

    // BAR0: 32-bit non-prefetchable Memory at 0xF6000000
    putU32Le(cfg[0..], 0x10, 0xF6000000);
    // BAR1+BAR2: 64-bit prefetchable Memory at 0x00000038_00000000 (type=0x02 -> bit2, prefetch -> bit3 => low nibble 0xC)
    putU32Le(cfg[0..], 0x14, 0x0000000C);
    putU32Le(cfg[0..], 0x18, 0x00000038);
    // BAR3: I/O Space at 0xE000 (bit0=1)
    putU32Le(cfg[0..], 0x1C, 0x0000E001);

    const b0 = decodePciBar(cfg[0..], 0) ?? unreachable;
    expect_eq(b0.is_io, false);
    expect_eq(b0.is_64bit, false);
    expect_eq(b0.prefetchable, false);
    expect_eq(b0.base_addr, 0xF6000000 as u64);

    const b1 = decodePciBar(cfg[0..], 1) ?? unreachable;
    expect_eq(b1.is_io, false);
    expect_eq(b1.is_64bit, true);
    expect_eq(b1.prefetchable, true);
    expect_eq(b1.base_addr, 0x00000038_00000000 as u64);

    const b3 = decodePciBar(cfg[0..], 3) ?? unreachable;
    expect_eq(b3.is_io, true);
    expect_eq(b3.is_64bit, false);
    expect_eq(b3.base_addr, 0xE000 as u64);

    // Out-of-range BAR index (6) rejected; 64-bit BAR in slot 5 (no slot 6) rejected
    expect_eq(decodePciBar(cfg[0..], 6) == null, true);
    putU32Le(cfg[0..], 0x24, 0x00000004); // 64-bit flag at BAR5
    expect_eq(decodePciBar(cfg[0..], 5) == null, true);
});

test("3. Standard PCI Capability linked list traversal (PM -> MSI -> PCIe -> MSI-X)", () => {
    let cfg: [256]u8 = std.mem.zeroes([256]u8);
    putU16Le(cfg[0..], 0x00, 0x144D);
    putU16Le(cfg[0..], 0x02, 0xA808);
    putU16Le(cfg[0..], 0x06, PCI_STATUS_CAP_LIST);
    cfg[0x34] = 0x40;

    // 0x40: PM -> 0x50
    cfg[0x40] = CAP_PM;
    cfg[0x41] = 0x50;
    // 0x50: MSI -> 0x70
    cfg[0x50] = CAP_MSI;
    cfg[0x51] = 0x70;
    // 0x70: PCIe -> 0xB0
    cfg[0x70] = CAP_PCIE;
    cfg[0x71] = 0xB0;
    // 0xB0: MSI-X -> 0x00 (end)
    cfg[0xB0] = CAP_MSIX;
    cfg[0xB1] = 0x00;

    let it = iterPciCaps(cfg[0..]);
    const c1 = it.next() ?? unreachable;
    expect_eq(c1.cap_id, CAP_PM);
    expect_eq(c1.offset, 0x40 as u8);

    const c2 = it.next() ?? unreachable;
    expect_eq(c2.cap_id, CAP_MSI);
    expect_eq(c2.offset, 0x50 as u8);

    const c3 = it.next() ?? unreachable;
    expect_eq(c3.cap_id, CAP_PCIE);
    expect_eq(c3.offset, 0x70 as u8);

    const c4 = it.next() ?? unreachable;
    expect_eq(c4.cap_id, CAP_MSIX);
    expect_eq(c4.offset, 0xB0 as u8);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);

    // Direct lookup via findPciCap
    const found = findPciCap(cfg[0..], CAP_MSIX) ?? unreachable;
    expect_eq(found.offset, 0xB0 as u8);
    expect_eq(findPciCap(cfg[0..], CAP_VENDOR) == null, true);
});

test("4. 4KB PCIe Extended Capability linked list traversal (AER -> SR-IOV -> Resizable BAR)", () => {
    let cfg: [4096]u8 = std.mem.zeroes([4096]u8);
    putU16Le(cfg[0..], 0x00, 0x8086);
    putU16Le(cfg[0..], 0x02, 0x1592);

    // 0x100: AER (id=0x0001, ver=2, next=0x160)
    putU32Le(cfg[0..], 0x100, (0x160 << 20) | (2 << 16) | (EXT_CAP_AER as u32));
    // 0x160: SR-IOV (id=0x0010, ver=1, next=0x200)
    putU32Le(cfg[0..], 0x160, (0x200 << 20) | (1 << 16) | (EXT_CAP_SRIOV as u32));
    // 0x200: Resizable BAR (id=0x0015, ver=1, next=0)
    putU32Le(cfg[0..], 0x200, (0 << 20) | (1 << 16) | (EXT_CAP_RESIZABLE_BAR as u32));

    let it = iterPcieExtCaps(cfg[0..]);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.cap_id, EXT_CAP_AER);
    expect_eq(e1.version, 2 as u8);
    expect_eq(e1.offset, 0x100 as u16);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.cap_id, EXT_CAP_SRIOV);
    expect_eq(e2.version, 1 as u8);
    expect_eq(e2.offset, 0x160 as u16);

    const e3 = it.next() ?? unreachable;
    expect_eq(e3.cap_id, EXT_CAP_RESIZABLE_BAR);
    expect_eq(e3.offset, 0x200 as u16);
    expect_eq(e3.next_offset, 0 as u16);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);

    const sriov = findPcieExtCap(cfg[0..], EXT_CAP_SRIOV) ?? unreachable;
    expect_eq(sriov.offset, 0x160 as u16);
    expect_eq(findPcieExtCap(cfg[0..], EXT_CAP_ACS) == null, true);
});

test("5. Absent device (0xFFFF), truncated config, and cyclic capability list loop guards", () => {
    // 0xFFFF vendor ID (empty PCI slot) rejected
    let empty_slot: [64]u8 = std.mem.zeroes([64]u8);
    putU16Le(empty_slot[0..], 0x00, 0xFFFF);
    expect_eq(parsePciHeader(empty_slot[0..]) == null, true);

    // Truncated config (< 64 bytes) rejected
    let short_cfg: [32]u8 = std.mem.zeroes([32]u8);
    putU16Le(short_cfg[0..], 0x00, 0x8086);
    expect_eq(parsePciHeader(short_cfg[0..]) == null, true);

    // Cyclic Standard Capability list: 0x40 -> 0x50 -> 0x40
    let cyc: [256]u8 = std.mem.zeroes([256]u8);
    putU16Le(cyc[0..], 0x00, 0x8086);
    putU16Le(cyc[0..], 0x02, 0x1234);
    putU16Le(cyc[0..], 0x06, PCI_STATUS_CAP_LIST);
    cyc[0x34] = 0x40;
    cyc[0x40] = CAP_PM;
    cyc[0x41] = 0x50;
    cyc[0x50] = CAP_MSI;
    cyc[0x51] = 0x40;

    let it = iterPciCaps(cyc[0..]);
    let count: usize = 0;
    while (true) {
        if (c := it.next()) {
            * = c;
            count += 1;
        } else {
            break;
        }
    }
    expect_eq(count, 48 as usize);
    expect_eq(it.hasError(), true);

    // Cyclic PCIe Extended Capability list: 0x100 -> 0x120 -> 0x100
    let ext_cyc: [512]u8 = std.mem.zeroes([512]u8);
    putU16Le(ext_cyc[0..], 0x00, 0x8086);
    putU16Le(ext_cyc[0..], 0x02, 0x5678);
    putU32Le(ext_cyc[0..], 0x100, (0x120 << 20) | (1 << 16) | (EXT_CAP_AER as u32));
    putU32Le(ext_cyc[0..], 0x120, (0x100 << 20) | (1 << 16) | (EXT_CAP_ACS as u32));

    let eit = iterPcieExtCaps(ext_cyc[0..]);
    let ecount: usize = 0;
    while (true) {
        if (ec := eit.next()) {
            * = ec;
            ecount += 1;
        } else {
            break;
        }
    }
    expect_eq(ecount, 96 as usize);
    expect_eq(eit.hasError(), true);
});
