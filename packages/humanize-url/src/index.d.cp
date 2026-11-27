// Type definitions for CP

import { isAlpha, isAlnum, toLowerByte, trim, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";

export fn isProtocolRelative(url: string): bool;
export fn extractScheme(url: string): string;
export fn stripProtocol(url: string): string;
export fn stripWww(host_and_rest: string): string;
export fn stripTrailingSlash(url: string): string;
export fn humanizeUrlSlice(url: string): string;
export fn humanizedLen(url: string): usize;
export fn humanizeUrl(url: string, out: []u8): usize;
