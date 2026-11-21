// Source: cp/cap/test/test_netcdf_production_deep.cp (Production deep tests for cap:netcdf)

import { test, expect_eq } from "cap:test";
import {
    NC_MAGIC,
    NC_ABSENT,
    NC_DIMENSION,
    NC_VARIABLE,
    NC_ATTRIBUTE,
    NC_UNLIMITED,
    NetcdfFormat,
    NetcdfType,
    netcdfTypeSize,
    decodeNetcdfType,
    NetcdfDim,
    NetcdfAttr,
    NetcdfVar,
    NetcdfHeader,
    detectNetcdfFormat,
    isNetcdf,
    parseNetcdfHeader,
    NetcdfDimIter,
    NetcdfAttrIter,
    NetcdfVarIter,
} from "cap:netcdf";

fn zeroBytes(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
}

fn writeU32BE(dst: []u8, off: usize, val: u32): usize {
    dst[off + 0] = ((val >> 24) & 0xFF) as u8;
    dst[off + 1] = ((val >> 16) & 0xFF) as u8;
    dst[off + 2] = ((val >> 8) & 0xFF) as u8;
    dst[off + 3] = (val & 0xFF) as u8;
    return off + 4;
}

fn writeU64BE(dst: []u8, off: usize, val: u64): usize {
    let p = writeU32BE(dst, off, ((val >> 32) & 0xFFFFFFFF) as u32);
    p = writeU32BE(dst, p, (val & 0xFFFFFFFF) as u32);
    return p;
}

fn writeNonNeg(dst: []u8, off: usize, val: u64, is_cdf5: bool): usize {
    if (is_cdf5) return writeU64BE(dst, off, val);
    return writeU32BE(dst, off, val as u32);
}

fn writeAlignedBytes(dst: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
    const pad: usize = ((4 as usize) - (src.len & 3)) & 3;
    return off + src.len + pad;
}

fn writeAlignedName(dst: []u8, off: usize, name: []const u8, is_cdf5: bool): usize {
    let p = writeNonNeg(dst, off, name.len as u64, is_cdf5);
    p = writeAlignedBytes(dst, p, name);
    return p;
}

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. parses CDF-1 Classic header, unlimited/fixed dims, global attrs, and multi-dim variable with local attrs", () => {
    let raw: [512]u8 = undefined;
    let buf = raw[0..512];
    zeroBytes(buf);

    buf[0] = 'C';
    buf[1] = 'D';
    buf[2] = 'F';
    buf[3] = 1; // CDF-1 Classic
    let p: usize = 4;
    p = writeU32BE(buf, p, 12); // numrecs = 12

    // Dimensions (2): time (UNLIMITED=0), lat (180)
    p = writeU32BE(buf, p, NC_DIMENSION);
    p = writeU32BE(buf, p, 2);
    p = writeAlignedName(buf, p, "time", false);
    p = writeU32BE(buf, p, 0); // NC_UNLIMITED
    p = writeAlignedName(buf, p, "lat", false); // 3 bytes -> 1 byte pad!
    p = writeU32BE(buf, p, 180);

    // Global attributes (1): title = "Climate" (7 bytes -> 1 byte pad!)
    p = writeU32BE(buf, p, NC_ATTRIBUTE);
    p = writeU32BE(buf, p, 1);
    p = writeAlignedName(buf, p, "title", false); // 5 bytes -> 3 bytes pad!
    p = writeU32BE(buf, p, 2); // NC_CHAR
    p = writeU32BE(buf, p, 7);
    p = writeAlignedBytes(buf, p, "Climate");

    // Variables (1): temp(time, lat) float32 with 1 local attr "units"="K"
    p = writeU32BE(buf, p, NC_VARIABLE);
    p = writeU32BE(buf, p, 1);
    p = writeAlignedName(buf, p, "temp", false);
    p = writeU32BE(buf, p, 2); // ndims
    p = writeU32BE(buf, p, 0); // dimid 0
    p = writeU32BE(buf, p, 1); // dimid 1
    // Local vatt_list (1 attr)
    p = writeU32BE(buf, p, NC_ATTRIBUTE);
    p = writeU32BE(buf, p, 1);
    p = writeAlignedName(buf, p, "units", false);
    p = writeU32BE(buf, p, 2); // NC_CHAR
    p = writeU32BE(buf, p, 1);
    p = writeAlignedBytes(buf, p, "K");
    p = writeU32BE(buf, p, 5); // NC_FLOAT
    p = writeU32BE(buf, p, 720); // vsize
    p = writeU32BE(buf, p, 1024); // begin_offset (32-bit in CDF-1)

    const nc = buf[0..p];
    expect_eq(isNetcdf(nc), true);
    expect_eq(detectNetcdfFormat(nc) == NetcdfFormat.classic, true);

    if (hdr := parseNetcdfHeader(nc)) {
        expect_eq(hdr.format == NetcdfFormat.classic, true);
        expect_eq(hdr.numrecs, 12);
        expect_eq(hdr.dim_count, 2);
        expect_eq(hdr.global_attr_count, 1);
        expect_eq(hdr.var_count, 1);
        expect_eq(hdr.header_end_offset, p);

        let dims = NetcdfDimIter.init(nc, hdr);
        if (d0 := dims.next()) {
            expect_eq(sliceEq(d0.name, "time"), true);
            expect_eq(d0.length, NC_UNLIMITED);
            expect_eq(d0.is_unlimited, true);
        } else {
            expect_eq(true, false);
        }
        if (d1 := dims.next()) {
            expect_eq(sliceEq(d1.name, "lat"), true);
            expect_eq(d1.length, 180);
            expect_eq(d1.is_unlimited, false);
        } else {
            expect_eq(true, false);
        }
        expect_eq(dims.next() == null, true);

        let attrs = NetcdfAttrIter.init(nc, hdr);
        if (a0 := attrs.next()) {
            expect_eq(sliceEq(a0.name, "title"), true);
            expect_eq(a0.nc_type == NetcdfType.char, true);
            expect_eq(a0.count, 7);
            expect_eq(sliceEq(a0.raw_values, "Climate"), true);
        } else {
            expect_eq(true, false);
        }
        expect_eq(attrs.next() == null, true);

        let vars = NetcdfVarIter.init(nc, hdr);
        if (v0 := vars.next()) {
            expect_eq(sliceEq(v0.name, "temp"), true);
            expect_eq(v0.ndims, 2);
            expect_eq(v0.nc_type == NetcdfType.float, true);
            expect_eq(v0.vsize, 720);
            expect_eq(v0.begin_offset, 1024);
            expect_eq(v0.attr_count, 1);
        } else {
            expect_eq(true, false);
        }
        expect_eq(vars.next() == null, true);
    } else {
        expect_eq(true, false);
    }
});

test("2. parses CDF-2 64-bit Offset format with 64-bit variable begin_offset (> 4 GiB)", () => {
    let raw: [256]u8 = undefined;
    let buf = raw[0..256];
    zeroBytes(buf);

    buf[0] = 'C';
    buf[1] = 'D';
    buf[2] = 'F';
    buf[3] = 2; // CDF-2 64-bit Offset
    let p: usize = 4;
    p = writeU32BE(buf, p, 0); // numrecs
    // Absent dims & global attrs
    p = writeU32BE(buf, p, NC_ABSENT);
    p = writeU32BE(buf, p, 0);
    p = writeU32BE(buf, p, NC_ABSENT);
    p = writeU32BE(buf, p, 0);
    // 1 variable with 64-bit begin_offset = 0x00000001_00002000 (4 GiB + 8 KiB)
    p = writeU32BE(buf, p, NC_VARIABLE);
    p = writeU32BE(buf, p, 1);
    p = writeAlignedName(buf, p, "weights", false);
    p = writeU32BE(buf, p, 0); // scalar (ndims=0)
    p = writeU32BE(buf, p, NC_ABSENT); // no local attrs
    p = writeU32BE(buf, p, 0);
    p = writeU32BE(buf, p, 6); // NC_DOUBLE
    p = writeU32BE(buf, p, 8); // vsize
    p = writeU64BE(buf, p, 4294975488); // 0x100002000

    const nc = buf[0..p];
    expect_eq(detectNetcdfFormat(nc) == NetcdfFormat.offset64, true);

    if (hdr := parseNetcdfHeader(nc)) {
        expect_eq(hdr.format == NetcdfFormat.offset64, true);
        expect_eq(hdr.dim_count, 0);
        expect_eq(hdr.global_attr_count, 0);
        expect_eq(hdr.var_count, 1);

        let vars = NetcdfVarIter.init(nc, hdr);
        if (v := vars.next()) {
            expect_eq(sliceEq(v.name, "weights"), true);
            expect_eq(v.nc_type == NetcdfType.double, true);
            expect_eq(v.begin_offset, 4294975488);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. parses CDF-5 64-bit Data format with 64-bit dim lengths, attr counts, and extended types", () => {
    let raw: [512]u8 = undefined;
    let buf = raw[0..512];
    zeroBytes(buf);

    buf[0] = 'C';
    buf[1] = 'D';
    buf[2] = 'F';
    buf[3] = 5; // CDF-5
    let p: usize = 4;
    p = writeU64BE(buf, p, 999); // numrecs (64-bit)

    // Dimensions (1): batch = 5000000000 (> u32::MAX)
    p = writeU32BE(buf, p, NC_DIMENSION);
    p = writeU64BE(buf, p, 1);
    p = writeAlignedName(buf, p, "batch", true);
    p = writeU64BE(buf, p, 5000000000);

    // Global attrs (1): version = 1 uint64 (8 bytes)
    p = writeU32BE(buf, p, NC_ATTRIBUTE);
    p = writeU64BE(buf, p, 1);
    p = writeAlignedName(buf, p, "v", true); // 1B + 3B pad
    p = writeU32BE(buf, p, 11); // NC_UINT64
    p = writeU64BE(buf, p, 1);
    p = writeU64BE(buf, p, 42);

    // Variables (1): ids(batch) int64
    p = writeU32BE(buf, p, NC_VARIABLE);
    p = writeU64BE(buf, p, 1);
    p = writeAlignedName(buf, p, "ids", true);
    p = writeU64BE(buf, p, 1); // ndims
    p = writeU64BE(buf, p, 0); // dimid 0 (64-bit in CDF-5)
    p = writeU32BE(buf, p, NC_ABSENT);
    p = writeU64BE(buf, p, 0);
    p = writeU32BE(buf, p, 10); // NC_INT64
    p = writeU64BE(buf, p, 8000000000); // vsize (64-bit)
    p = writeU64BE(buf, p, 8589934592); // begin_offset (8 GiB)

    const nc = buf[0..p];
    expect_eq(detectNetcdfFormat(nc) == NetcdfFormat.cdf5, true);

    if (hdr := parseNetcdfHeader(nc)) {
        expect_eq(hdr.format == NetcdfFormat.cdf5, true);
        expect_eq(hdr.numrecs, 999);
        expect_eq(hdr.dim_count, 1);
        expect_eq(hdr.global_attr_count, 1);
        expect_eq(hdr.var_count, 1);

        let dims = NetcdfDimIter.init(nc, hdr);
        if (d := dims.next()) {
            expect_eq(sliceEq(d.name, "batch"), true);
            expect_eq(d.length, 5000000000);
        } else {
            expect_eq(true, false);
        }

        let attrs = NetcdfAttrIter.init(nc, hdr);
        if (a := attrs.next()) {
            expect_eq(sliceEq(a.name, "v"), true);
            expect_eq(a.nc_type == NetcdfType.uint64, true);
            expect_eq(a.raw_values.len, 8);
        } else {
            expect_eq(true, false);
        }

        let vars = NetcdfVarIter.init(nc, hdr);
        if (v := vars.next()) {
            expect_eq(sliceEq(v.name, "ids"), true);
            expect_eq(v.nc_type == NetcdfType.int64, true);
            expect_eq(v.vsize, 8000000000);
            expect_eq(v.begin_offset, 8589934592);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("4. validates netcdfTypeSize and 4-byte alignment padding across 1/2/3/4-byte attribute values", () => {
    expect_eq(netcdfTypeSize(NetcdfType.byte), 1);
    expect_eq(netcdfTypeSize(NetcdfType.char), 1);
    expect_eq(netcdfTypeSize(NetcdfType.short), 2);
    expect_eq(netcdfTypeSize(NetcdfType.int), 4);
    expect_eq(netcdfTypeSize(NetcdfType.float), 4);
    expect_eq(netcdfTypeSize(NetcdfType.double), 8);
    expect_eq(netcdfTypeSize(NetcdfType.ubyte), 1);
    expect_eq(netcdfTypeSize(NetcdfType.ushort), 2);
    expect_eq(netcdfTypeSize(NetcdfType.uint), 4);
    expect_eq(netcdfTypeSize(NetcdfType.int64), 8);
    expect_eq(netcdfTypeSize(NetcdfType.uint64), 8);
    expect_eq(netcdfTypeSize(NetcdfType.unknown), 0);
    expect_eq(decodeNetcdfType(99) == NetcdfType.unknown, true);

    // 4 attrs with lengths 1, 2, 3, 4 bytes to verify 4B padding boundaries
    let raw: [256]u8 = undefined;
    let buf = raw[0..256];
    zeroBytes(buf);
    buf[0] = 'C';
    buf[1] = 'D';
    buf[2] = 'F';
    buf[3] = 1;
    let p: usize = 4;
    p = writeU32BE(buf, p, 0);
    p = writeU32BE(buf, p, NC_ABSENT);
    p = writeU32BE(buf, p, 0);
    p = writeU32BE(buf, p, NC_ATTRIBUTE);
    p = writeU32BE(buf, p, 4);

    p = writeAlignedName(buf, p, "a", false);
    p = writeU32BE(buf, p, 2);
    p = writeU32BE(buf, p, 1);
    p = writeAlignedBytes(buf, p, "1");

    p = writeAlignedName(buf, p, "bb", false);
    p = writeU32BE(buf, p, 2);
    p = writeU32BE(buf, p, 2);
    p = writeAlignedBytes(buf, p, "22");

    p = writeAlignedName(buf, p, "ccc", false);
    p = writeU32BE(buf, p, 2);
    p = writeU32BE(buf, p, 3);
    p = writeAlignedBytes(buf, p, "333");

    p = writeAlignedName(buf, p, "dddd", false);
    p = writeU32BE(buf, p, 2);
    p = writeU32BE(buf, p, 4);
    p = writeAlignedBytes(buf, p, "4444");

    p = writeU32BE(buf, p, NC_ABSENT);
    p = writeU32BE(buf, p, 0);

    if (hdr := parseNetcdfHeader(buf[0..p])) {
        let attrs = NetcdfAttrIter.init(buf[0..p], hdr);
        let n: usize = 0;
        while (a := attrs.next()) {
            n += 1;
            expect_eq(a.raw_values.len, n);
        }
        expect_eq(n, 4);
        expect_eq(attrs.had_error, false);
    } else {
        expect_eq(true, false);
    }
});

test("5. defensive bounds: truncated header, bad magic, invalid version, bad section tag, and overflow counts", () => {
    // 5a. Short / bad magic / invalid version (CDF-3 doesn't exist)
    expect_eq(isNetcdf("CD"), false);
    expect_eq(isNetcdf("XYZ1"), false);
    let bad_ver: [4]u8 = ['C', 'D', 'F', 3];
    expect_eq(isNetcdf(bad_ver[0..4]), false);
    expect_eq(parseNetcdfHeader(bad_ver[0..4]) == null, true);

    // 5b. Invalid section tag (99 instead of NC_DIMENSION/NC_ABSENT)
    let raw: [64]u8 = undefined;
    let buf = raw[0..64];
    zeroBytes(buf);
    buf[0] = 'C';
    buf[1] = 'D';
    buf[2] = 'F';
    buf[3] = 1;
    let p: usize = 4;
    p = writeU32BE(buf, p, 0);
    p = writeU32BE(buf, p, 99); // bad tag
    p = writeU32BE(buf, p, 0);
    expect_eq(parseNetcdfHeader(buf[0..32]) == null, true);

    // 5c. NC_ABSENT with nonzero count
    zeroBytes(buf);
    buf[0] = 'C';
    buf[1] = 'D';
    buf[2] = 'F';
    buf[3] = 1;
    p = writeU32BE(buf, 4, 0);
    p = writeU32BE(buf, p, NC_ABSENT);
    p = writeU32BE(buf, p, 5); // nonzero count for NC_ABSENT
    expect_eq(parseNetcdfHeader(buf[0..32]) == null, true);

    // 5d. Huge dim_count exceeding buffer
    zeroBytes(buf);
    buf[0] = 'C';
    buf[1] = 'D';
    buf[2] = 'F';
    buf[3] = 1;
    p = writeU32BE(buf, 4, 0);
    p = writeU32BE(buf, p, NC_DIMENSION);
    p = writeU32BE(buf, p, 0x7FFFFFFF);
    expect_eq(parseNetcdfHeader(buf[0..32]) == null, true);
});
