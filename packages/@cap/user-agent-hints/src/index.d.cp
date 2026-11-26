// Type definitions for CP

export class BrandVersion {
    brand: string,
    version: string,
}

export fn uahToLower(c: u8): u8;
export fn uahIsOws(c: u8): bool;
export fn uahTrimOws(s: &[]u8): &[]u8;
export fn uahEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn uahFindIgnoreCaseFrom(haystack: &[]u8, needle: &[]u8, start_idx: usize): usize;
export fn uahScanQuoted(header: &[]u8, pos: &mut usize, innerStart: &mut usize, innerEnd: &mut usize): bool;
export fn uahSkipToNextItem(header: &[]u8, pos: &mut usize);
export fn uahParseNextBrand(header: &[]u8, pos: &mut usize, item: &mut BrandVersion): bool;
export fn parseBrands(header: string, out: []BrandVersion): usize;
export fn isGreaseBrand(brand: string): bool;
export fn findBrandVersion(header: string, brand: string): string;
export fn parseMobile(header: string): i32;
export fn parsePlatform(header: string): string;
export fn uahQuotedEncodedLen(s: &[]u8): usize;
export fn uahWriteQuoted(out: []u8, pos: &mut usize, s: &[]u8);
export fn formatBrands(brands: &[]BrandVersion, out: []u8): usize;
