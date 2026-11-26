// Type definitions for CP

export class GqlToken {
    kind: u8,
    start: usize,
    end: usize,
    line: usize,
    col: usize,
}

export class GqlParseResult {
    ok: bool,
    error_pos: usize,
    operation_count: usize,
    fragment_count: usize,
    field_count: usize,
    max_depth: usize,
    primary_op_type: u8,
    primary_op_name: string,
}

export fn gqlIsDigit(c: u8): bool;
export fn gqlIsHex(c: u8): bool;
export fn gqlIsNameStart(c: u8): bool;
export fn gqlIsNameContinue(c: u8): bool;
export fn gqlStrEq(a: string, b: string): bool;
export fn gqlIsWordTok(kind: u8): bool;
export fn gqlWriteByte(out: []u8, pos: &mut usize, b: u8);
export fn gqlWriteStr(out: []u8, pos: &mut usize, s: string);
export class GqlLexer {
    src: string,
    pos: usize,
    line: usize,
    col: usize,
    fn new(src: string): GqlLexer;
    fn tokenText(tok: GqlToken): string;
    fn peekToken(): GqlToken;
    fn nextToken(_: auto): GqlToken;
}

export class GqlParserState {
    lexer: GqlLexer,
    cur: GqlToken,
    ok: bool,
    error_pos: usize,
    operation_count: usize,
    fragment_count: usize,
    field_count: usize,
    max_depth: usize,
    primary_op_type: u8,
    primary_op_name: string,
}

export fn gqlAdvance(st: &mut GqlParserState);
export fn gqlFail(st: &mut GqlParserState);
export fn gqlCurText(st: &mut GqlParserState): string;
export fn gqlExpect(st: &mut GqlParserState, kind: u8): bool;
export fn gqlParseType(st: &mut GqlParserState, depth: usize);
export fn gqlParseValue(st: &mut GqlParserState, is_const: bool, depth: usize);
export fn gqlParseArguments(st: &mut GqlParserState, is_const: bool);
export fn gqlParseDirectives(st: &mut GqlParserState, is_const: bool);
export fn gqlParseVariableDefinitions(st: &mut GqlParserState);
export fn gqlParseSelectionSet(st: &mut GqlParserState, depth: usize);
export fn gqlParseDefinition(st: &mut GqlParserState);
export fn parseGraphql(src: string): GqlParseResult;
export fn minifyGraphql(src: string, out: []u8): usize;
