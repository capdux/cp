// Type definitions for CP

import { componentSet, percentEncode, percentDecodePlus } from "../../percent-encoding/src/index.cp";
export class QueryEntry {
    key: string,
    value: string,
}

export fn qstFindByte(s: string, start: usize, end: usize, target: u8): usize;
export fn qstStrEq(a: string, b: string): bool;
export fn qstNeedsDecode(s: string): bool;
export fn qstHasSlashBefore(s: string, end: usize): bool;
export fn escapeQuery(raw: string, out: []u8): usize;
export fn unescapeQuery(raw: string, out: []u8): usize;
export fn extractUrlQuery(url: string): string;
export fn parseQueryString(input: string, sep: u8, eq: u8, out_entries: []QueryEntry): usize;
export fn stringifyQueryString(entries: &[]QueryEntry, sep: u8, eq: u8, out: []u8): usize;
export fn pickQueryParam(url_or_query: string, target_key: string, out: []u8): usize?;
