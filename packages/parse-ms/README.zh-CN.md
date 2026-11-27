[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# parse-ms

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 parse-ms

## 安装与导入

```bash
cap add parse-ms
```

```cp
import { signedPart } from "parse-ms";
```

## 核心导出 API

- `export fn signedPart(mag: u64, is_neg: bool): i64;`
- `export fn splitWholeMs(whole_ms: u64, us: u64, ns: u64, is_neg: bool): ParsedMs;`
- `export fn parseMs(ms: i64): ParsedMs;`
- `export fn parseMsF64(ms: f64): ParsedMs;`
- `export fn toMilliseconds(p: ParsedMs): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`parse-ms@1.0.0`)，许可证：MIT
