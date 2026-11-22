// Source: Absorbed from /data/dev/bun/src/runtime/image/codec_gif.cp | Synced: 2026-11-21T23:33:53Z
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/codec_gif.cp
// Upstream Sync: Refer to bun/src/runtime/image/codec_gif.zig for future upstream updates.
// ============================================================================
const std = @import("std");
import { Codecs } from "./codecs";

pub struct GifHeader {
    pub width: usize,
    pub height: usize,
    pub has_gct: bool,
    pub gct_size: usize,
}

pub struct GifImage {
    pub width: usize,
    pub height: usize,
    pub rgba: []u8,
}

struct BitReader {
    src: []const u8,
    i: usize,
    block: usize,
    acc: u32,
    nbits: u32,
    eof: bool,

    fn init(src: []const u8, start_offset: usize): BitReader {
        return BitReader{
            src: src,
            i: start_offset,
            block: 0,
            acc: 0,
            nbits: 0,
            eof: false,
        };
    }

    fn readBits(*_, n: u32): u16 {
        while (_.nbits < n && !_.eof) {
            if (_.block == 0) {
                if (_.i >= _.src.len) {
                    _.eof = true;
                    break;
                }
                _.block = (_.src[_.i] as usize);
                _.i = _.i + 1;
                if (_.block == 0) {
                    _.eof = true;
                    break;
                }
            }
            if (_.i >= _.src.len) {
                _.eof = true;
                break;
            }
            let byte_val = (_.src[_.i] as u32);
            zig {
                self.acc = self.acc | (byte_val << @as(u5, @intCast(self.nbits)));
            }
            _.i = _.i + 1;
            _.block = _.block - 1;
            _.nbits = _.nbits + 8;
        }
        if (_.nbits < n) {
            return 0;
        }
        let v: u16 = 0;
        zig {
            const shift: u5 = @intCast(n);
            const mask: u32 = (@as(u32, 1) << shift) - 1;
            v = @as(u16, @intCast(self.acc & mask));
            self.acc = self.acc >> shift;
        }
        _.nbits = _.nbits - n;
        return v;
    }

    fn drain(*_) {
        let mut_idx = _.i + _.block;
        let idx = mut_idx;
        while (idx < _.src.len) {
            let n = (_.src[idx] as usize);
            idx = idx + 1 + n;
            if (n == 0) {
                break;
            }
        }
        if (idx < _.src.len) {
            _.i = idx;
        } else {
            _.i = _.src.len;
        }
    }
}

pub class Gif {
    static fn getTableSize(idx: usize): usize {
        return switch (idx) {
            0 => 2,
            1 => 4,
            2 => 8,
            3 => 16,
            4 => 32,
            5 => 64,
            6 => 128,
            7 => 256,
            else => 0,
        };
    }

    /// 快速探测 GIF 头与全局屏幕描述符 (Logical Screen Descriptor)
    pub static fn probe(bytes: []const u8): GifHeader? {
        if (bytes.len < 13) {
            return null;
        }
        let is_gif87 = bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == '8' && bytes[4] == '7' && bytes[5] == 'a';
        let is_gif89 = bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == '8' && bytes[4] == '9' && bytes[5] == 'a';
        if (!is_gif87 && !is_gif89) {
            return null;
        }

        let w = (bytes[6] as usize) | ((bytes[7] as usize) << 8);
        let h = (bytes[8] as usize) | ((bytes[9] as usize) << 8);
        let lsd_packed = bytes[10];
        let has_gct = (lsd_packed & 0x80) != 0;
        let gct_size: usize = if (has_gct) Gif.getTableSize((lsd_packed & 7) as usize) else 0;

        return GifHeader{
            width: w,
            height: h,
            has_gct: has_gct,
            gct_size: gct_size,
        };
    }

    /// 解码 GIF 首帧为完整 RGBA8 像素矩阵
    pub static fn decode(bytes: []const u8): GifImage? {
        if (Gif.probe(bytes) == null) {
            return null;
        }
        let lsd_packed = bytes[10];
        let has_gct = (lsd_packed & 0x80) != 0;
        let gct_size: usize = if (has_gct) Gif.getTableSize((lsd_packed & 7) as usize) else 0;

        let start_offset: usize = (13 as usize) + gct_size * (3 as usize);
        if (start_offset > bytes.len) {
            return null;
        }

        let gct = if (has_gct) bytes[13 .. ((13 as usize) + gct_size * (3 as usize))] else bytes[0..0];
        let mut_trns_idx: i32 = -1;
        let trns_idx = mut_trns_idx;
        let offset = start_offset;

        // 遍历数据块
        while (offset < bytes.len) {
            let marker = bytes[offset];
            if (marker == 0x3B) {
                // Trailer
                break;
            }

            if (marker == 0x21) {
                // Extension Introducer
                if (offset + 2 > bytes.len) {
                    return null;
                }
                let label = bytes[offset + 1];
                offset = offset + 2;
                if (label == 0xF9 && offset + 6 <= bytes.len && bytes[offset] == 4) {
                    // Graphic Control Extension (透明通道)
                    if ((bytes[offset + 1] & 1) != 0) {
                        trns_idx = (bytes[offset + 4] as i32);
                    }
                }
                // 跳过后续子块
                while (offset < bytes.len) {
                    let n = (bytes[offset] as usize);
                    offset = offset + 1 + n;
                    if (n == 0) {
                        break;
                    }
                }
            } else if (marker == 0x2C) {
                // Image Descriptor
                if (offset + 10 > bytes.len) {
                    return null;
                }
                let w = (bytes[offset + 5] as usize) | ((bytes[offset + 6] as usize) << 8);
                let h = (bytes[offset + 7] as usize) | ((bytes[offset + 8] as usize) << 8);
                let ipacked = bytes[offset + 9];
                let interlace = (ipacked & 0x40) != 0;
                let has_lct = (ipacked & 0x80) != 0;
                let lct_size: usize = if (has_lct) Gif.getTableSize((ipacked & 7) as usize) else 0;
                offset = offset + 10;

                if (w == 0 || h == 0) {
                    return null;
                }

                let ct: []const u8 = gct;
                if (has_lct) {
                    if (offset + lct_size * (3 as usize) > bytes.len) {
                        return null;
                    }
                    ct = bytes[offset .. (offset + lct_size * (3 as usize))];
                    offset = offset + lct_size * (3 as usize);
                }

                if (ct.len == 0 || offset >= bytes.len) {
                    return null;
                }

                let min_code = (bytes[offset] as u32);
                offset = offset + 1;

                return Gif.decodeFrame(bytes, offset, w, h, interlace, ct, min_code, trns_idx);
            } else {
                return null;
            }
        }
        return null;
    }

    static fn decodeFrame(
        bytes: []const u8,
        lzw_offset: usize,
        w: usize,
        h: usize,
        interlace: bool,
        ct: []const u8,
        min_code_in: u32,
        trns_idx: i32,
    ): GifImage? {
        let npix = w * h;
        if (npix == 0) {
            return null;
        }

        let min_code = if (min_code_in < 2) 2 else if (min_code_in > 11) 11 else min_code_in;

        let clear: u16 = 0;
        zig {
            clear = @as(u16, 1) << @as(u4, @intCast(min_code));
        }
        let eoi: u16 = clear + 1;
        let code_size = min_code + 1;
        let avail: u16 = eoi + 1;
        let prev_code: i32 = -1;

        // 4096 节点 LZW 字典
        let prefix: []u16 = [];
        zig {
            prefix = std.heap.page_allocator.alloc(u16, 4096) catch unreachable;
        }
        defer {
            zig {
                std.heap.page_allocator.free(prefix);
            }
        }
        let suffix_ptr: [*]u8 = @alloc(4096);
        let suffix = unsafe { suffix_ptr[0..4096] };
        let scratch_ptr: [*]u8 = @alloc(4096);
        let scratch = unsafe { scratch_ptr[0..4096] };

        let idx_ptr: [*]u8 = @alloc(npix);
        let idx_buf = unsafe { idx_ptr[0..npix] };
        let written: usize = 0;

        let bits = BitReader.init(bytes, lzw_offset);

        while (written < npix) {
            let code = bits.readBits(code_size);
            if (bits.eof && code == 0) {
                break;
            }
            if (code == clear) {
                code_size = min_code + 1;
                avail = eoi + 1;
                prev_code = -1;
                continue;
            }
            if (code == eoi) {
                break;
            }

            let first_byte: u8 = 0;
            if (code < avail) {
                // emit from dict
                let curr = code;
                let n: usize = 0;
                while (curr >= clear && n < 4096) {
                    scratch[n] = suffix[curr as usize];
                    curr = prefix[curr as usize];
                    n = n + 1;
                }
                scratch[n] = (curr as u8);
                n = n + 1;
                first_byte = scratch[n - 1];

                let cap = if (n < (npix - written)) n else (npix - written);
                let k: usize = 0;
                while (k < cap) {
                    idx_buf[written + k] = scratch[n - 1 - k];
                    k += 1;
                }
                written = written + cap;
            } else if (code == avail && prev_code >= 0) {
                // K-w-K 特殊情况
                let curr = (prev_code as u16);
                let n: usize = 0;
                while (curr >= clear && n < 4096) {
                    scratch[n] = suffix[curr as usize];
                    curr = prefix[curr as usize];
                    n = n + 1;
                }
                scratch[n] = (curr as u8);
                n = n + 1;
                first_byte = scratch[n - 1];

                let cap = if (n < (npix - written)) n else (npix - written);
                let k: usize = 0;
                while (k < cap) {
                    idx_buf[written + k] = scratch[n - 1 - k];
                    k += 1;
                }
                written = written + cap;
                if (written < npix) {
                    idx_buf[written] = first_byte;
                    written = written + 1;
                }
            } else {
                break;
            }

            if (prev_code >= 0 && avail < 4096) {
                prefix[avail as usize] = (prev_code as u16);
                suffix[avail as usize] = first_byte;
                avail = avail + 1;
                let limit_code: u16 = 0;
                zig {
                    limit_code = @as(u16, 1) << @as(u4, @intCast(code_size));
                }
                if (avail == limit_code && code_size < 12) {
                    code_size = code_size + 1;
                }
            }
            prev_code = (code as i32);
        }
        bits.drain();

        // 索引颜色映射至 RGBA 像素矩阵
        let rgba_len = npix * 4;
        let rgba_ptr: [*]u8 = @alloc(rgba_len);
        let rgba = unsafe { rgba_ptr[0..rgba_len] };

        if (!interlace) {
            let p: usize = 0;
            while (p < npix) {
                let color_idx = (idx_buf[p] as usize);
                let dst_off = p * 4;
                if (color_idx * 3 + 2 < ct.len) {
                    rgba[dst_off + 0] = ct[color_idx * 3 + 0];
                    rgba[dst_off + 1] = ct[color_idx * 3 + 1];
                    rgba[dst_off + 2] = ct[color_idx * 3 + 2];
                } else {
                    rgba[dst_off + 0] = 0;
                    rgba[dst_off + 1] = 0;
                    rgba[dst_off + 2] = 0;
                }
                if (trns_idx >= 0 && (color_idx as i32) == trns_idx) {
                    rgba[dst_off + 3] = 0;
                } else {
                    rgba[dst_off + 3] = 255;
                }
                p += 1;
            }
        } else {
            // GIF 4-pass 交织扫描
            let passes_start: [4]usize = [0, 4, 2, 1];
            let passes_step: [4]usize = [8, 8, 4, 2];
            let src_pix: usize = 0;

            let pass: usize = 0;
            while (pass < 4) {
                let start_y = passes_start[pass];
                let step_y = passes_step[pass];
                let y = start_y;
                while (y < h && src_pix < npix) {
                    let x: usize = 0;
                    while (x < w) {
                        if (src_pix >= npix) {
                            break;
                        }
                        let color_idx = (idx_buf[src_pix] as usize);
                        src_pix = src_pix + 1;

                        let dst_off = (y * w + x) * 4;
                        if (color_idx * 3 + 2 < ct.len) {
                            rgba[dst_off + 0] = ct[color_idx * 3 + 0];
                            rgba[dst_off + 1] = ct[color_idx * 3 + 1];
                            rgba[dst_off + 2] = ct[color_idx * 3 + 2];
                        } else {
                            rgba[dst_off + 0] = 0;
                            rgba[dst_off + 1] = 0;
                            rgba[dst_off + 2] = 0;
                        }
                        if (trns_idx >= 0 && (color_idx as i32) == trns_idx) {
                            rgba[dst_off + 3] = 0;
                        } else {
                            rgba[dst_off + 3] = 255;
                        }
                        x += 1;
                    }
                    y = y + step_y;
                }
                pass += 1;
            }
        }

        return GifImage{
            width: w,
            height: h,
            rgba: rgba,
        };
    }
}
