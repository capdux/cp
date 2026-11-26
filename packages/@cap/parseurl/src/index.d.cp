// Type definitions for CP

import { pickQueryParam, unescapeQuery } from "../../querystring/src/index.cp";
export fn purlIsAlpha(b: u8): bool;
export fn purlIsSchemeChar(b: u8): bool;
export fn purlHexVal(ch: u8): u8;
export fn purlFindOriginEnd(raw_url: string): usize;
export class ParsedRequestUrl {
    href: string,
    path: string,
    pathname: string,
    search: string,
    query: string,
    hash: string,
    has_query: bool,
    fn empty(): ParsedRequestUrl;
}

export fn parseRequestUrl(raw_url: string): ParsedRequestUrl;
export fn decodePathname(pathname: string, out: []u8): usize;
export fn isSafeRequestPath(pathname: string): bool;
export fn getRequestQueryParam(raw_url: string, key: string, out: []u8): usize?;
export fn decodeQueryValue(raw: string, out: []u8): usize;
