[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# shebang-command

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation shebang executable command extractor reusing @cap/shebang-regex for CP

## 安装与导入

```bash
cap add shebang-command
```

```cp
import { skipSpaces } from "shebang-command";
```

## 核心导出 API

- `export fn skipSpaces(s: string, pos: usize, end: usize): usize;`
- `export fn scanTokenEnd(s: string, pos: usize, end: usize): usize;`
- `export fn trimRight(s: string, start: usize, end: usize): usize;`
- `export fn basenameStart(s: string, start: usize, end: usize): usize;`
- `export fn locateParts(input: string): Parts;`
- `export fn shebangCommand(input: string, out_cmd: []u8): usize;`
- `export fn parseShebangCmd(input: string, out_bin: []u8, out_arg: []u8): ShebangCmd;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`shebang-command@1.0.0`)，许可证：MIT
