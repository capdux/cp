[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cjs-module-lexer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CommonJS static named export and re-export lexer (npm cjs-module-lexer parity) for CP

## 安装与导入

```bash
cap add cjs-module-lexer
```

```cp
import { cjslIsWs } from "cjs-module-lexer";
```

## 核心导出 API

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

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cjs-module-lexer@1.0.0`)，许可证：MIT
