// Type definitions for CP

import { isSpace, trim, eqIgnoreCase, findByte, rfindByte } from "cap:ascii";
import { indexOfBytes } from "cap:bin";
import { writeStr, writeU64, parseU16 } from "cap:fmt";
export enum GitProtocol {
    Https = 0,
    Http = 1,
    Ssh = 2,
    Git = 3,
    File = 4,
    Shorthand = 5
}

export enum GitHostKind {
    GitHub = 0,
    GitLab = 1,
    Bitbucket = 2,
    Gitee = 3,
    Codeberg = 4,
    Other = 5
}

export class ParsedGitUrl {
    valid: bool,
    protocol: GitProtocol,
    host_kind: GitHostKind,
    user: string,
    host: string,
    port: u16,
    owner: string,
    repo: string,
    ref_name: string,
    subpath: string,
    fn invalid(): ParsedGitUrl;
}

export fn startsWithIgnoreCase(s: string, prefix: string): bool;
export fn indexOfChar(s: string, ch: u8): usize?;
export fn lastIndexOfChar(s: string, ch: u8): usize?;
export fn indexOfSubstr(s: string, sub: string): usize?;
export fn hasWhitespace(s: string): bool;
export fn parsePort(s: string): u16?;
export fn detectHostKind(host: string): GitHostKind;
export fn stripDotGit(s: string): string;
export fn isValidSegment(s: string): bool;
export fn parseGitUrl(raw: string): ParsedGitUrl;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn appendU16(out: []u8, pos: usize, val: u16): usize;
export fn toHttpsUrl(parsed: ParsedGitUrl, out: []u8): string;
export fn toSshUrl(parsed: ParsedGitUrl, out: []u8): string;
export fn toBrowseUrl(parsed: ParsedGitUrl, out: []u8): string;
