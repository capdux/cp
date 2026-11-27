// Type definitions for CP

export class Rgb8 {
    r: u8,
    g: u8,
    b: u8,
}

export class ColorPalette {
    colors: [256]Rgb8,
    len: usize,
    fn nearestIndex(px: Rgb8): u8;
}

export fn clqtEmptyPalette(): ColorPalette;
export fn clqtBucketIndex(px: Rgb8): usize;
export fn buildUniformPalette(): ColorPalette;
export fn quantizeHistogram(pixels: &[]Rgb8, max_colors: usize): ColorPalette;
export fn mapPixelsToPalette(pixels: &[]Rgb8, pal: ColorPalette, out_indices: []u8): usize;
