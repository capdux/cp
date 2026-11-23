// Source: bun/src/exe_format/pe.cp -> cp/lib/exe_format/pe.cp (Zero-copy PE-COFF header parser)
import std from "std";
import { ExeArch, archFromPeMachine, readU16Le, readU32Le, readU64Le } from "./exeTypes.cp";

pub const PE_SIGNATURE: u32 = 0x00004550;
pub const DOS_SIGNATURE: u16 = 0x5A4D;
pub const PE32_MAGIC: u16 = 0x010B;
pub const PE32_PLUS_MAGIC: u16 = 0x020B;
pub const PE_SECTION_HEADER_SIZE: usize = 40;

pub struct PeHeaderInfo {
    is_pe32_plus: bool,
    machine: u16,
    arch: ExeArch,
    num_sections: u16,
    size_of_optional_header: u16,
    characteristics: u16,
    pe_offset: usize,
    sections_offset: usize,
    entry_point_rva: u32,
    image_base: u64,
}

pub fn parsePeHeader(bytes: []const u8): PeHeaderInfo! {
    if (bytes.len < 64) {
        return err.TruncatedHeader;
    }
    const dos_magic = readU16Le(bytes, 0)?;
    if (dos_magic != DOS_SIGNATURE) {
        return err.InvalidMagic;
    }

    const pe_off_u32 = readU32Le(bytes, 60)?;
    const pe_off: usize = pe_off_u32 as usize;

    // Need 4 bytes PE signature + 20 bytes COFF header + at least 2 bytes OptionalHeader magic
    if (pe_off > bytes.len || bytes.len - pe_off < 26) {
        return err.OutOfBounds;
    }

    const pe_sig = readU32Le(bytes, pe_off)?;
    if (pe_sig != PE_SIGNATURE) {
        return err.InvalidMagic;
    }

    const coff_off = pe_off + 4;
    const machine = readU16Le(bytes, coff_off)?;
    const num_sections = readU16Le(bytes, coff_off + 2)?;
    const size_of_opt = readU16Le(bytes, coff_off + 16)?;
    const characteristics = readU16Le(bytes, coff_off + 18)?;

    const opt_off = coff_off + 20;
    if ((size_of_opt as usize) > bytes.len - opt_off) {
        return err.OutOfBounds;
    }

    const opt_magic = readU16Le(bytes, opt_off)?;
    let is_pe32_plus = false;
    let entry_point_rva: u32 = 0;
    let image_base: u64 = 0;

    if (opt_magic == PE32_PLUS_MAGIC) {
        is_pe32_plus = true;
        if (size_of_opt < 32 || opt_off + 32 > bytes.len) {
            return err.TruncatedHeader;
        }
        entry_point_rva = readU32Le(bytes, opt_off + 16)?;
        image_base = readU64Le(bytes, opt_off + 24)?;
    } else if (opt_magic == PE32_MAGIC) {
        is_pe32_plus = false;
        if (size_of_opt < 32 || opt_off + 32 > bytes.len) {
            return err.TruncatedHeader;
        }
        entry_point_rva = readU32Le(bytes, opt_off + 16)?;
        image_base = (readU32Le(bytes, opt_off + 28)?) as u64;
    } else {
        return err.UnsupportedClass;
    }

    const sections_off = opt_off + (size_of_opt as usize);
    const total_sec_bytes = (num_sections as usize) * PE_SECTION_HEADER_SIZE;
    if (sections_off > bytes.len || total_sec_bytes > bytes.len - sections_off) {
        return err.OutOfBounds;
    }

    return PeHeaderInfo{
        is_pe32_plus: is_pe32_plus,
        machine: machine,
        arch: archFromPeMachine(machine),
        num_sections: num_sections,
        size_of_optional_header: size_of_opt,
        characteristics: characteristics,
        pe_offset: pe_off,
        sections_offset: sections_off,
        entry_point_rva: entry_point_rva,
        image_base: image_base,
    };
}
