// Source: cp/lib/color/wcag.cp (WCAG 2.1 Relative Luminance, Contrast & Palette Generator)
// Pure CP WCAG 2.1 accessibility luminance/contrast evaluator and harmonic palette generator.

import { Rgba, srgbToLinear, rotateHue } from "./spaces.cp";

fn blendChannelOver(fg: u8, bg: u8, alpha_norm: f64): u8 {
    const fg_f: f64 = @floatFromInt(fg);
    const bg_f: f64 = @floatFromInt(bg);
    const out = fg_f * alpha_norm + bg_f * (1.0 - alpha_norm);
    if (out <= 0.0) return 0;
    if (out >= 255.0) return 255;
    return @intFromFloat(@round(out));
}

fn compositeOver(fg: Rgba, bg: Rgba): Rgba {
    if (fg.a == 255) return fg;
    const alpha_f: f64 = @floatFromInt(fg.a);
    const alpha_norm = alpha_f / 255.0;
    return Rgba {
        r: blendChannelOver(fg.r, bg.r, alpha_norm),
        g: blendChannelOver(fg.g, bg.g, alpha_norm),
        b: blendChannelOver(fg.b, bg.b, alpha_norm),
        a: 255,
    };
}

/// WCAG 2.1 relative luminance and contrast ratio calculator.
pub static struct Wcag {
    /// Computes WCAG 2.1 relative luminance in `[0.0, 1.0]`.
    static fn relativeLuminance(c: Rgba): f64 {
        const r = srgbToLinear(c.r);
        const g = srgbToLinear(c.g);
        const b = srgbToLinear(c.b);
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    }

    /// Computes WCAG 2.1 contrast ratio `(L1 + 0.05) / (L2 + 0.05)` in `[1.0, 21.0]`.
    /// Translucent foreground colors (`fg.a < 255`) are alpha-composited over `bg`.
    static fn contrastRatio(fg: Rgba, bg: Rgba): f64 {
        const eff_fg = compositeOver(fg, bg);
        const l_fg = Wcag.relativeLuminance(eff_fg);
        const l_bg = Wcag.relativeLuminance(bg);
        const lighter = if (l_fg > l_bg) l_fg else l_bg;
        const darker = if (l_fg > l_bg) l_bg else l_fg;
        return (lighter + 0.05) / (darker + 0.05);
    }

    /// Returns `true` if `fg` over `bg` meets WCAG 2.1 Level AA normal text (`>= 4.5`).
    static fn meetsAA(fg: Rgba, bg: Rgba): bool {
        return Wcag.contrastRatio(fg, bg) >= 4.5;
    }

    /// Returns `true` if `fg` over `bg` meets WCAG 2.1 Level AA large text (`>= 3.0`).
    static fn meetsAALarge(fg: Rgba, bg: Rgba): bool {
        return Wcag.contrastRatio(fg, bg) >= 3.0;
    }

    /// Returns `true` if `fg` over `bg` meets WCAG 2.1 Level AAA normal text (`>= 7.0`).
    static fn meetsAAA(fg: Rgba, bg: Rgba): bool {
        return Wcag.contrastRatio(fg, bg) >= 7.0;
    }

    /// Picks either pure black `#000000` or pure white `#ffffff` for maximum WCAG contrast over `bg`.
    static fn bestTextColor(bg: Rgba): Rgba {
        const black = Rgba.rgb(0, 0, 0);
        const white = Rgba.rgb(255, 255, 255);
        const c_black = Wcag.contrastRatio(black, bg);
        const c_white = Wcag.contrastRatio(white, bg);
        if (c_black >= c_white) return black;
        return white;
    }
}

/// Harmonic color palette generator based on HSL hue wheel geometry.
pub static struct Palette {
    /// Returns `[base, base + 180deg]` complementary color pair.
    static fn complementary(c: Rgba): [2]Rgba {
        const out: [2]Rgba = [c, rotateHue(c, 180.0)];
        return out;
    }

    /// Returns `[base, base + 120deg, base + 240deg]` triadic color scheme.
    static fn triadic(c: Rgba): [3]Rgba {
        const out: [3]Rgba = [c, rotateHue(c, 120.0), rotateHue(c, 240.0)];
        return out;
    }

    /// Returns `[base - 30deg, base, base + 30deg]` analogous color scheme.
    static fn analogous(c: Rgba): [3]Rgba {
        const out: [3]Rgba = [rotateHue(c, -30.0), c, rotateHue(c, 30.0)];
        return out;
    }
}
