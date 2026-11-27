[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pretty-ms

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation millisecond decomposer and human-readable duration formatter composing @cap/parse-ms for CP

## 安装与导入

```bash
cap add pretty-ms
```

```cp
import { prettyMs } from "pretty-ms";
```

## 核心导出 API

- `export fn prettyMs(ms: i64, out: []u8): string;`
- `export fn prettyMsWith(ms: i64, opts: PrettyMsOptions, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pretty-ms@1.0.0`)，许可证：MIT
