// Source: Bun src/exe_format/macho.cp
// lib/elf/macho_pe.cp
// Mach-O 64-bit Header & PE/COFF Header Binary Inspectors
// Absorbed from Bun src/exe_format/macho.cp & src/exe_format/pe.cp

import { ExeInspector } from "./format.cp";

pub struct MachHeader64 {
    pub magic: u32 = 0xFEEDFACF;
    pub cputype: u32 = 0;
    pub cpusubtype: u32 = 0;
    pub filetype: u32 = 0;
    pub ncmds: u32 = 0;
    pub sizeofcmds: u32 = 0;
    pub flags: u32 = 0;
    pub reserved: u32 = 0;
}

pub struct MachoInspector {
    pub static fn magic64(): u32 { return 0xFEEDFACF; }
    pub static fn cpuTypeX86_64(): u32 { return 0x01000007; }
    pub static fn cpuTypeArm64(): u32 { return 0x0100000C; }
    pub static fn fileTypeExecute(): u32 { return 2; }
    pub static fn fileTypeDylib(): u32 { return 6; }

    /// Parses a 32-byte little-endian `mach_header_64` from `bytes`.
    pub static fn parseHeader64(bytes: []const u8): MachHeader64? {
        if (bytes.len < 32) {
            return null;
        }
        const magic = ExeInspector.readU32Le(bytes, 0);
        if (magic != 0xFEEDFACF) {
            return null;
        }
        return MachHeader64 {
            magic: magic,
            cputype: ExeInspector.readU32Le(bytes, 4),
            cpusubtype: ExeInspector.readU32Le(bytes, 8),
            filetype: ExeInspector.readU32Le(bytes, 12),
            ncmds: ExeInspector.readU32Le(bytes, 16),
            sizeofcmds: ExeInspector.readU32Le(bytes, 20),
            flags: ExeInspector.readU32Le(bytes, 24),
            reserved: ExeInspector.readU32Le(bytes, 28),
        };
    }

    pub static fn writeU32Le(out: []u8, offset: usize, val: u32) {
        out[offset] = (val & 0xFF) as u8;
        out[offset + 1] = ((val >> 8) & 0xFF) as u8;
        out[offset + 2] = ((val >> 16) & 0xFF) as u8;
        out[offset + 3] = ((val >> 24) & 0xFF) as u8;
    }

    /// Encodes a 32-byte little-endian `mach_header_64` into `out`.
    pub static fn encodeHeader64(out: []u8, hdr: MachHeader64): usize {
        if (out.len < 32) {
            return 0;
        }
        MachoInspector.writeU32Le(out, 0, hdr.magic);
        MachoInspector.writeU32Le(out, 4, hdr.cputype);
        MachoInspector.writeU32Le(out, 8, hdr.cpusubtype);
        MachoInspector.writeU32Le(out, 12, hdr.filetype);
        MachoInspector.writeU32Le(out, 16, hdr.ncmds);
        MachoInspector.writeU32Le(out, 20, hdr.sizeofcmds);
        MachoInspector.writeU32Le(out, 24, hdr.flags);
        MachoInspector.writeU32Le(out, 28, hdr.reserved);
        return 32;
    }
}

pub struct PeCoffHeader {
    pub e_lfanew: u32 = 0;
    pub machine: u16 = 0;
    pub num_sections: u16 = 0;
    pub time_date_stamp: u32 = 0;
    pub pointer_to_symbol_table: u32 = 0;
    pub number_of_symbols: u32 = 0;
    pub size_of_optional_header: u16 = 0;
    pub characteristics: u16 = 0;
}

pub struct PeInspector {
    pub static fn machineAmd64(): u16 { return 0x8664; }
    pub static fn machineArm64(): u16 { return 0xAA64; }

    /// Parses a PE/COFF header starting from the DOS `MZ` stub, following `e_lfanew` at offset 60 to the `PE\0\0` signature and 20-byte COFF header.
    pub static fn parseCoffHeader(bytes: []const u8): PeCoffHeader? {
        const pe_off_opt = ExeInspector.parsePeSignatureOffset(bytes);
        if (pe_off_opt == null) {
            return null;
        }
        const pe_off = pe_off_opt.?;
        const coff_off = (pe_off as usize) + 4;
        if (coff_off + 20 > bytes.len) {
            return null;
        }
        return PeCoffHeader {
            e_lfanew: pe_off,
            machine: ExeInspector.readU16Le(bytes, coff_off),
            num_sections: ExeInspector.readU16Le(bytes, coff_off + 2),
            time_date_stamp: ExeInspector.readU32Le(bytes, coff_off + 4),
            pointer_to_symbol_table: ExeInspector.readU32Le(bytes, coff_off + 8),
            number_of_symbols: ExeInspector.readU32Le(bytes, coff_off + 12),
            size_of_optional_header: ExeInspector.readU16Le(bytes, coff_off + 16),
            characteristics: ExeInspector.readU16Le(bytes, coff_off + 18),
        };
    }
}
