// Origin: Pure-CP Stdlib (Zero-Alloc ACPI 6.5 RSDP / SDT / MADT / MCFG Firmware Table Parser) | Created: 2026-11-22

// ACPI Structure Signatures & Sizes
pub const RSDP_SIGNATURE: []const u8 = "RSD PTR ";
pub const RSDP_V1_SIZE: usize = 20;
pub const RSDP_V2_SIZE: usize = 36;
pub const SDT_HEADER_SIZE: usize = 36;

// Common ACPI Table Signatures
pub const SIG_RSDT: []const u8 = "RSDT";
pub const SIG_XSDT: []const u8 = "XSDT";
pub const SIG_FACP: []const u8 = "FACP";
pub const SIG_APIC: []const u8 = "APIC";
pub const SIG_MCFG: []const u8 = "MCFG";
pub const SIG_HPET: []const u8 = "HPET";
pub const SIG_DSDT: []const u8 = "DSDT";

// MADT Interrupt Controller Structure (ICS) Entry Types (ACPI 6.5 §5.2.12)
pub const MADT_LOCAL_APIC: u8 = 0;
pub const MADT_IO_APIC: u8 = 1;
pub const MADT_INT_SRC_OVERRIDE: u8 = 2;
pub const MADT_NMI_SOURCE: u8 = 3;
pub const MADT_LOCAL_APIC_NMI: u8 = 4;
pub const MADT_LOCAL_APIC_ADDR_OVERRIDE: u8 = 5;
pub const MADT_LOCAL_X2APIC: u8 = 9;

fn readU16Le(data: []const u8, off: usize): u16 {
    const b0 = data[off] as u16;
    const b1 = (data[off + 1] as u16) << 8;
    return b0 | b1;
}

fn readU32Le(data: []const u8, off: usize): u32 {
    const b0 = data[off] as u32;
    const b1 = (data[off + 1] as u32) << 8;
    const b2 = (data[off + 2] as u32) << 16;
    const b3 = (data[off + 3] as u32) << 24;
    return b0 | b1 | b2 | b3;
}

fn readU64Le(data: []const u8, off: usize): u64 {
    const lo = readU32Le(data, off) as u64;
    const hi = (readU32Le(data, off + 4) as u64) << 32;
    return lo | hi;
}

fn sigEq4(sig: [4]u8, expected: []const u8): bool {
    return expected.len == 4
        && sig[0] == expected[0]
        && sig[1] == expected[1]
        && sig[2] == expected[2]
        && sig[3] == expected[3];
}

/// Computes the 8-bit ACPI modular checksum (byte sum mod 256). Valid ACPI tables sum to `0`.
pub fn acpiChecksum(data: []const u8): u8 {
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < data.len) {
        sum += data[i] as u32;
        i += 1;
    }
    return (sum & 0xFF) as u8;
}

/// Returns `true` if `data` is non-empty and its 8-bit modular checksum equals `0`.
pub fn verifyAcpiChecksum(data: []const u8): bool {
    return data.len > 0 && acpiChecksum(data) == 0;
}

/// ACPI 6.5 §5.2.5.3 Root System Description Pointer (RSDP v1 / v2).
pub struct AcpiRsdp {
    revision: u8,
    oem_id: [6]u8,
    rsdt_addr: u32,
    length: u32,
    xsdt_addr: u64,
    is_v2: bool,
}

/// Parses an ACPI RSDP structure, verifying the 20B v1 checksum and (for revision >= 2) the extended v2 checksum.
pub fn parseAcpiRsdp(data: []const u8): AcpiRsdp? {
    if (data.len < RSDP_V1_SIZE) {
        return null;
    }
    let i: usize = 0;
    while (i < 8) {
        if (data[i] != RSDP_SIGNATURE[i]) {
            return null;
        }
        i += 1;
    }
    if (!verifyAcpiChecksum(data[0..RSDP_V1_SIZE])) {
        return null;
    }

    let oem_id: [6]u8 = [0; 6];
    i = 0;
    while (i < 6) {
        oem_id[i] = data[9 + i];
        i += 1;
    }
    const revision = data[15];
    const rsdt_addr = readU32Le(data, 16);

    if (revision >= 2) {
        if (data.len < RSDP_V2_SIZE) {
            return null;
        }
        const length = readU32Le(data, 20);
        if (length < (RSDP_V2_SIZE as u32) || (length as usize) > data.len) {
            return null;
        }
        if (!verifyAcpiChecksum(data[0..(length as usize)])) {
            return null;
        }
        const xsdt_addr = readU64Le(data, 24);
        return AcpiRsdp{
            .revision = revision,
            .oem_id = oem_id,
            .rsdt_addr = rsdt_addr,
            .length = length,
            .xsdt_addr = xsdt_addr,
            .is_v2 = true,
        };
    }

    return AcpiRsdp{
        .revision = revision,
        .oem_id = oem_id,
        .rsdt_addr = rsdt_addr,
        .length = RSDP_V1_SIZE as u32,
        .xsdt_addr = 0,
        .is_v2 = false,
    };
}

/// ACPI 6.5 §5.2.6 36-byte System Description Table (SDT) header.
pub struct AcpiSdtHeader {
    signature: [4]u8,
    length: u32,
    revision: u8,
    checksum: u8,
    oem_id: [6]u8,
    oem_table_id: [8]u8,
    oem_revision: u32,
    creator_id: [4]u8,
    creator_revision: u32,
}

/// Parsed ACPI System Description Table with zero-copy body slice.
pub struct AcpiTable {
    header: AcpiSdtHeader,
    body: []const u8,
}

/// Parses an ACPI SDT table, verifying `length >= 36 && length <= data.len` and full-table checksum == 0.
pub fn parseAcpiTable(data: []const u8): AcpiTable? {
    if (data.len < SDT_HEADER_SIZE) {
        return null;
    }
    const length = readU32Le(data, 4);
    if (length < (SDT_HEADER_SIZE as u32) || (length as usize) > data.len) {
        return null;
    }
    if (!verifyAcpiChecksum(data[0..(length as usize)])) {
        return null;
    }

    const sig: [4]u8 = [data[0], data[1], data[2], data[3]];
    let oem_id: [6]u8 = [0; 6];
    let i: usize = 0;
    while (i < 6) {
        oem_id[i] = data[10 + i];
        i += 1;
    }
    let oem_table_id: [8]u8 = [0; 8];
    i = 0;
    while (i < 8) {
        oem_table_id[i] = data[16 + i];
        i += 1;
    }
    const creator_id: [4]u8 = [data[28], data[29], data[30], data[31]];

    return AcpiTable{
        .header = AcpiSdtHeader{
            .signature = sig,
            .length = length,
            .revision = data[8],
            .checksum = data[9],
            .oem_id = oem_id,
            .oem_table_id = oem_table_id,
            .oem_revision = readU32Le(data, 24),
            .creator_id = creator_id,
            .creator_revision = readU32Le(data, 32),
        },
        .body = data[SDT_HEADER_SIZE..(length as usize)],
    };
}

/// Returns the number of 32-bit SDT pointers in an `"RSDT"` table (0 if not `"RSDT"`).
pub fn rsdtEntryCount(tbl: AcpiTable): usize {
    if (!sigEq4(tbl.header.signature, SIG_RSDT)) {
        return 0;
    }
    return tbl.body.len / 4;
}

/// Returns the `idx`-th 32-bit physical table address from an `"RSDT"` table.
pub fn rsdtEntryAt(tbl: AcpiTable, idx: usize): u32? {
    if (!sigEq4(tbl.header.signature, SIG_RSDT)) {
        return null;
    }
    const off = idx * 4;
    if (off + 4 > tbl.body.len) {
        return null;
    }
    return readU32Le(tbl.body, off);
}

/// Returns the number of 64-bit SDT pointers in an `"XSDT"` table (0 if not `"XSDT"`).
pub fn xsdtEntryCount(tbl: AcpiTable): usize {
    if (!sigEq4(tbl.header.signature, SIG_XSDT)) {
        return 0;
    }
    return tbl.body.len / 8;
}

/// Returns the `idx`-th 64-bit physical table address from an `"XSDT"` table.
pub fn xsdtEntryAt(tbl: AcpiTable, idx: usize): u64? {
    if (!sigEq4(tbl.header.signature, SIG_XSDT)) {
        return null;
    }
    const off = idx * 8;
    if (off + 8 > tbl.body.len) {
        return null;
    }
    return readU64Le(tbl.body, off);
}

/// Raw MADT Interrupt Controller Structure (ICS) TLV entry (`data` is the payload after the 2B header).
pub struct MadtEntry {
    entry_type: u8,
    length: u8,
    data: []const u8,
}

/// Zero-copy iterator over MADT Interrupt Controller Structure entries.
pub struct MadtEntryIter {
    bytes: []const u8,
    pos: usize,
    malformed: bool,

    pub fn init(bytes: []const u8): MadtEntryIter {
        return MadtEntryIter{
            .bytes = bytes,
            .pos = 0,
            .malformed = false,
        };
    }

    fn next(*_): MadtEntry? {
        if (_.malformed || _.pos >= _.bytes.len) {
            return null;
        }
        if (_.bytes.len - _.pos < 2) {
            _.malformed = true;
            return null;
        }
        const entry_type = _.bytes[_.pos];
        const entry_len = _.bytes[_.pos + 1] as usize;
        if (entry_len < 2 || entry_len > _.bytes.len - _.pos) {
            _.malformed = true;
            return null;
        }
        const payload = _.bytes[(_.pos + 2)..(_.pos + entry_len)];
        _.pos += entry_len;
        return MadtEntry{
            .entry_type = entry_type,
            .length = entry_len as u8,
            .data = payload,
        };
    }

    fn isMalformed(): bool {
        return _.malformed;
    }
}

/// Parsed MADT (`"APIC"`) table header and raw ICS entries slice.
pub struct MadtTable {
    local_apic_addr: u32,
    flags: u32,
    entries_bytes: []const u8,

    fn hasDual8259(): bool {
        return (_.flags & 0x00000001) != 0;
    }

    fn entries(): MadtEntryIter {
        return MadtEntryIter.init(_.entries_bytes);
    }
}

/// Parses an `"APIC"` (MADT) table body.
pub fn parseMadt(tbl: AcpiTable): MadtTable? {
    if (!sigEq4(tbl.header.signature, SIG_APIC) || tbl.body.len < 8) {
        return null;
    }
    return MadtTable{
        .local_apic_addr = readU32Le(tbl.body, 0),
        .flags = readU32Le(tbl.body, 4),
        .entries_bytes = tbl.body[8..],
    };
}

/// MADT Type 0: Processor Local APIC structure.
pub struct MadtLocalApic {
    processor_uid: u8,
    apic_id: u8,
    flags: u32,

    fn isEnabled(): bool {
        return (_.flags & 0x00000001) != 0;
    }

    fn isOnlineCapable(): bool {
        return (_.flags & 0x00000002) != 0;
    }
}

/// Parses a `MADT_LOCAL_APIC (0)` entry.
pub fn parseMadtLocalApic(entry: MadtEntry): MadtLocalApic? {
    if (entry.entry_type != MADT_LOCAL_APIC || entry.length != 8 || entry.data.len < 6) {
        return null;
    }
    return MadtLocalApic{
        .processor_uid = entry.data[0],
        .apic_id = entry.data[1],
        .flags = readU32Le(entry.data, 2),
    };
}

/// MADT Type 1: I/O APIC structure.
pub struct MadtIoApic {
    io_apic_id: u8,
    io_apic_addr: u32,
    gsi_base: u32,
}

/// Parses a `MADT_IO_APIC (1)` entry.
pub fn parseMadtIoApic(entry: MadtEntry): MadtIoApic? {
    if (entry.entry_type != MADT_IO_APIC || entry.length != 12 || entry.data.len < 10) {
        return null;
    }
    return MadtIoApic{
        .io_apic_id = entry.data[0],
        .io_apic_addr = readU32Le(entry.data, 2),
        .gsi_base = readU32Le(entry.data, 6),
    };
}

/// MADT Type 2: Interrupt Source Override structure.
pub struct MadtIntOverride {
    bus: u8,
    irq_source: u8,
    gsi: u32,
    flags: u16,
}

/// Parses a `MADT_INT_SRC_OVERRIDE (2)` entry.
pub fn parseMadtIntOverride(entry: MadtEntry): MadtIntOverride? {
    if (entry.entry_type != MADT_INT_SRC_OVERRIDE || entry.length != 10 || entry.data.len < 8) {
        return null;
    }
    return MadtIntOverride{
        .bus = entry.data[0],
        .irq_source = entry.data[1],
        .gsi = readU32Le(entry.data, 2),
        .flags = readU16Le(entry.data, 6),
    };
}

/// PCI Firmware Spec MCFG 16-byte ECAM Configuration Space allocation entry.
pub struct McfgEntry {
    base_addr: u64,
    segment_group: u16,
    start_bus: u8,
    end_bus: u8,
}

/// Zero-copy iterator over 16-byte MCFG ECAM entries.
pub struct McfgEntryIter {
    bytes: []const u8,
    pos: usize,

    pub fn init(bytes: []const u8): McfgEntryIter {
        return McfgEntryIter{
            .bytes = bytes,
            .pos = 0,
        };
    }

    fn next(*_): McfgEntry? {
        if (_.pos + 16 > _.bytes.len) {
            return null;
        }
        const entry = McfgEntry{
            .base_addr = readU64Le(_.bytes, _.pos),
            .segment_group = readU16Le(_.bytes, _.pos + 8),
            .start_bus = _.bytes[_.pos + 10],
            .end_bus = _.bytes[_.pos + 11],
        };
        _.pos += 16;
        return entry;
    }
}

/// Parses an `"MCFG"` table and returns a zero-copy iterator over its 16-byte ECAM entries.
pub fn parseMcfgEntries(tbl: AcpiTable): McfgEntryIter? {
    if (!sigEq4(tbl.header.signature, SIG_MCFG) || tbl.body.len < 8) {
        return null;
    }
    const entries_len = tbl.body.len - 8;
    if ((entries_len % 16) != 0) {
        return null;
    }
    return McfgEntryIter.init(tbl.body[8..]);
}
