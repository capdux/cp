[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cexpr

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation C constant expression parser and evaluator (full C operator precedence, hex/bin/oct/char literals) for CP

## 安装与导入

```bash
cap add cexpr
```

```cp
import { skipWs } from "cexpr";
```

## 核心导出 API

- `export fn skipWs(p: &mut Parser, src: string);`
- `export fn peek(p: &mut Parser, src: string): u8;`
- `export fn peek2(p: &mut Parser, src: string): u8;`
- `export fn absI64(x: i64): u64;`
- `export fn signedDivI64(a: i64, b: i64): i64;`
- `export fn signedRemI64(a: i64, b: i64): i64;`
- `export fn skipIntSuffix(p: &mut Parser, src: string);`
- `export fn parseCharLit(p: &mut Parser, src: string): i64;`
- `export fn parseIntLit(p: &mut Parser, src: string): i64;`
- `export fn parsePrimary(p: &mut Parser, src: string): i64;`
- `export fn parseUnary(p: &mut Parser, src: string): i64;`
- `export fn parseMul(p: &mut Parser, src: string): i64;`
- `export fn parseAdd(p: &mut Parser, src: string): i64;`
- `export fn parseShift(p: &mut Parser, src: string): i64;`
- `export fn parseRel(p: &mut Parser, src: string): i64;`
- `export fn parseEq(p: &mut Parser, src: string): i64;`
- `export fn parseBitAnd(p: &mut Parser, src: string): i64;`
- `export fn parseBitXor(p: &mut Parser, src: string): i64;`
- `export fn parseBitOr(p: &mut Parser, src: string): i64;`
- `export fn parseLogAnd(p: &mut Parser, src: string): i64;`
- `export fn parseLogOr(p: &mut Parser, src: string): i64;`
- `export fn parseTernary(p: &mut Parser, src: string): i64;`
- `export fn evalCExpr(expr: string): EvalResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`cexpr@1.0.0`)，许可证：MIT
