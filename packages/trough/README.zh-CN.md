[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# trough

> 版本：`2.2.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap middleware pipeline engine and tag router in pure CP (covers unified & zwitch)

## 安装与导入

```bash
cap add trough
```

```cp
import { trghStrEq } from "trough";
```

## 核心导出 API

- `export fn trghStrEq(a: string, b: string): bool;`
- `export fn trghIdentity(v: i64): StageOutcome;`
- `export fn stageOk(value: i64): StageOutcome;`
- `export fn stageErr(err_code: i32): StageOutcome;`
- `export fn stageHalt(value: i64): StageOutcome;`
- `export fn dispatchTag(tag: string, routes: &[]TagRoute): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`trough@2.2.0`)（https://github.com/wooorm/trough），许可证：MIT
