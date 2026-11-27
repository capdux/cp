[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# anstyle-parse

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 anstyle-parse

## 安装与导入

```bash
cap add anstyle-parse
```

```cp
import { nextToken } from "anstyle-parse";
```

## 核心导出 API

- `export fn nextToken(input: string, pos: usize): AnsiToken;`
- `export fn tokenizeAnsi(input: string, out: []AnsiToken): usize;`
- `export fn parseCsiParams(csi_seq: string): SgrParams;`
- `export fn stripAnsiToBuf(input: string, out: []u8): usize;`
- `export fn hasAnsi(input: string): bool;`
- `export fn visibleByteLen(input: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`anstyle-parse@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`anstream@1.0.0`)，许可证：MIT
