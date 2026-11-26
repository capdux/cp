import { hashBytes } from "../../crc32fast/src/index.cp";
import {
    readU16Le, readU32Le, writeU16Le, writeU32Le
} from "../../bytemuck-crate/src/index.cp";

pub const ZIP_LOCAL_SIG: u32 = 67324752;
pub const ZIP_CD_SIG: u32 = 33639248;
pub const ZIP_EOCD_SIG: u32 = 101010256;
pub const ZIP_METHOD_STORED: u16 = 0;
pub const ZIP_METHOD_DEFLATE: u16 = 8;

pub struct ZipLocalHeader {
    pub version_needed: u16,
    pub flags: u16,
    pub method: u16,
    pub crc32: u32,
    pub compressed_size: u32,
    pub uncompressed_size: u32,
    pub name_len: u16,
    pub extra_len: u16,
    pub data_offset: usize,
    pub valid: bool,
}

pub struct ZipEocd {
    pub disk_num: u16,
    pub total_entries: u16,
    pub cd_size: u32,
    pub cd_offset: u32,
    pub valid: bool,
}

fn zipEmptyLocalHeader(): ZipLocalHeader {
    return ZipLocalHeader {
        version_needed: 0,
        flags: 0,
        method: 0,
        crc32: 0,
        compressed_size: 0,
        uncompressed_size: 0,
        name_len: 0,
        extra_len: 0,
        data_offset: 0,
        valid: false,
    };
}

fn zipEmptyEocd(): ZipEocd {
    return ZipEocd {
        disk_num: 0,
        total_entries: 0,
        cd_size: 0,
        cd_offset: 0,
        valid: false,
    };
}

fn zipCopyStr(dst: []u8, offset: usize, s: string) {
    let i: usize = 0;
    while (i < s.len) {
        dst[offset + i] = s[i];
        i = i + 1;
    }
}

fn zipCopyBytes(dst: []u8, offset: usize, src: []u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[offset + i] = src[i];
        i = i + 1;
    }
}

pub fn writeStoredZipSingle(filename: string, payload: []u8, out: []u8): usize? {
    if (filename.len == 0 || filename.len > 65535) {
        return null;
    }
    let max_u32: usize = 4294967295;
    if (payload.len > max_u32) {
        return null;
    }
    let local_hdr_len: usize = 30 + filename.len;
    let cd_offset: usize = local_hdr_len + payload.len;
    let cd_size: usize = 46 + filename.len;
    let total_len: usize = cd_offset + cd_size + 22;
    if (out.len < total_len) {
        return null;
    }

    let crc: u32 = hashBytes(payload);
    let sz32: u32 = payload.len as u32;
    let name_len16: u16 = filename.len as u16;

    // 1. Local File Header (30 bytes + filename)
    * = writeU32Le(ZIP_LOCAL_SIG, out[0..4]);
    * = writeU16Le(20, out[4..6]);
    * = writeU16Le(0, out[6..8]);
    * = writeU16Le(ZIP_METHOD_STORED, out[8..10]);
    * = writeU16Le(0, out[10..12]);
    * = writeU16Le(0, out[12..14]);
    * = writeU32Le(crc, out[14..18]);
    * = writeU32Le(sz32, out[18..22]);
    * = writeU32Le(sz32, out[22..26]);
    * = writeU16Le(name_len16, out[26..28]);
    * = writeU16Le(0, out[28..30]);
    zipCopyStr(out, 30, filename);

    // 2. Payload
    zipCopyBytes(out, local_hdr_len, payload);

    // 3. Central Directory Header (46 bytes + filename)
    let c: usize = cd_offset;
    * = writeU32Le(ZIP_CD_SIG, out[c..(c + 4)]);
    * = writeU16Le(20, out[(c + 4)..(c + 6)]);
    * = writeU16Le(20, out[(c + 6)..(c + 8)]);
    * = writeU16Le(0, out[(c + 8)..(c + 10)]);
    * = writeU16Le(ZIP_METHOD_STORED, out[(c + 10)..(c + 12)]);
    * = writeU16Le(0, out[(c + 12)..(c + 14)]);
    * = writeU16Le(0, out[(c + 14)..(c + 16)]);
    * = writeU32Le(crc, out[(c + 16)..(c + 20)]);
    * = writeU32Le(sz32, out[(c + 20)..(c + 24)]);
    * = writeU32Le(sz32, out[(c + 24)..(c + 28)]);
    * = writeU16Le(name_len16, out[(c + 28)..(c + 30)]);
    * = writeU16Le(0, out[(c + 30)..(c + 32)]);
    * = writeU16Le(0, out[(c + 32)..(c + 34)]);
    * = writeU16Le(0, out[(c + 34)..(c + 36)]);
    * = writeU16Le(0, out[(c + 36)..(c + 38)]);
    * = writeU32Le(0, out[(c + 38)..(c + 42)]);
    * = writeU32Le(0, out[(c + 42)..(c + 46)]);
    zipCopyStr(out, c + 46, filename);

    // 4. End of Central Directory (22 bytes)
    let e: usize = cd_offset + cd_size;
    * = writeU32Le(ZIP_EOCD_SIG, out[e..(e + 4)]);
    * = writeU16Le(0, out[(e + 4)..(e + 6)]);
    * = writeU16Le(0, out[(e + 6)..(e + 8)]);
    * = writeU16Le(1, out[(e + 8)..(e + 10)]);
    * = writeU16Le(1, out[(e + 10)..(e + 12)]);
    * = writeU32Le(cd_size as u32, out[(e + 12)..(e + 16)]);
    * = writeU32Le(cd_offset as u32, out[(e + 16)..(e + 20)]);
    * = writeU16Le(0, out[(e + 20)..(e + 22)]);

    return total_len;
}

pub fn parseLocalHeader(buf: []u8, offset: usize): ZipLocalHeader {
    if (offset + 30 > buf.len) {
        return zipEmptyLocalHeader();
    }
    let sig: u32 = readU32Le(buf[offset..(offset + 4)]) ?? 0;
    if (sig != ZIP_LOCAL_SIG) {
        return zipEmptyLocalHeader();
    }
    let ver: u16 = readU16Le(buf[(offset + 4)..(offset + 6)]) ?? 0;
    let flg: u16 = readU16Le(buf[(offset + 6)..(offset + 8)]) ?? 0;
    let meth: u16 = readU16Le(buf[(offset + 8)..(offset + 10)]) ?? 0;
    let crc: u32 = readU32Le(buf[(offset + 14)..(offset + 18)]) ?? 0;
    let csize: u32 = readU32Le(buf[(offset + 18)..(offset + 22)]) ?? 0;
    let usize_val: u32 = readU32Le(buf[(offset + 22)..(offset + 26)]) ?? 0;
    let nlen: u16 = readU16Le(buf[(offset + 26)..(offset + 28)]) ?? 0;
    let elen: u16 = readU16Le(buf[(offset + 28)..(offset + 30)]) ?? 0;

    let data_off: usize = offset + 30 + (nlen as usize) + (elen as usize);
    if (data_off > buf.len) {
        return zipEmptyLocalHeader();
    }
    return ZipLocalHeader {
        version_needed: ver,
        flags: flg,
        method: meth,
        crc32: crc,
        compressed_size: csize,
        uncompressed_size: usize_val,
        name_len: nlen,
        extra_len: elen,
        data_offset: data_off,
        valid: true,
    };
}

pub fn findAndParseEocd(buf: []u8): ZipEocd {
    if (buf.len < 22) {
        return zipEmptyEocd();
    }
    let max_back: usize = 22 + 65535;
    let min_pos: usize = 0;
    if (buf.len > max_back) {
        min_pos = buf.len - max_back;
    }
    let pos: usize = buf.len - 22;
    while (true) {
        let sig: u32 = readU32Le(buf[pos..(pos + 4)]) ?? 0;
        if (sig == ZIP_EOCD_SIG) {
            let comment_len: u16 = readU16Le(buf[(pos + 20)..(pos + 22)]) ?? 0;
            if (pos + 22 + (comment_len as usize) == buf.len) {
                let dnum: u16 = readU16Le(buf[(pos + 4)..(pos + 6)]) ?? 0;
                let total: u16 = readU16Le(buf[(pos + 10)..(pos + 12)]) ?? 0;
                let csize: u32 = readU32Le(buf[(pos + 12)..(pos + 16)]) ?? 0;
                let coff: u32 = readU32Le(buf[(pos + 16)..(pos + 20)]) ?? 0;
                return ZipEocd {
                    disk_num: dnum,
                    total_entries: total,
                    cd_size: csize,
                    cd_offset: coff,
                    valid: true,
                };
            }
        }
        if (pos == min_pos) {
            break;
        }
        pos = pos - 1;
    }
    return zipEmptyEocd();
}

pub fn verifyStoredEntry(buf: []u8, hdr: ZipLocalHeader): bool {
    if (!hdr.valid) {
        return false;
    }
    if (hdr.method != ZIP_METHOD_STORED) {
        return false;
    }
    if (hdr.compressed_size != hdr.uncompressed_size) {
        return false;
    }
    let end: usize = hdr.data_offset + (hdr.compressed_size as usize);
    if (end > buf.len) {
        return false;
    }
    let computed_crc: u32 = hashBytes(buf[hdr.data_offset..end]);
    return computed_crc == hdr.crc32;
}
