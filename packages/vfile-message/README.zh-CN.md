[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# vfile-message

> 版本：`4.0.2` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation structured lint/AST diagnostic message with unist Point/Position formatting

## 安装与导入

```bash
cap add vfile-message
```

```cp
import { makePoint } from "vfile-message";
```

## 核心导出 API

- `export enum Severity`
- `export fn makePoint(line: u32, column: u32, offset: u32): Point;`
- `export fn makePosition(start: Point, end: Point): Position;`
- `export fn pointAtOffset(text: string, offset: usize): Point;`
- `export fn createMessage(reason: string, file_path: string, source: string, rule_id: string, severity: Severity, at: Point): VFileMessage;`
- `export fn stringifyPosition(pos: Position, out: []u8): usize;`
- `export fn formatMessage(msg: VFileMessage, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`vfile-message@4.0.2`)（https://github.com/vfile/vfile-message），许可证：MIT
