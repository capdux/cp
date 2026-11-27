// Type definitions for CP

export enum SelectorKind {
    Wildcard,
    NodeType,
    Attribute,
    Compound,
    Descendant,
    Child,
    AdjacentSibling,
    Matches
}

export enum AttrOp {
    Exists,
    Eq,
    NotEq,
    Lt,
    Gt
}

export class AstNodeInfo {
    node_type: string,
    name: string,
    operator: string,
    parent_idx: i32,
    prev_sibling_idx: i32,
}

export class SelectorNode {
    kind: SelectorKind,
    attr_op: AttrOp,
    text: string,
    attr_name: string,
    attr_val: string,
    left: i32,
    right: i32,
}

export class ParsedSelector {
    ok: bool,
    root: i32,
    count: usize,
    error_pos: usize,
    nodes: [64]SelectorNode,
}

export class EsqParser {
    src: string,
    pos: usize,
    has_err: bool,
    err_pos: usize,
    count: usize,
    nodes: [64]SelectorNode,
}

export class EsqIntParse {
    ok: bool,
    val: i64,
}

export fn esqEmptyNode(): SelectorNode;
export fn esqIsWs(ch: u8): bool;
export fn esqIsIdentStart(ch: u8): bool;
export fn esqIsIdentChar(ch: u8): bool;
export fn esqCanStartSimple(ch: u8): bool;
export fn esqStrEq(a: string, b: string): bool;
export fn esqStrCmp(a: string, b: string): i32;
export fn esqParseI64(s: string): EsqIntParse;
export fn esqSetErr(p: &mut EsqParser, pos: usize);
export fn esqSkipWs(p: &mut EsqParser): bool;
export fn esqAddNode(p: &mut EsqParser, node: SelectorNode): i32;
export fn esqParseAttr(p: &mut EsqParser): i32;
export fn esqParseSimple(p: &mut EsqParser): i32;
export fn esqParseCompound(p: &mut EsqParser): i32;
export fn esqParseComplex(p: &mut EsqParser): i32;
export fn esqParseList(p: &mut EsqParser): i32;
export fn parseSelector(sel: string): ParsedSelector;
export fn esqGetAttrVal(ast_node: AstNodeInfo, attr_name: string): string;
export fn esqMatchAttr(ast_node: AstNodeInfo, sel_node: SelectorNode): bool;
export fn esqMatchRec(sel_nodes: &[]SelectorNode, sel_idx: i32, nodes: &[]AstNodeInfo, idx: usize, depth: usize): bool;
export fn matchesSelector(sel: ParsedSelector, nodes: &[]AstNodeInfo, idx: usize): bool;
export fn queryAll(sel_str: string, nodes: &[]AstNodeInfo, out_indices: []usize): usize;
