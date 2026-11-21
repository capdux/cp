// Source: cp/cap/test/test_ubifs_production_deep.cp (Production deep tests for cap:ubifs)

import { test, expect_eq } from "cap:test";
import {
    UBI_EC_HDR_MAGIC,
    UBI_VID_HDR_MAGIC,
    UBI_HDR_SIZE,
    UBIFS_NODE_MAGIC,
    UBIFS_CH_SIZE,
    UBIFS_CRC32_INIT,
    UBIFS_INO_NODE,
    UBIFS_SB_NODE,
    UBIFS_MST_NODE,
    COMPR_NONE,
    COMPR_LZO,
    COMPR_ZLIB,
    COMPR_ZSTD,
    ubifsCrc32,
    parseUbiEcHeader,
    parseUbiVidHeader,
    isUbifsNode,
    parseUbifsNode,
    parseUbifsSuperblock,
    parseUbifsMaster,
} from "cap:ubifs";

fn writeU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU64Le(buf: []u8, off: usize, v: u64) {
    writeU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    writeU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn writeU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeU64Be(buf: []u8, off: usize, v: u64) {
    writeU32Be(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    writeU32Be(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn buildUbiEc(buf: []u8, ec: u64, vid_off: u32, data_off: u32, image_seq: u32) {
    let i: usize = 0;
    while (i < UBI_HDR_SIZE) {
        buf[i] = 0;
        i += 1;
    }
    writeU32Be(buf, 0, UBI_EC_HDR_MAGIC);
    buf[4] = 1; // version
    writeU64Be(buf, 8, ec);
    writeU32Be(buf, 16, vid_off);
    writeU32Be(buf, 20, data_off);
    writeU32Be(buf, 24, image_seq);
    writeU32Be(buf, 60, ubifsCrc32(buf[0..60]));
}

fn buildUbiVid(buf: []u8, vol_type: u8, vol_id: u32, lnum: u32, data_size: u32, sqnum: u64) {
    let i: usize = 0;
    while (i < UBI_HDR_SIZE) {
        buf[i] = 0;
        i += 1;
    }
    writeU32Be(buf, 0, UBI_VID_HDR_MAGIC);
    buf[4] = 1; // version
    buf[5] = vol_type;
    buf[6] = 0; // copy_flag
    buf[7] = 0; // compat
    writeU32Be(buf, 8, vol_id);
    writeU32Be(buf, 12, lnum);
    writeU32Be(buf, 20, data_size);
    writeU32Be(buf, 24, 16);        // used_ebs
    writeU32Be(buf, 28, 0);         // data_pad
    writeU32Be(buf, 32, 0x11223344); // data_crc
    writeU64Be(buf, 40, sqnum);
    writeU32Be(buf, 60, ubifsCrc32(buf[0..60]));
}

fn buildUbifsSb(buf: []u8, leb_size: u32, leb_cnt: u32, compr: u16) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
    writeU32Le(buf, 0, UBIFS_NODE_MAGIC);
    writeU64Le(buf, 8, 1); // sqnum
    writeU32Le(buf, 16, buf.len as u32);
    buf[20] = UBIFS_SB_NODE;
    buf[21] = 0;

    buf[26] = 0; // key_hash (R5)
    buf[27] = 0; // key_fmt (SIMPLE)
    writeU32Le(buf, 28, 0x01); // flags
    writeU32Le(buf, 32, 2048); // min_io_size
    writeU32Le(buf, 36, leb_size);
    writeU32Le(buf, 40, leb_cnt);
    writeU32Le(buf, 44, leb_cnt * 2); // max_leb_cnt
    writeU64Le(buf, 48, 8388608);     // max_bud_bytes = 8MB
    writeU32Le(buf, 56, 4);           // log_lebs
    writeU32Le(buf, 60, 2);           // lpt_lebs
    writeU32Le(buf, 64, 2);           // orph_lebs
    writeU32Le(buf, 68, 1);           // jhead_cnt
    writeU32Le(buf, 72, 8);           // fanout
    writeU32Le(buf, 76, 256);         // lsave_cnt
    writeU32Le(buf, 80, 5);           // fmt_version
    writeU16Le(buf, 84, compr);
    let g: usize = 0;
    while (g < 16) {
        buf[108 + g] = (0x70 + g) as u8;
        g += 1;
    }

    writeU32Le(buf, 4, ubifsCrc32(buf[8..buf.len]));
}

fn buildUbifsMaster(buf: []u8, cmt_no: u64, highest_inum: u64) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
    writeU32Le(buf, 0, UBIFS_NODE_MAGIC);
    writeU64Le(buf, 8, 99); // sqnum
    writeU32Le(buf, 16, buf.len as u32);
    buf[20] = UBIFS_MST_NODE;
    buf[21] = 0;

    writeU64Le(buf, 24, highest_inum);
    writeU64Le(buf, 32, cmt_no);
    writeU32Le(buf, 40, 0x03); // flags
    writeU32Le(buf, 44, 3);    // log_lnum
    writeU32Le(buf, 48, 12);   // root_lnum
    writeU32Le(buf, 52, 4096); // root_offs
    writeU32Le(buf, 56, 188);  // root_len
    writeU64Le(buf, 80, 50000000); // total_free
    writeU64Le(buf, 88, 1200000);  // total_dirty
    writeU64Le(buf, 96, 15000000); // total_used

    writeU32Le(buf, 4, ubifsCrc32(buf[8..buf.len]));
}

test("1. UBI 64B big-endian EC Header (UBI#) and VID Header (UBI!) with CRC32 verification", () => {
    expect_eq(UBI_EC_HDR_MAGIC, 0x55424923 as u32);
    expect_eq(UBI_VID_HDR_MAGIC, 0x55424921 as u32);
    expect_eq(UBI_HDR_SIZE, 64);
    expect_eq(UBIFS_CRC32_INIT, 0xFFFFFFFF as u32);

    // IEEE 802.3 CRC32 check: ubifsCrc32("123456789") ^ 0xFFFFFFFF == 0xCBF43926
    expect_eq(ubifsCrc32("123456789") ^ 0xFFFFFFFF, 0xCBF43926 as u32);

    let ec_buf: [64]u8 = [0; 64];
    buildUbiEc(ec_buf[0..], 42, 2048, 4096, 0xCAFEBABE);
    const ec = parseUbiEcHeader(ec_buf[0..]) ?? unreachable;
    expect_eq(ec.version, 1 as u8);
    expect_eq(ec.ec, 42 as u64);
    expect_eq(ec.vid_hdr_offset, 2048 as u32);
    expect_eq(ec.data_offset, 4096 as u32);
    expect_eq(ec.image_seq, 0xCAFEBABE as u32);

    let vid_buf: [64]u8 = [0; 64];
    buildUbiVid(vid_buf[0..], 1, 0, 7, 126976, 1234567);
    const vid = parseUbiVidHeader(vid_buf[0..]) ?? unreachable;
    expect_eq(vid.version, 1 as u8);
    expect_eq(vid.vol_type, 1 as u8);
    expect_eq(vid.vol_id, 0 as u32);
    expect_eq(vid.lnum, 7 as u32);
    expect_eq(vid.data_size, 126976 as u32);
    expect_eq(vid.used_ebs, 16 as u32);
    expect_eq(vid.data_crc, 0x11223344 as u32);
    expect_eq(vid.sqnum, 1234567 as u64);
});

test("2. UBIFS 24B Common Node Header (0x06101831) and [8..len] CRC32 verification", () => {
    expect_eq(UBIFS_NODE_MAGIC, 0x06101831 as u32);
    expect_eq(UBIFS_CH_SIZE, 24);

    let node: [40]u8 = [0; 40];
    writeU32Le(node[0..], 0, UBIFS_NODE_MAGIC);
    writeU64Le(node[0..], 8, 555);
    writeU32Le(node[0..], 16, 40);
    node[20] = UBIFS_INO_NODE;
    node[21] = 1;
    node[24] = 0xAA;
    node[39] = 0x55;
    writeU32Le(node[0..], 4, ubifsCrc32(node[8..40]));

    expect_eq(isUbifsNode(node[0..]), true);
    const ch = parseUbifsNode(node[0..]) ?? unreachable;
    expect_eq(ch.sqnum, 555 as u64);
    expect_eq(ch.len, 40 as u32);
    expect_eq(ch.node_type, UBIFS_INO_NODE);
    expect_eq(ch.group_type, 1 as u8);
    expect_eq(ch.body.len, 16);
    expect_eq(ch.body[0], 0xAA as u8);
    expect_eq(ch.body[15], 0x55 as u8);
});

test("3. UBIFS Superblock Node (type 6) full field parsing and totalBytes()", () => {
    let sb_buf: [256]u8 = [0; 256];
    buildUbifsSb(sb_buf[0..], 126976, 1024, COMPR_ZSTD);

    const sb = parseUbifsSuperblock(sb_buf[0..]) ?? unreachable;
    expect_eq(sb.ch.node_type, UBIFS_SB_NODE);
    expect_eq(sb.min_io_size, 2048 as u32);
    expect_eq(sb.leb_size, 126976 as u32);
    expect_eq(sb.leb_cnt, 1024 as u32);
    expect_eq(sb.max_leb_cnt, 2048 as u32);
    expect_eq(sb.totalBytes(), (126976 as u64) * (1024 as u64));
    expect_eq(sb.max_bud_bytes, 8388608 as u64);
    expect_eq(sb.log_lebs, 4 as u32);
    expect_eq(sb.lpt_lebs, 2 as u32);
    expect_eq(sb.orph_lebs, 2 as u32);
    expect_eq(sb.jhead_cnt, 1 as u32);
    expect_eq(sb.fanout, 8 as u32);
    expect_eq(sb.lsave_cnt, 256 as u32);
    expect_eq(sb.fmt_version, 5 as u32);
    expect_eq(sb.default_compr, COMPR_ZSTD);
    expect_eq(sb.uuid[0], 0x70 as u8);
    expect_eq(sb.uuid[15], 0x7F as u8);

    // Verify compression constants
    expect_eq(COMPR_NONE, 0 as u16);
    expect_eq(COMPR_LZO, 1 as u16);
    expect_eq(COMPR_ZLIB, 2 as u16);
});

test("4. UBIFS Master Node (type 7) full field parsing", () => {
    let mst_buf: [512]u8 = [0; 512];
    buildUbifsMaster(mst_buf[0..], 42, 65535);

    const mst = parseUbifsMaster(mst_buf[0..]) ?? unreachable;
    expect_eq(mst.ch.node_type, UBIFS_MST_NODE);
    expect_eq(mst.ch.sqnum, 99 as u64);
    expect_eq(mst.highest_inum, 65535 as u64);
    expect_eq(mst.cmt_no, 42 as u64);
    expect_eq(mst.flags, 0x03 as u32);
    expect_eq(mst.log_lnum, 3 as u32);
    expect_eq(mst.root_lnum, 12 as u32);
    expect_eq(mst.root_offs, 4096 as u32);
    expect_eq(mst.root_len, 188 as u32);
    expect_eq(mst.total_free, 50000000 as u64);
    expect_eq(mst.total_dirty, 1200000 as u64);
    expect_eq(mst.total_used, 15000000 as u64);
});

test("5. Malformed UBI & UBIFS rejection: bad magic, bad CRC32, wrong node_type, bad version, truncation", () => {
    // Truncation
    expect_eq(parseUbiEcHeader("short") == null, true);
    expect_eq(parseUbiVidHeader("short") == null, true);
    expect_eq(isUbifsNode("x"), false);
    expect_eq(parseUbifsNode("short") == null, true);

    // UBI EC: bad CRC32 & bad version
    let ec_buf: [64]u8 = [0; 64];
    buildUbiEc(ec_buf[0..], 1, 2048, 4096, 1);
    ec_buf[10] ^= 0x01;
    expect_eq(parseUbiEcHeader(ec_buf[0..]) == null, true);
    buildUbiEc(ec_buf[0..], 1, 2048, 4096, 1);
    ec_buf[4] = 2; // version != 1
    writeU32Be(ec_buf[0..], 60, ubifsCrc32(ec_buf[0..60]));
    expect_eq(parseUbiEcHeader(ec_buf[0..]) == null, true);

    // UBI VID: bad magic & bad CRC32
    let vid_buf: [64]u8 = [0; 64];
    buildUbiVid(vid_buf[0..], 1, 0, 0, 0, 1);
    vid_buf[0] = 0x00;
    expect_eq(parseUbiVidHeader(vid_buf[0..]) == null, true);
    buildUbiVid(vid_buf[0..], 1, 0, 0, 0, 1);
    vid_buf[40] ^= 0xFF;
    expect_eq(parseUbiVidHeader(vid_buf[0..]) == null, true);

    // UBIFS Superblock: bad CRC32 & passing Master node to parseUbifsSuperblock (node_type mismatch)
    let sb_buf: [256]u8 = [0; 256];
    buildUbifsSb(sb_buf[0..], 126976, 100, COMPR_LZO);
    sb_buf[50] ^= 0x02;
    expect_eq(parseUbifsSuperblock(sb_buf[0..]) == null, true);

    let mst_buf: [512]u8 = [0; 512];
    buildUbifsMaster(mst_buf[0..], 1, 10);
    expect_eq(parseUbifsSuperblock(mst_buf[0..]) == null, true);

    buildUbifsSb(sb_buf[0..], 126976, 100, COMPR_LZO);
    expect_eq(parseUbifsMaster(sb_buf[0..]) == null, true);
});
