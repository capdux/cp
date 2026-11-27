import { hashBytes } from "../../crc32fast/src/index.cp";
import { adler32 } from "../../adler2/src/index.cp";
import {
    readU16Le, readU32Le, readU32Be,
    writeU16Le, writeU32Le, writeU32Be
} from "../../bytemuck/src/index.cp";

pub const GZIP_ID1: u8 = 31;
pub const GZIP_ID2: u8 = 139;
pub const GZIP_CM_DEFLATE: u8 = 8;
pub const ZLIB_CMF_DEFAULT: u8 = 120;
pub const ZLIB_FLG_NO_COMP: u8 = 1;

pub struct GzipHeader {
    pub method: u8,
    pub flags: u8,
    pub mtime: u32,
    pub os: u8,
    pub data_offset: usize,
    pub valid: bool,
}

fn fl2EmptyGzipHeader(): GzipHeader {
    return GzipHeader {
        method: 0,
        flags: 0,
        mtime: 0,
        os: 0,
        data_offset: 0,
        valid: false,
    };
}

fn fl2StoredEncodedLen(payload_len: usize): usize {
    if (payload_len == 0) {
        return 5;
    }
    let blocks: usize = (payload_len + 65534) / 65535;
    return payload_len + (blocks * 5);
}

fn fl2EncodeStoredBlocks(payload: []u8, out: []u8, start: usize): usize {
    let pos: usize = start;
    if (payload.len == 0) {
        out[pos] = 1;
        * = writeU16Le(0, out[(pos + 1)..(pos + 3)]);
        * = writeU16Le(65535, out[(pos + 3)..(pos + 5)]);
        return pos + 5;
    }
    let src_pos: usize = 0;
    while (src_pos < payload.len) {
        let rem: usize = payload.len - src_pos;
        let chunk: usize = rem;
        if (chunk > 65535) {
            chunk = 65535;
        }
        let is_final: bool = (src_pos + chunk) == payload.len;
        if (is_final) {
            out[pos] = 1;
        } else {
            out[pos] = 0;
        }
        let clen16: u16 = chunk as u16;
        let nlen16: u16 = clen16 ^ 65535;
        * = writeU16Le(clen16, out[(pos + 1)..(pos + 3)]);
        * = writeU16Le(nlen16, out[(pos + 3)..(pos + 5)]);
        pos = pos + 5;

        let i: usize = 0;
        while (i < chunk) {
            out[pos + i] = payload[src_pos + i];
            i = i + 1;
        }
        pos = pos + chunk;
        src_pos = src_pos + chunk;
    }
    return pos;
}

fn fl2DecodeStoredBlocks(stream: []u8, out: []u8): usize? {
    let pos: usize = 0;
    let out_pos: usize = 0;
    let done: bool = false;
    while (!done) {
        if (pos + 5 > stream.len) {
            return null;
        }
        let hdr_byte: u8 = stream[pos];
        let bfinal: bool = (hdr_byte & 1) != 0;
        let btype: u8 = (hdr_byte >> 1) & 3;
        let reserved: u8 = hdr_byte >> 3;
        if (btype != 0 || reserved != 0) {
            return null;
        }
        let blen: u16 = readU16Le(stream[(pos + 1)..(pos + 3)]) ?? 0;
        let nlen: u16 = readU16Le(stream[(pos + 3)..(pos + 5)]) ?? 0;
        if ((blen ^ nlen) != 65535) {
            return null;
        }
        pos = pos + 5;
        let chunk: usize = blen as usize;
        if (pos + chunk > stream.len || out_pos + chunk > out.len) {
            return null;
        }
        let i: usize = 0;
        while (i < chunk) {
            out[out_pos + i] = stream[pos + i];
            i = i + 1;
        }
        pos = pos + chunk;
        out_pos = out_pos + chunk;
        if (bfinal) {
            done = true;
        }
    }
    if (pos != stream.len) {
        return null;
    }
    return out_pos;
}

pub fn parseGzipHeader(frame: []u8): GzipHeader {
    if (frame.len < 10) {
        return fl2EmptyGzipHeader();
    }
    if (frame[0] != GZIP_ID1 || frame[1] != GZIP_ID2 || frame[2] != GZIP_CM_DEFLATE) {
        return fl2EmptyGzipHeader();
    }
    let flg: u8 = frame[3];
    if ((flg & 224) != 0) {
        return fl2EmptyGzipHeader();
    }
    let mtime_val: u32 = readU32Le(frame[4..8]) ?? 0;
    let os_val: u8 = frame[9];
    let pos: usize = 10;

    // FEXTRA
    if ((flg & 4) != 0) {
        if (pos + 2 > frame.len) {
            return fl2EmptyGzipHeader();
        }
        let xlen: u16 = readU16Le(frame[pos..(pos + 2)]) ?? 0;
        pos = pos + 2 + (xlen as usize);
        if (pos > frame.len) {
            return fl2EmptyGzipHeader();
        }
    }
    // FNAME
    if ((flg & 8) != 0) {
        while (pos < frame.len && frame[pos] != 0) {
            pos = pos + 1;
        }
        if (pos >= frame.len) {
            return fl2EmptyGzipHeader();
        }
        pos = pos + 1;
    }
    // FCOMMENT
    if ((flg & 16) != 0) {
        while (pos < frame.len && frame[pos] != 0) {
            pos = pos + 1;
        }
        if (pos >= frame.len) {
            return fl2EmptyGzipHeader();
        }
        pos = pos + 1;
    }
    // FHCRC
    if ((flg & 2) != 0) {
        if (pos + 2 > frame.len) {
            return fl2EmptyGzipHeader();
        }
        pos = pos + 2;
    }

    return GzipHeader {
        method: frame[2],
        flags: flg,
        mtime: mtime_val,
        os: os_val,
        data_offset: pos,
        valid: true,
    };
}

pub fn encodeGzipStored(payload: []u8, mtime: u32, out: []u8): usize? {
    let deflate_len: usize = fl2StoredEncodedLen(payload.len);
    let total_len: usize = 10 + deflate_len + 8;
    if (out.len < total_len) {
        return null;
    }

    out[0] = GZIP_ID1;
    out[1] = GZIP_ID2;
    out[2] = GZIP_CM_DEFLATE;
    out[3] = 0;
    * = writeU32Le(mtime, out[4..8]);
    out[8] = 0;
    out[9] = 255;

    let trailer: usize = fl2EncodeStoredBlocks(payload, out, 10);
    let crc: u32 = hashBytes(payload);
    let isize_val: u32 = payload.len as u32;
    * = writeU32Le(crc, out[trailer..(trailer + 4)]);
    * = writeU32Le(isize_val, out[(trailer + 4)..(trailer + 8)]);
    return trailer + 8;
}

pub fn decodeGzipStored(frame: []u8, out: []u8): usize? {
    let hdr = parseGzipHeader(frame);
    if (!hdr.valid) {
        return null;
    }
    if (frame.len < hdr.data_offset + 8) {
        return null;
    }
    let trailer: usize = frame.len - 8;
    let dec_opt = fl2DecodeStoredBlocks(frame[hdr.data_offset..trailer], out);
    if (dec_opt == null) {
        return null;
    }
    let dec_len: usize = dec_opt ?? 0;
    let expected_crc: u32 = readU32Le(frame[trailer..(trailer + 4)]) ?? 0;
    let expected_isize: u32 = readU32Le(frame[(trailer + 4)..(trailer + 8)]) ?? 0;
    let actual_crc: u32 = hashBytes(out[0..dec_len]);
    if (actual_crc != expected_crc) {
        return null;
    }
    if ((dec_len as u32) != expected_isize) {
        return null;
    }
    return dec_len;
}

pub fn encodeZlibStored(payload: []u8, out: []u8): usize? {
    let deflate_len: usize = fl2StoredEncodedLen(payload.len);
    let total_len: usize = 2 + deflate_len + 4;
    if (out.len < total_len) {
        return null;
    }

    out[0] = ZLIB_CMF_DEFAULT;
    out[1] = ZLIB_FLG_NO_COMP;

    let trailer: usize = fl2EncodeStoredBlocks(payload, out, 2);
    let checksum: u32 = adler32(payload);
    * = writeU32Be(checksum, out[trailer..(trailer + 4)]);
    return trailer + 4;
}

pub fn decodeZlibStored(frame: []u8, out: []u8): usize? {
    if (frame.len < 11) {
        return null;
    }
    let cmf: u8 = frame[0];
    let flg: u8 = frame[1];
    if ((cmf & 15) != 8 || (cmf >> 4) > 7) {
        return null;
    }
    let check_val: u32 = ((cmf as u32) * 256) + (flg as u32);
    if ((check_val % 31) != 0) {
        return null;
    }
    if ((flg & 32) != 0) {
        return null;
    }

    let trailer: usize = frame.len - 4;
    let dec_opt = fl2DecodeStoredBlocks(frame[2..trailer], out);
    if (dec_opt == null) {
        return null;
    }
    let dec_len: usize = dec_opt ?? 0;
    let expected_adler: u32 = readU32Be(frame[trailer..(trailer + 4)]) ?? 0;
    let actual_adler: u32 = adler32(out[0..dec_len]);
    if (actual_adler != expected_adler) {
        return null;
    }
    return dec_len;
}
