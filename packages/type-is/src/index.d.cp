// Type definitions for CP

import { lookup, parseMimeType } from "../../mime-types/src/index.cp";
import { eqAscii, endsWithIgnoreCase } from "../../unicase/src/index.cp";
export fn tisTrim(s: string): string;
export fn tisFindByte(s: string, target: u8): usize;
export fn normalizeTypePattern(pattern: string): string;
export fn typeMatches(content_type: string, pattern: string): bool;
export fn typeIs(content_type: string, candidates_csv: string): string?;
export fn hasBody(transfer_encoding: string, content_length: string): bool;
