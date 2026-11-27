// Type definitions for CP

export class SitemapUrl {
    loc: &[]u8,
    lastmod: &[]u8,
    changefreq: u8,
    priority_tenths: i8,
}

export fn freqName(f: u8): &[]u8;
export fn isValidPriorityTenths(p: i8): bool;
export class SitemapWriter {
    buf: []u8,
    pos: usize,
    url_count: usize,
    closed: bool,
    overflow: bool,
    fn init(buf: []u8): SitemapWriter;
    fn writeRaw(_: auto, s: &[]u8): bool;
    fn writeXmlEscaped(_: auto, s: &[]u8): bool;
    fn addUrl(_: auto, entry: SitemapUrl): bool;
    fn finish(_: auto): i64;
    fn urlCount(): usize;
    fn len(): usize;
    fn written(): &[]u8;
}

export class SitemapIndexWriter {
    buf: []u8,
    pos: usize,
    entry_count: usize,
    closed: bool,
    overflow: bool,
    fn init(buf: []u8): SitemapIndexWriter;
    fn writeRaw(_: auto, s: &[]u8): bool;
    fn writeXmlEscaped(_: auto, s: &[]u8): bool;
    fn addSitemap(_: auto, loc: &[]u8, lastmod: &[]u8): bool;
    fn finish(_: auto): i64;
    fn entryCount(): usize;
    fn written(): &[]u8;
}

