// Type definitions for CP

import { trim, isAlpha, isDigit, startsWith, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";
export fn phtIsSchemeChar(c: u8): bool;
export fn isProtocolRelative(url: string): bool;
export fn hasScheme(url: string): bool;
export fn prependHttp(url: string, https: bool, out: []u8): usize;
