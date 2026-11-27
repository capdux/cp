# cjs-module-lexer@1.0.0 (Agent Quickref)

> Zero-allocation CommonJS static named export and re-export lexer (npm cjs-module-lexer parity) for CP | License: MIT
> Import: `import { ... } from "cjs-module-lexer";` | Full types: `src/index.d.cp`

## Functions
- `export fn cjslIsWs(ch: u8): bool;`
- `export fn cjslIsIdentStart(ch: u8): bool;`
- `export fn cjslIsIdentChar(ch: u8): bool;`
- `export fn cjslStrEq(a: string, b: string): bool;`
- `export fn cjslAddExport(res: &mut CjsParseResult, name: string);`
- `export fn cjslAddReexport(res: &mut CjsParseResult, spec: string);`
- `export fn cjslSkipString(lex: &mut CjslLexer);`
- `export fn cjslSkipWsAndComments(lex: &mut CjslLexer);`
- `export fn cjslReadIdent(lex: &mut CjslLexer): string;`
- `export fn cjslReadStringLit(lex: &mut CjslLexer): CjslStrRes;`
- `export fn cjslMatchRequireCall(lex: &mut CjslLexer): CjslStrRes;`
- `export fn cjslSkipPropValue(lex: &mut CjslLexer);`
- `export fn cjslParseObjectExports(lex: &mut CjslLexer, res: &mut CjsParseResult);`
- `export fn cjslParseExportsMemberAssign(lex: &mut CjslLexer, res: &mut CjsParseResult);`
- `export fn cjslParseDefineProperty(lex: &mut CjslLexer, res: &mut CjsParseResult);`
- `export fn cjslParseExportStar(lex: &mut CjslLexer, res: &mut CjsParseResult);`
- `export fn parseCjs(src: string): CjsParseResult;`
