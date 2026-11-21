// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Truevision TGA Image Codec)

/// Truevision TGA 图像类型
pub enum TgaImageType {
    no_image,
    color_mapped,
    true_color,
    grayscale,
    rle_color_mapped,
    rle_true_color,
    rle_grayscale,
    unknown,
}

/// 18 字节小端 Truevision TGA 文件头
pub struct TgaHeader {
    id_length: u8,
    color_map_type: u8,
    image_type: TgaImageType,
    width: u16,
    height: u16,
    pixel_depth: u8,
    descriptor: u8,
    color_map_bytes: usize = 0,

    /// 图像原点是否在左上角（`descriptor & 0x20 != 0`）
    fn isTopOrigin(): bool {
        return (_.descriptor & 0x20) != 0;
    }

    /// 是否为 RLE 压缩类型
    fn isRle(): bool {
        return _.image_type == TgaImageType.rle_color_mapped
            || _.image_type == TgaImageType.rle_true_color
            || _.image_type == TgaImageType.rle_grayscale;
    }

    /// 每像素字节数（`8->1`, `24->3`, `32->4`）
    fn bytesPerPixel(): u8 {
        return _.pixel_depth / 8;
    }

    /// 像素数据起始字节偏移（18 字节头 + Image ID + 调色板长度）
    fn pixelDataOffset(): usize {
        return 18 + usize(_.id_length) + _.color_map_bytes;
    }
}

fn readU16Le(data: []const u8, pos: usize): u16 {
    return u16(data[pos]) | (u16(data[pos + 1]) << 8);
}

fn writeU16Le(out: []u8, pos: usize, v: u16) {
    out[pos] = u8(v & 0xFF);
    out[pos + 1] = u8((v >> 8) & 0xFF);
}

fn decodeImageType(raw: u8): TgaImageType {
    if (raw == 0) return TgaImageType.no_image;
    if (raw == 1) return TgaImageType.color_mapped;
    if (raw == 2) return TgaImageType.true_color;
    if (raw == 3) return TgaImageType.grayscale;
    if (raw == 9) return TgaImageType.rle_color_mapped;
    if (raw == 10) return TgaImageType.rle_true_color;
    if (raw == 11) return TgaImageType.rle_grayscale;
    return TgaImageType.unknown;
}

/// 解析 18 字节小端 TGA 文件头（校验长度、非零尺寸、合法色深与偏移边界）
pub fn parseTgaHeader(data: []const u8): TgaHeader? {
    if (data.len < 18) {
        return null;
    }
    const id_length = data[0];
    const color_map_type = data[1];
    if (color_map_type > 1) {
        return null;
    }
    const image_type = decodeImageType(data[2]);
    if (image_type == TgaImageType.unknown) {
        return null;
    }

    const cm_length = readU16Le(data, 5);
    const cm_entry_bits = data[7];
    let cm_bytes: usize = 0;
    if (color_map_type == 1) {
        const entry_bytes = usize((u16(cm_entry_bits) + 7) / 8);
        cm_bytes = usize(cm_length) * entry_bytes;
    }

    const width = readU16Le(data, 12);
    const height = readU16Le(data, 14);
    const pixel_depth = data[16];
    const descriptor = data[17];

    if (width == 0 || height == 0) {
        return null;
    }
    if (pixel_depth != 8 && pixel_depth != 24 && pixel_depth != 32) {
        return null;
    }

    const hdr = TgaHeader {
        id_length: id_length,
        color_map_type: color_map_type,
        image_type: image_type,
        width: width,
        height: height,
        pixel_depth: pixel_depth,
        descriptor: descriptor,
        color_map_bytes: cm_bytes,
    };

    if (hdr.pixelDataOffset() > data.len) {
        return null;
    }
    return hdr;
}

/// 将 top-origin RGB(3) 或 RGBA(4) 像素编码为未压缩 TrueColor TGA（BGR/BGRA，top-origin）；缓冲不足返回 `null`
pub fn encodeTgaTrueColor(
    rgba_or_rgb: []const u8,
    width: u16,
    height: u16,
    channels: u8,
    out: []u8,
): ([]const u8)? {
    if (width == 0 || height == 0) return null;
    if (channels != 3 && channels != 4) return null;

    const pixel_count = u64(width) * u64(height);
    const payload_u64 = pixel_count * u64(channels);
    if (payload_u64 != u64(rgba_or_rgb.len)) {
        return null;
    }
    const total_len = 18 + rgba_or_rgb.len;
    if (out.len < total_len) {
        return null;
    }

    // 清零 18 字节头
    let h: usize = 0;
    while (h < 18) {
        out[h] = 0;
        h += 1;
    }
    out[2] = 2; // uncompressed true-color
    writeU16Le(out, 12, width);
    writeU16Le(out, 14, height);
    out[16] = channels * 8;
    out[17] = if (channels == 4) 0x28 else 0x20; // top-origin (+ 8 alpha bits for 32-bit)

    const ch = usize(channels);
    let i: usize = 0;
    while (i < rgba_or_rgb.len) {
        out[18 + i] = rgba_or_rgb[i + 2];     // B
        out[18 + i + 1] = rgba_or_rgb[i + 1]; // G
        out[18 + i + 2] = rgba_or_rgb[i];     // R
        if (channels == 4) {
            out[18 + i + 3] = rgba_or_rgb[i + 3]; // A
        }
        i += ch;
    }

    return out[0..total_len];
}

fn writeRgbaAt(
    out_rgba: []u8,
    px_index: usize,
    width: usize,
    height: usize,
    top_origin: bool,
    right_origin: bool,
    r: u8,
    g: u8,
    b: u8,
    a: u8,
) {
    const src_y = px_index / width;
    const src_x = px_index % width;
    const dst_y = if (top_origin) src_y else height - 1 - src_y;
    const dst_x = if (right_origin) width - 1 - src_x else src_x;
    const off = (dst_y * width + dst_x) * 4;
    out_rgba[off] = r;
    out_rgba[off + 1] = g;
    out_rgba[off + 2] = b;
    out_rgba[off + 3] = a;
}

/// 零堆分配解码未压缩或 RLE 的 24/32-bit TrueColor 与 8-bit 灰度 TGA 为标准 top-origin RGBA
pub fn decodeTgaToRgba(data: []const u8, out_rgba: []u8): TgaHeader? {
    let hdr = TgaHeader {
        id_length: 0,
        color_map_type: 0,
        image_type: TgaImageType.unknown,
        width: 0,
        height: 0,
        pixel_depth: 0,
        descriptor: 0,
        color_map_bytes: 0,
    };
    if (h := parseTgaHeader(data)) {
        hdr = h;
    } else {
        return null;
    }

    const is_tc = hdr.image_type == TgaImageType.true_color
        || hdr.image_type == TgaImageType.rle_true_color;
    const is_gray = hdr.image_type == TgaImageType.grayscale
        || hdr.image_type == TgaImageType.rle_grayscale;

    if (is_tc) {
        if (hdr.pixel_depth != 24 && hdr.pixel_depth != 32) return null;
    } else if (is_gray) {
        if (hdr.pixel_depth != 8) return null;
    } else {
        return null;
    }

    const w = usize(hdr.width);
    const h = usize(hdr.height);
    const total_pixels = w * h;
    const needed_rgba = total_pixels * 4;
    if (out_rgba.len < needed_rgba) {
        return null;
    }

    const bpp = usize(hdr.bytesPerPixel());
    const top_origin = hdr.isTopOrigin();
    const right_origin = (hdr.descriptor & 0x10) != 0;
    let src = hdr.pixelDataOffset();

    if (!hdr.isRle()) {
        const needed_src = total_pixels * bpp;
        if (needed_src > data.len - src) {
            return null;
        }
        let px: usize = 0;
        while (px < total_pixels) {
            let r: u8 = 0;
            let g: u8 = 0;
            let b: u8 = 0;
            let a: u8 = 255;
            if (bpp == 1) {
                const v = data[src];
                r = v;
                g = v;
                b = v;
            } else if (bpp == 3) {
                b = data[src];
                g = data[src + 1];
                r = data[src + 2];
            } else {
                b = data[src];
                g = data[src + 1];
                r = data[src + 2];
                a = data[src + 3];
            }
            writeRgbaAt(out_rgba, px, w, h, top_origin, right_origin, r, g, b, a);
            src += bpp;
            px += 1;
        }
        return hdr;
    }

    // RLE 解码路径
    let px: usize = 0;
    while (px < total_pixels) {
        if (src >= data.len) {
            return null;
        }
        const pkt = data[src];
        src += 1;
        const count = usize(pkt & 0x7F) + 1;
        if (count > total_pixels - px) {
            return null;
        }

        if ((pkt & 0x80) != 0) {
            // RLE 重复像素包
            if (bpp > data.len - src) {
                return null;
            }
            let r: u8 = 0;
            let g: u8 = 0;
            let b: u8 = 0;
            let a: u8 = 255;
            if (bpp == 1) {
                const v = data[src];
                r = v;
                g = v;
                b = v;
            } else if (bpp == 3) {
                b = data[src];
                g = data[src + 1];
                r = data[src + 2];
            } else {
                b = data[src];
                g = data[src + 1];
                r = data[src + 2];
                a = data[src + 3];
            }
            src += bpp;

            let rep: usize = 0;
            while (rep < count) {
                writeRgbaAt(out_rgba, px, w, h, top_origin, right_origin, r, g, b, a);
                px += 1;
                rep += 1;
            }
        } else {
            // Raw 原始像素包
            const raw_bytes = count * bpp;
            if (raw_bytes > data.len - src) {
                return null;
            }
            let k: usize = 0;
            while (k < count) {
                let r: u8 = 0;
                let g: u8 = 0;
                let b: u8 = 0;
                let a: u8 = 255;
                if (bpp == 1) {
                    const v = data[src];
                    r = v;
                    g = v;
                    b = v;
                } else if (bpp == 3) {
                    b = data[src];
                    g = data[src + 1];
                    r = data[src + 2];
                } else {
                    b = data[src];
                    g = data[src + 1];
                    r = data[src + 2];
                    a = data[src + 3];
                }
                writeRgbaAt(out_rgba, px, w, h, top_origin, right_origin, r, g, b, a);
                src += bpp;
                px += 1;
                k += 1;
            }
        }
    }

    return hdr;
}
