# graphql@1.0.0 (Agent Quickref)

> Zero-heap GraphQL Spec lexer, recursive-descent document parser/validator, and query minifier in pure CP (clean-room MIT counterpart to npm graphql v16 parser). | License: MIT
> Import: `import { ... } from "graphql";` | Full types: `src/index.d.cp`

## Functions
- `export fn gqlIsNameStart(c: u8): bool;`
- `export fn gqlIsNameContinue(c: u8): bool;`
- `export fn gqlIsWordTok(kind: u8): bool;`
- `export fn gqlWriteByte(out: []u8, pos: &mut usize, b: u8);`
- `export fn gqlAdvance(st: &mut GqlParserState);`
- `export fn gqlFail(st: &mut GqlParserState);`
- `export fn gqlCurText(st: &mut GqlParserState): string;`
- `export fn gqlExpect(st: &mut GqlParserState, kind: u8): bool;`
- `export fn gqlParseType(st: &mut GqlParserState, depth: usize);`
- `export fn gqlParseValue(st: &mut GqlParserState, is_const: bool, depth: usize);`
- `export fn gqlParseArguments(st: &mut GqlParserState, is_const: bool);`
- `export fn gqlParseDirectives(st: &mut GqlParserState, is_const: bool);`
- `export fn gqlParseVariableDefinitions(st: &mut GqlParserState);`
- `export fn gqlParseSelectionSet(st: &mut GqlParserState, depth: usize);`
- `export fn gqlParseDefinition(st: &mut GqlParserState);`
- `export fn parseGraphql(src: string): GqlParseResult;`
- `export fn minifyGraphql(src: string, out: []u8): usize;`
