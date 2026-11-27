[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# p-try

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap structured sync/async execution outcome wrapper preventing escaping errors, with safe integer parsing, division, and short-circuit chaining (clean-room port of npm p-try)

## 安装与导入

```bash
cap add p-try
```

```cp
import { ptryIsSpace } from "p-try";
```

## 核心导出 API

- `export fn ptryIsSpace(ch: u8): bool;`
- `export fn ptryIsDigit(ch: u8): bool;`
- `export fn ptryI64Min(): i64;`
- `export fn ptryAbsU64(v: i64): u64;`
- `export fn tryOkInt(v: i64): TryOutcome;`
- `export fn tryOkStr(s: string): TryOutcome;`
- `export fn tryErr(code: i32, msg: string): TryOutcome;`
- `export fn unwrapOrInt(o: TryOutcome, fallback: i64): i64;`
- `export fn unwrapOrStr(o: TryOutcome, fallback: string): string;`
- `export fn tryParseI64(s: string): TryOutcome;`
- `export fn tryDivI64(a: i64, b: i64): TryOutcome;`
- `export fn tryChain(first: TryOutcome, next_ok: bool, next_val: i64, next_err_code: i32, next_err_msg: string): TryOutcome;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`p-try@1.0.0`)，许可证：MIT
