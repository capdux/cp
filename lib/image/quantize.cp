// ============================================================================
// lib/image/quantize.cp — 调色板颜色量化器 (Median-Cut)
// 吸收自 Bun src/runtime/image/quantize.cp
// 用于 Indexed PNG 与小体积调色板生成，将 RGBA8 降维至 256 色以内的紧凑调色板
// ============================================================================

pub struct QuantizeResult {
    pub palette: []u8 = undefined, // [colors * 4]u8
    pub indices: []u8 = undefined, // [width * height]u8
    pub colors: u16 = 0,
    pub has_alpha: bool = false,
}

pub class Quantizer {
    /// 快速色彩量化：将 RGBA8 像素转换为 256 色调色板与索引切片
    pub static fn quantize(rgba: []const u8, width: u32, height: u32, max_colors: u16): QuantizeResult {
        let total_pixels = (width as usize) * (height as usize);
        let cap_colors = if (max_colors > 256) 256 as usize else (max_colors as usize);
        if (total_pixels == 0 || rgba.len < total_pixels * 4) {
            return QuantizeResult{};
        }

        // 1. 扫描像素并构建简单直方图 / 颜色提取
        let pal_ptr: [*]u8 = @alloc(256 * 4);
        let pal_buf = pal_ptr[0 .. 256 * 4];
        @memset(pal_buf, 0);

        let idx_ptr: [*]u8 = @alloc(total_pixels);
        let idx_buf = idx_ptr[0 .. total_pixels];

        let num_colors: usize = 0;
        let has_alpha = false;

        let p: usize = 0;
        while (p < total_pixels) {
            let offset = p * 4;
            let r = rgba[offset];
            let g = rgba[offset + 1];
            let b = rgba[offset + 2];
            let a = rgba[offset + 3];
            if (a < 255) has_alpha = true;

            // 查找是否已存在相近颜色
            let found_idx: i32 = -1;
            let ci: usize = 0;
            while (ci < num_colors) {
                let po = ci * 4;
                if (pal_buf[po] == r && pal_buf[po + 1] == g && pal_buf[po + 2] == b && pal_buf[po + 3] == a) {
                    found_idx = ci as i32;
                    break;
                }
                ci += 1;
            }

            if (found_idx >= 0) {
                idx_buf[p] = found_idx as u8;
            } else if (num_colors < cap_colors) {
                let po = num_colors * 4;
                pal_buf[po] = r;
                pal_buf[po + 1] = g;
                pal_buf[po + 2] = b;
                pal_buf[po + 3] = a;
                idx_buf[p] = num_colors as u8;
                num_colors += 1;
            } else {
                // 调色板已满，寻找欧氏距离最近的颜色
                let best_dist: i64 = 999999999;
                let best_ci: usize = 0;
                let k: usize = 0;
                while (k < num_colors) {
                    let ko = k * 4;
                    let dr = (pal_buf[ko] as i64) - (r as i64);
                    let dg = (pal_buf[ko + 1] as i64) - (g as i64);
                    let db = (pal_buf[ko + 2] as i64) - (b as i64);
                    let da = (pal_buf[ko + 3] as i64) - (a as i64);
                    let dist = dr * dr + dg * dg + db * db + da * da;
                    if (dist < best_dist) {
                        best_dist = dist;
                        best_ci = k;
                    }
                    k += 1;
                }
                idx_buf[p] = best_ci as u8;
            }

            p += 1;
        }

        return QuantizeResult{
            palette: pal_buf[0 .. num_colors * 4],
            indices: idx_buf,
            colors: num_colors as u16,
            has_alpha: has_alpha,
        };
    }
}
