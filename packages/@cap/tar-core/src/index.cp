pub const TAR_BLOCK_SIZE: usize = 512;
pub const TYPE_FILE: u8 = 48;
pub const TYPE_SYMLINK: u8 = 50;
pub const TYPE_DIR: u8 = 53;

pub struct TarHeader {
    pub name: string,
    pub mode: u32,
    pub uid: u32,
    pub gid: u32,
    pub size: u64,
    pub mtime: u64,
    pub typeflag: u8,
    pub checksum: u32,
    pub is_ustar: bool,
    pub valid: bool,
}

pub struct TarEntrySpan {
    pub header: TarHeader,
    pub data_offset: usize,
    pub data_len: usize,
}

fn tarEmptyHeader(): TarHeader {
    return TarHeader {
        name: "",
        mode: 0,
        uid: 0,
        gid: 0,
        size: 0,
        mtime: 0,
        typeflag: 0,
        checksum: 0,
        is_ustar: false,
        valid: false,
    };
}

fn tarWriteOctal(out: []u8, offset: usize, digits: usize, val: u64): bool {
    let v: u64 = val;
    let i: usize = digits;
    while (i > 0) {
        i = i - 1;
        let d: u8 = (v & 7) as u8;
        out[offset + i] = 48 + d;
        v = v >> 3;
    }
    return v == 0;
}

fn tarParseOctal(buf: []u8, offset: usize, width: usize, require_digit: bool): u64? {
    let i: usize = 0;
    let end: usize = offset + width;
    if (end > buf.len) {
        return null;
    }
    while (i < width) {
        let b: u8 = buf[offset + i];
        if (b == 32 || b == 0) {
            i = i + 1;
        } else {
            break;
        }
    }
    let val: u64 = 0;
    let digits: usize = 0;
    while (i < width) {
        let b: u8 = buf[offset + i];
        if (b == 0 || b == 32) {
            break;
        }
        if (b < 48 || b > 55) {
            return null;
        }
        let d: u64 = (b - 48) as u64;
        val = (val << 3) | d;
        digits = digits + 1;
        i = i + 1;
    }
    if (require_digit && digits == 0) {
        return null;
    }
    while (i < width) {
        let b: u8 = buf[offset + i];
        if (b == 0) {
            break;
        }
        if (b != 32) {
            return null;
        }
        i = i + 1;
    }
    return val;
}

fn tarComputeChecksum(block512: []u8): u32 {
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < TAR_BLOCK_SIZE) {
        if (i >= 148 && i < 156) {
            sum = sum + 32;
        } else {
            sum = sum + (block512[i] as u32);
        }
        i = i + 1;
    }
    return sum;
}

fn tarIsUstarMagic(block512: []u8): bool {
    if (block512.len < TAR_BLOCK_SIZE) {
        return false;
    }
    let magic_ok: bool =
        block512[257] == 117 &&
        block512[258] == 115 &&
        block512[259] == 116 &&
        block512[260] == 97 &&
        block512[261] == 114 &&
        block512[262] == 0;
    let ver_ok: bool = block512[263] == 48 && block512[264] == 48;
    return magic_ok && ver_ok;
}

pub fn isZeroBlock(block512: []u8): bool {
    if (block512.len < TAR_BLOCK_SIZE) {
        return false;
    }
    let acc: u8 = 0;
    let i: usize = 0;
    while (i < TAR_BLOCK_SIZE) {
        acc = acc | block512[i];
        i = i + 1;
    }
    return acc == 0;
}

pub fn tarPaddedSize(raw_size: u64): u64 {
    if (raw_size == 0) {
        return 0;
    }
    let rem: u64 = raw_size % 512;
    if (rem == 0) {
        return raw_size;
    }
    return raw_size + (512 - rem);
}

pub fn encodeTarHeader(
    name: string,
    size: u64,
    mode: u32,
    mtime: u64,
    typeflag: u8,
    out512: []u8
): bool {
    if (out512.len < TAR_BLOCK_SIZE) {
        return false;
    }
    if (name.len == 0 || name.len > 100) {
        return false;
    }
    let i: usize = 0;
    while (i < TAR_BLOCK_SIZE) {
        out512[i] = 0;
        i = i + 1;
    }

    let ni: usize = 0;
    while (ni < name.len) {
        out512[ni] = name[ni];
        ni = ni + 1;
    }

    if (!tarWriteOctal(out512, 100, 7, mode as u64)) {
        return false;
    }
    out512[107] = 0;

    * = tarWriteOctal(out512, 108, 7, 0);
    out512[115] = 0;

    * = tarWriteOctal(out512, 116, 7, 0);
    out512[123] = 0;

    if (!tarWriteOctal(out512, 124, 11, size)) {
        return false;
    }
    out512[135] = 0;

    if (!tarWriteOctal(out512, 136, 11, mtime)) {
        return false;
    }
    out512[147] = 0;

    let ci: usize = 148;
    while (ci < 156) {
        out512[ci] = 32;
        ci = ci + 1;
    }

    out512[156] = typeflag;

    out512[257] = 117;
    out512[258] = 115;
    out512[259] = 116;
    out512[260] = 97;
    out512[261] = 114;
    out512[262] = 0;
    out512[263] = 48;
    out512[264] = 48;

    let sum: u32 = tarComputeChecksum(out512);
    if (!tarWriteOctal(out512, 148, 6, sum as u64)) {
        return false;
    }
    out512[154] = 0;
    out512[155] = 32;
    return true;
}

pub fn parseTarHeader(block512: []u8): TarHeader {
    if (block512.len < TAR_BLOCK_SIZE) {
        return tarEmptyHeader();
    }
    if (isZeroBlock(block512)) {
        return tarEmptyHeader();
    }

    let stored_opt = tarParseOctal(block512, 148, 8, true);
    if (stored_opt == null) {
        return tarEmptyHeader();
    }
    let stored_sum: u32 = (stored_opt ?? 0) as u32;
    let computed_sum: u32 = tarComputeChecksum(block512);
    if (stored_sum != computed_sum) {
        return tarEmptyHeader();
    }

    let mode_opt = tarParseOctal(block512, 100, 8, false);
    let uid_opt = tarParseOctal(block512, 108, 8, false);
    let gid_opt = tarParseOctal(block512, 116, 8, false);
    let size_opt = tarParseOctal(block512, 124, 12, false);
    let mtime_opt = tarParseOctal(block512, 136, 12, false);
    if (mode_opt == null || uid_opt == null || gid_opt == null || size_opt == null || mtime_opt == null) {
        return tarEmptyHeader();
    }

    let name_len: usize = 0;
    while (name_len < 100 && block512[name_len] != 0) {
        name_len = name_len + 1;
    }
    let name_str: string = block512[0..name_len];

    return TarHeader {
        name: name_str,
        mode: (mode_opt ?? 0) as u32,
        uid: (uid_opt ?? 0) as u32,
        gid: (gid_opt ?? 0) as u32,
        size: size_opt ?? 0,
        mtime: mtime_opt ?? 0,
        typeflag: block512[156],
        checksum: computed_sum,
        is_ustar: tarIsUstarMagic(block512),
        valid: true,
    };
}

pub struct TarArchiveCursor {
    pub offset: usize,
    pub finished: bool,

    pub static fn init(): TarArchiveCursor {
        return TarArchiveCursor {
            offset: 0,
            finished: false,
        };
    }

    pub fn reset(*_) {
        _.offset = 0;
        _.finished = false;
    }

    pub fn nextEntry(*_, archive: []u8): TarEntrySpan? {
        if (_.finished) {
            return null;
        }
        if (_.offset + TAR_BLOCK_SIZE > archive.len) {
            _.finished = true;
            return null;
        }
        let hdr_block = archive[_.offset..(_.offset + TAR_BLOCK_SIZE)];
        if (isZeroBlock(hdr_block)) {
            _.finished = true;
            return null;
        }
        let hdr = parseTarHeader(hdr_block);
        if (!hdr.valid) {
            _.finished = true;
            return null;
        }
        let data_off: usize = _.offset + TAR_BLOCK_SIZE;
        let data_len: usize = hdr.size as usize;
        if (data_off + data_len > archive.len) {
            _.finished = true;
            return null;
        }
        let padded: usize = tarPaddedSize(hdr.size) as usize;
        if (data_off + padded > archive.len) {
            _.offset = archive.len;
            _.finished = true;
        } else {
            _.offset = data_off + padded;
        }
        return TarEntrySpan {
            header: hdr,
            data_offset: data_off,
            data_len: data_len,
        };
    }
}
