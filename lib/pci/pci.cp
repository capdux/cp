// Origin: Pure-CP Stdlib (Zero-Alloc PCI / PCIe Configuration Space, BAR & Capability Parser) | Created: 2026-11-22

pub const PCI_CONFIG_SIZE: usize = 256;
pub const PCIE_EXT_CONFIG_SIZE: usize = 4096;
pub const PCI_HEADER_TYPE_ENDPOINT: u8 = 0;
pub const PCI_HEADER_TYPE_BRIDGE: u8 = 1;
pub const PCI_HEADER_MULTIFUNC_BIT: u8 = 0x80;
pub const PCI_STATUS_CAP_LIST: u16 = 0x0010;

// Standard PCI Capability IDs
pub const CAP_PM: u8 = 0x01;
pub const CAP_MSI: u8 = 0x05;
pub const CAP_VENDOR: u8 = 0x09;
pub const CAP_PCIE: u8 = 0x10;
pub const CAP_MSIX: u8 = 0x11;

// PCIe Extended Capability IDs
pub const EXT_CAP_AER: u16 = 0x0001;
pub const EXT_CAP_ACS: u16 = 0x000D;
pub const EXT_CAP_SRIOV: u16 = 0x0010;
pub const EXT_CAP_RESIZABLE_BAR: u16 = 0x0015;

// Common PCI Base Class Codes
pub const CLASS_MASS_STORAGE: u8 = 0x01;
pub const CLASS_NETWORK: u8 = 0x02;
pub const CLASS_DISPLAY: u8 = 0x03;
pub const CLASS_BRIDGE: u8 = 0x06;
pub const CLASS_SERIAL_BUS: u8 = 0x0C;

fn readU16Le(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return b0 | (b1 << 8);
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

/// Parsed 64-byte PCI / PCIe standard configuration space header.
pub struct PciHeader {
    vendor_id: u16,
    device_id: u16,
    command: u16,
    status: u16,
    revision_id: u8,
    prog_if: u8,
    subclass: u8,
    class_code: u8,
    cache_line_size: u8,
    latency_timer: u8,
    header_type_raw: u8,
    bist: u8,
    subsys_vendor_id: u16,
    subsys_id: u16,
    cap_ptr: u8,
    irq_line: u8,
    irq_pin: u8,

    fn headerType(): u8 {
        return _.header_type_raw & 0x7F;
    }

    fn isMultiFunction(): bool {
        return (_.header_type_raw & PCI_HEADER_MULTIFUNC_BIT) != 0;
    }

    fn hasCapabilities(): bool {
        return (_.status & PCI_STATUS_CAP_LIST) != 0;
    }

    fn isEndpoint(): bool {
        return _.headerType() == PCI_HEADER_TYPE_ENDPOINT;
    }

    fn isBridge(): bool {
        return _.headerType() == PCI_HEADER_TYPE_BRIDGE;
    }

    fn isPresent(): bool {
        return _.vendor_id != 0xFFFF && _.vendor_id != 0x0000;
    }
}

/// Strictly parses a PCI / PCIe configuration header (requires `cfg.len >= 64` and present vendor ID).
pub fn parsePciHeader(cfg: []const u8): PciHeader? {
    if (cfg.len < 64) {
        return null;
    }
    const vendor_id = readU16Le(cfg, 0x00);
    if (vendor_id == 0xFFFF || vendor_id == 0x0000) {
        return null;
    }
    const header_type_raw = cfg[0x0E];
    const htype = header_type_raw & 0x7F;
    const subsys_vendor_id: u16 = if (htype == PCI_HEADER_TYPE_ENDPOINT) readU16Le(cfg, 0x2C) else 0;
    const subsys_id: u16 = if (htype == PCI_HEADER_TYPE_ENDPOINT) readU16Le(cfg, 0x2E) else 0;

    return PciHeader{
        .vendor_id = vendor_id,
        .device_id = readU16Le(cfg, 0x02),
        .command = readU16Le(cfg, 0x04),
        .status = readU16Le(cfg, 0x06),
        .revision_id = cfg[0x08],
        .prog_if = cfg[0x09],
        .subclass = cfg[0x0A],
        .class_code = cfg[0x0B],
        .cache_line_size = cfg[0x0C],
        .latency_timer = cfg[0x0D],
        .header_type_raw = header_type_raw,
        .bist = cfg[0x0F],
        .subsys_vendor_id = subsys_vendor_id,
        .subsys_id = subsys_id,
        .cap_ptr = cfg[0x34] & 0xFC,
        .irq_line = cfg[0x3C],
        .irq_pin = cfg[0x3D],
    };
}

/// Decoded PCI Base Address Register (32-bit Mem, 64-bit Mem, or I/O Space).
pub struct PciBar {
    index: u8,
    is_io: bool,
    is_64bit: bool,
    prefetchable: bool,
    base_addr: u64,
}

/// Decodes BAR `bar_idx` (`0..5` for Endpoint, `0..1` for Bridge), merging adjacent high 32-bit word for 64-bit BARs.
pub fn decodePciBar(cfg: []const u8, bar_idx: u8): PciBar? {
    const hdr = parsePciHeader(cfg) ?? return null;
    const max_bars: u8 = if (hdr.isEndpoint()) 6 else if (hdr.isBridge()) 2 else return null;
    if (bar_idx >= max_bars) {
        return null;
    }

    const off: usize = (0x10 as usize) + (bar_idx as usize) * 4;
    const raw_lo = readU32Le(cfg, off);

    if ((raw_lo & 0x01) != 0) {
        return PciBar{
            .index = bar_idx,
            .is_io = true,
            .is_64bit = false,
            .prefetchable = false,
            .base_addr = (raw_lo & 0xFFFFFFFC) as u64,
        };
    }

    const mem_type = (raw_lo >> 1) & 0x03;
    const prefetchable = (raw_lo & 0x08) != 0;
    const lo_addr = (raw_lo & 0xFFFFFFF0) as u64;

    if (mem_type == 0x02) {
        // 64-bit memory BAR occupies bar_idx and bar_idx + 1
        if (bar_idx + 1 >= max_bars) {
            return null;
        }
        const raw_hi = readU32Le(cfg, off + 4);
        const full_addr: u64 = ((raw_hi as u64) << 32) | lo_addr;
        return PciBar{
            .index = bar_idx,
            .is_io = false,
            .is_64bit = true,
            .prefetchable = prefetchable,
            .base_addr = full_addr,
        };
    }

    return PciBar{
        .index = bar_idx,
        .is_io = false,
        .is_64bit = false,
        .prefetchable = prefetchable,
        .base_addr = lo_addr,
    };
}

/// Entry in the 256-byte standard PCI Capability linked list.
pub struct PciCap {
    cap_id: u8,
    offset: u8,
    next_offset: u8,
}

/// Zero-copy iterator over the PCI Standard Capability list with <= 48 hop loop guard.
pub struct PciCapIter {
    cfg: []const u8,
    cur_offset: u8,
    steps: u8,
    failed: bool,

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): PciCap? {
        if (_.failed || _.cur_offset == 0) {
            return null;
        }
        if (_.steps >= 48) {
            _.failed = true;
            return null;
        }
        const off = _.cur_offset as usize;
        if (off < 0x40 || off + 2 > _.cfg.len || off + 2 > PCI_CONFIG_SIZE) {
            _.failed = true;
            return null;
        }
        const cap_id = _.cfg[off];
        const next_off = _.cfg[off + 1] & 0xFC;
        if (next_off == _.cur_offset) {
            _.failed = true;
            return null;
        }
        const res = PciCap{
            .cap_id = cap_id,
            .offset = _.cur_offset,
            .next_offset = next_off,
        };
        _.cur_offset = next_off;
        _.steps += 1;
        return res;
    }
}

/// Creates a Standard PCI Capability iterator starting from `cap_ptr & 0xFC`.
pub fn iterPciCaps(cfg: []const u8): PciCapIter {
    if (hdr := parsePciHeader(cfg)) {
        if (hdr.hasCapabilities() && hdr.cap_ptr >= 0x40) {
            return PciCapIter{
                .cfg = cfg,
                .cur_offset = hdr.cap_ptr,
                .steps = 0,
                .failed = false,
            };
        }
    }
    return PciCapIter{
        .cfg = cfg,
        .cur_offset = 0,
        .steps = 0,
        .failed = false,
    };
}

/// Searches the Standard PCI Capability list for the first entry matching `cap_id`.
pub fn findPciCap(cfg: []const u8, cap_id: u8): PciCap? {
    let it = iterPciCaps(cfg);
    while (true) {
        if (cap := it.next()) {
            if (cap.cap_id == cap_id) {
                return cap;
            }
        } else {
            break;
        }
    }
    return null;
}

/// Entry in the 4096-byte PCIe Extended Capability linked list.
pub struct PcieExtCap {
    cap_id: u16,
    version: u8,
    offset: u16,
    next_offset: u16,
}

/// Zero-copy iterator over the PCIe Extended Capability list starting at `0x100` with <= 96 hop loop guard.
pub struct PcieExtCapIter {
    cfg: []const u8,
    cur_offset: u16,
    steps: u8,
    failed: bool,

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): PcieExtCap? {
        if (_.failed || _.cur_offset == 0) {
            return null;
        }
        if (_.steps >= 96) {
            _.failed = true;
            return null;
        }
        const off = _.cur_offset as usize;
        if (off < 0x100 || off + 4 > _.cfg.len || off + 4 > PCIE_EXT_CONFIG_SIZE) {
            _.failed = true;
            return null;
        }
        const word = readU32Le(_.cfg, off);
        if (word == 0x00000000 || word == 0xFFFFFFFF) {
            _.cur_offset = 0;
            return null;
        }
        const cap_id = (word & 0xFFFF) as u16;
        const version = ((word >> 16) & 0x0F) as u8;
        const next_off = ((word >> 20) & 0x0FFC) as u16;
        if (next_off == _.cur_offset) {
            _.failed = true;
            return null;
        }
        const res = PcieExtCap{
            .cap_id = cap_id,
            .version = version,
            .offset = _.cur_offset,
            .next_offset = next_off,
        };
        _.cur_offset = next_off;
        _.steps += 1;
        return res;
    }
}

/// Creates a PCIe Extended Capability iterator starting at offset `0x100`.
pub fn iterPcieExtCaps(cfg: []const u8): PcieExtCapIter {
    if (cfg.len >= 0x104) {
        if (hdr := parsePciHeader(cfg)) {
            * = hdr;
            return PcieExtCapIter{
                .cfg = cfg,
                .cur_offset = 0x100,
                .steps = 0,
                .failed = false,
            };
        }
    }
    return PcieExtCapIter{
        .cfg = cfg,
        .cur_offset = 0,
        .steps = 0,
        .failed = false,
    };
}

/// Searches the PCIe Extended Capability list for the first entry matching `ext_cap_id`.
pub fn findPcieExtCap(cfg: []const u8, ext_cap_id: u16): PcieExtCap? {
    let it = iterPcieExtCaps(cfg);
    while (true) {
        if (cap := it.next()) {
            if (cap.cap_id == ext_cap_id) {
                return cap;
            }
        } else {
            break;
        }
    }
    return null;
}
