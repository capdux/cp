import { test, expect } from "cap:test";
import { Transform, Orientation, parseTiff, readJpeg } from "cap:exif";

test("exif: orientation transforms and dimension swaps", () => {
    const t1 = Orientation.normal.transform();
    expect(t1.isIdentity()).toBeTrue();
    expect(t1.swapsDimensions()).toBeFalse();

    const t3 = Orientation.rotate180.transform();
    expect(t3.flop).toBeFalse();
    expect(t3.flip).toBeFalse();
    expect(t3.rotate).toBe(180);
    expect(t3.swapsDimensions()).toBeFalse();

    const t6 = Orientation.rotate90.transform();
    expect(t6.flop).toBeFalse();
    expect(t6.flip).toBeFalse();
    expect(t6.rotate).toBe(90);
    expect(t6.swapsDimensions()).toBeTrue();

    const t5 = Orientation.flop_rotate90.transform();
    expect(t5.flop).toBeTrue();
    expect(t5.flip).toBeFalse();
    expect(t5.rotate).toBe(270);
    expect(t5.swapsDimensions()).toBeTrue();

    const t7 = Orientation.flop_rotate270.transform();
    expect(t7.flop).toBeTrue();
    expect(t7.flip).toBeFalse();
    expect(t7.rotate).toBe(90);
    expect(t7.swapsDimensions()).toBeTrue();

    expect(Orientation.fromU16(0) == null).toBeTrue();
    expect(Orientation.fromU16(9) == null).toBeTrue();
    expect(Orientation.fromU16(6).? == .rotate90).toBeTrue();
    expect(Orientation.rotate270.toU8()).toBe(8);
});

test("exif: parseTiff little-endian (II) and big-endian (MM)", () => {
    // Little-endian TIFF: "II", magic=42, ifd0=8, count=1, tag=0x0112, type=3, count=1, val=6
    const tiff_le: [22]u8 = [
        'I', 'I', 0x2A, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x01, 0x00,
        0x12, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    ];
    expect(parseTiff(tiff_le[0..]).? == .rotate90).toBeTrue();

    // Big-endian TIFF: "MM", magic=42, ifd0=8, count=2 (first tag skipped, second is 0x0112 val=8)
    const tiff_be: [34]u8 = [
        'M', 'M', 0x00, 0x2A, 0x00, 0x00, 0x00, 0x08,
        0x00, 0x02,
        // Entry 0: tag 0x010F (Make), skipped
        0x01, 0x0F, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
        // Entry 1: tag 0x0112 (Orientation), type=3, count=1, val=8
        0x01, 0x12, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00,
    ];
    expect(parseTiff(tiff_be[0..]).? == .rotate270).toBeTrue();
});

test("exif: readJpeg APP1 segment with padding and preceding APP0", () => {
    // SOI (2) + APP0 (6) + 0xFF pad (1) + APP1 hdr (4) + Exif\0\0 (6) + TIFF LE (22) + SOS (2) = 43 bytes
    const jpg: [43]u8 = [
        0xFF, 0xD8,
        0xFF, 0xE0, 0x00, 0x04, 0x00, 0x00,
        0xFF, 0xFF, 0xE1, 0x00, 0x1E,
        'E', 'x', 'i', 'f', 0x00, 0x00,
        'I', 'I', 0x2A, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x01, 0x00,
        0x12, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0xFF, 0xDA,
    ];
    expect(readJpeg(jpg[0..]) == .rotate180).toBeTrue();
});

test("exif: malformed inputs gracefully return normal / null", () => {
    const empty: [0]u8 = [];
    expect(readJpeg(empty[0..]) == .normal).toBeTrue();
    expect(parseTiff(empty[0..]) == null).toBeTrue();

    // Non-JPEG header
    const png_sig: [8]u8 = [0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A];
    expect(readJpeg(png_sig[0..]) == .normal).toBeTrue();

    // Truncated APP1 segment
    const trunc: [10]u8 = [0xFF, 0xD8, 0xFF, 0xE1, 0x00, 0x50, 'E', 'x', 'i', 'f'];
    expect(readJpeg(trunc[0..]) == .normal).toBeTrue();

    // Invalid TIFF magic (43 instead of 42)
    const bad_magic: [10]u8 = ['I', 'I', 0x2B, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00];
    expect(parseTiff(bad_magic[0..]) == null).toBeTrue();

    // Out-of-range orientation value (9)
    const bad_val: [22]u8 = [
        'I', 'I', 0x2A, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x01, 0x00,
        0x12, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
    ];
    expect(parseTiff(bad_val[0..]) == null).toBeTrue();
});
