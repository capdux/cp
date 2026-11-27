[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# graphql

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap GraphQL Spec lexer, recursive-descent document parser/validator, and query minifier in pure CP (clean-room MIT counterpart to npm graphql v16 parser).

## 安装与导入

```bash
cap add graphql
```

```cp
import { gqlIsDigit } from "graphql";
```

## 核心导出 API

- `export fn gqlIsDigit(c: u8): bool;`
- `export fn gqlIsHex(c: u8): bool;`
- `export fn gqlIsNameStart(c: u8): bool;`
- `export fn gqlIsNameContinue(c: u8): bool;`
- `export fn gqlStrEq(a: string, b: string): bool;`
- `export fn gqlIsWordTok(kind: u8): bool;`
- `export fn gqlWriteByte(out: []u8, pos: &mut usize, b: u8);`
- `export fn gqlWriteStr(out: []u8, pos: &mut usize, s: string);`
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

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`graphql@1.0.0`)，许可证：MIT
