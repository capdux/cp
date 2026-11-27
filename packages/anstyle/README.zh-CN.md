[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# anstyle

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 anstyle

## 安装与导入

```bash
cap add anstyle
```

```cp
import { writeByte } from "anstyle";
```

## 核心导出 API

- `export fn writeByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn writeU8Dec(out: []u8, pos: usize, val: u8): usize;`
- `export fn writeParam(out: []u8, pos: usize, has_prev: bool, val: u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`anstyle@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`console@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`colored@1.0.0`)，许可证：MIT
