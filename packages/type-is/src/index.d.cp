// Type definitions for CP

import { trim, findByte } from "cap:ascii";
import { lookup, parseMimeType } from "../../mime-types/src/index.cp";
import { eqAscii, endsWithIgnoreCase } from "../../unicase/src/index.cp";
export class RequestHeaders {
    content_type: string,
    transfer_encoding: string,
    content_length: string,
}

export fn normalizeTypePattern(pattern: string): string;
export fn normalize(pattern: string): string?;
export fn extractEssence(content_type: string): string?;
export fn isValidContentType(content_type: string): bool;
export fn typeMatches(content_type: string, pattern: string): bool;
export fn matchType(expected: string, actual: string): bool;
export fn is(content_type: string, types: &[]string): string?;
export fn typeIs(content_type: string, candidates_csv: string): string?;
export fn hasBody(transfer_encoding: string, content_length: string): bool;
export fn requestHasBody(req: RequestHeaders): bool;
export fn typeis(req: RequestHeaders, types: &[]string): string?;
export fn typeisCsv(req: RequestHeaders, candidates_csv: string): string?;
export fn isJson(content_type: string): bool;
export fn isText(content_type: string): bool;
export fn isForm(content_type: string): bool;
export fn isMultipart(content_type: string): bool;
