// Type definitions for CP

import { isSpace, eqIgnoreCase, findByte, trim } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
export class LinkEntry {
    url: string,
    rel: string,
    title: string,
    type_val: string,
    hreflang: string,
    fn empty(): LinkEntry;
}

export fn relMatches(rel_attr: string, target: string): bool;
export class LinkHeader {
    entries: [32]LinkEntry,
    count: usize,
    fn empty(): LinkHeader;
    fn entryAt(i: usize): LinkEntry?;
    fn getByRel(rel: string): LinkEntry?;
    fn nextUrl(): string?;
    fn prevUrl(): string?;
    fn firstUrl(): string?;
    fn lastUrl(): string?;
}

export fn skipToNextEntry(s: string, start: usize): usize;
export fn parseLinkHeader(header: string): LinkHeader;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendParam(out: []u8, pos: usize, name: string, val: string): usize;
export fn formatLinkHeader(entries: []LinkEntry, out: []u8): string;
export fn appendPageLink(out: []u8, start_pos: usize, base_url: string, sep: u8, target_page: usize, rel: string, emitted: usize): usize;
export fn formatPaginationLinks(base_url: string, page: usize, total_pages: usize, out: []u8): string;
