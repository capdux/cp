// Type definitions for CP

import { QualitySpec, parseWeightedList } from "../../negotiator/src/index.cp";
import { eqAscii } from "../../unicase/src/index.cp";
export fn acphFindSlash(s: string): usize;
export fn acphIsStar(s: string): bool;
export fn acphMediaSpecificity(range: string): u8;
export fn acphMatchesRange(range: string, mime: string): bool;
export fn acphEmptyEntry(): AcceptEntry;
export fn acphEmptySpec(): QualitySpec;
export class AcceptEntry {
    media_range: string,
    quality_milli: u16,
    specificity: u8,
}

export class AcceptHeader {
    entries: [16]AcceptEntry,
    count: usize,
    fn parse(header: string): AcceptHeader;
    fn len(): usize;
    fn accepts(mime: string): bool;
    fn bestMatch(available: []string): string?;
}

export fn negotiateMediaType(header: string, available: []string): string?;
