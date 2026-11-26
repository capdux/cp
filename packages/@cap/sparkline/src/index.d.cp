// Type definitions for CP

export class SeriesStats {
    count: usize,
    minVal: f64,
    maxVal: f64,
    sum: f64,
    mean: f64,
    hasValues: bool,
}

export class SparkOptions {
    useCustomBounds: bool,
    minBound: f64,
    maxBound: f64,
    asciiOnly: bool,
}

export class SparkResult {
    buf: [512]u8,
    len: usize,
    truncated: bool,
    fn asSlice(): string;
}

export fn defaultOptions(): SparkOptions;
export fn computeStats(values: &[]f64): SeriesStats;
export fn appendBlock(res: &mut SparkResult, level: u8, asciiOnly: bool);
export fn appendByte(res: &mut SparkResult, b: u8);
export fn appendBytes(res: &mut SparkResult, s: string);
export fn quantizeLevel(value: f64, lo: f64, hi: f64): u8;
export fn renderWithOptions(values: &[]f64, opts: SparkOptions): SparkResult;
export fn render(values: &[]f64): SparkResult;
export fn renderAscii(values: &[]f64): SparkResult;
export fn renderBar(ratio: f64, width: usize, fillAscii: bool): SparkResult;
export fn renderDownsampled(values: &[]f64, targetBuckets: usize, asciiOnly: bool): SparkResult;
export fn renderLabeledBar(label: string, ratio: f64, barWidth: usize, fillAscii: bool): SparkResult;
