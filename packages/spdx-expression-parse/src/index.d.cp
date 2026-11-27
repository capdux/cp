// Type definitions for CP

import { lookupLicense } from "../../spdx/src/index.cp";
import { strEq, startsWith, isSpace, isAlnum } from "cap:ascii";
export class SpdxAstNode {
    kind: u8,
    license: string,
    exception: string,
    plus: bool,
    left: i32,
    right: i32,
}

export class SpdxAst {
    nodes: [32]SpdxAstNode,
    root: i32,
    node_count: usize,
    valid: bool,
}

export class SpdxpState {
    src: string,
    pos: usize,
    depth: usize,
    failed: bool,
    ast: SpdxAst,
}

export fn spdxpEmptyNode(): SpdxAstNode;
export fn spdxpEmptyAst(): SpdxAst;
export fn spdxpIsIdChar(ch: u8): bool;
export fn spdxpIsKeyword(tok: string): bool;
export fn spdxpIsAllIdChars(s: string): bool;
export fn spdxpIsValidLicenseRef(id: string): bool;
export fn spdxpIsValidLicenseId(id: string): bool;
export fn spdxpIsValidExceptionId(exc: string): bool;
export fn spdxpSkipSpaces(st: &mut SpdxpState);
export fn spdxpAllocNode(st: &mut SpdxpState, node: SpdxAstNode): i32;
export fn spdxpConsumeKeyword(st: &mut SpdxpState, kw: string): bool;
export fn spdxpParseSimple(st: &mut SpdxpState): i32;
export fn spdxpParsePrimary(st: &mut SpdxpState): i32;
export fn spdxpParseAnd(st: &mut SpdxpState): i32;
export fn spdxpParseOr(st: &mut SpdxpState): i32;
export fn parseSpdxExpression(expr: string): SpdxAst;
export fn isValidSpdxExpression(expr: string): bool;
export fn spdxpMatchLicenseNode(node: SpdxAstNode, allowed: string): bool;
export fn spdxpEvalAst(ast: SpdxAst, idx: i32, allowed: string, depth: usize): bool;
export fn satisfiesSpdx(ast: SpdxAst, allowed_license: string): bool;
