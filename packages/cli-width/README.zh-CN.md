[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cli-width

纯 CP 实现的零分配终端列宽解析器，完整支持 TTY 流、`COLUMNS` 环境变量解析、默认值回退及 `[min_width, max_width]` 区间钳制。

- **源包对齐**：对标 `npm` `cli-width`（ISC）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，全部采用值类型结构体与只读字符串切片。

## 安装与导入

```bash
cap add cli-width
```

```cp
import {
    CliWidthOptions,
    TtyStreamInfo,
    CliWidthConfig,
    defaultCliWidth,
    clampCliWidth,
    parseColumnsEnv,
    parseColumnsFromEnvList,
    resolveCliWidth,
    cliWidth,
} from "cli-width";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `CliWidthOptions` | `class CliWidthOptions { default_width: u32, min_width: u32, max_width: u32 }` | `resolveCliWidth` 选项；`CliWidthOptions.defaults()` 默认 `default_width: 80`、`min_width: 0`、`max_width: 0`。 |
| `TtyStreamInfo` | `class TtyStreamInfo { is_tty: bool, columns: u32, rows: u32 }` | TTY 流信息；构造器 `TtyStreamInfo.none()`、`TtyStreamInfo.tty(columns, rows)`。 |
| `CliWidthConfig` | `class CliWidthConfig { default_width, output, tty, env_columns, min_width, max_width }` | 完整配置类；`CliWidthConfig.defaults()` 默认宽度 `80`、不启用上下限钳制。 |
| `defaultCliWidth` | `fn defaultCliWidth(): u32` | 返回默认回退列宽（`80`）。 |
| `clampCliWidth` | `fn clampCliWidth(width: u32, min_width: u32, max_width: u32): u32` | 将 `width` 钳制到 `[min_width, max_width]`；某侧边界为 `0` 表示不限制该侧。 |
| `parseColumnsEnv` | `fn parseColumnsEnv(env_val: string): u32?` | 解析正整数 `COLUMNS` 值（如 `"120"`）；为空、非数字或 `0` 时返回 `null`。 |
| `parseColumnsFromEnvList` | `fn parseColumnsFromEnvList(env_entries: []const string): u32?` | 从 `"KEY=VALUE"` 环境变量列表中查找并解析 `COLUMNS=...`。 |
| `resolveCliWidth` | `fn resolveCliWidth(tty_cols: u32, env_columns: string, opts: CliWidthOptions): u32` | 按优先级解析：`tty_cols > 0` > `COLUMNS` > `opts.default_width`（回退 `80`），最后钳制。 |
| `cliWidth` | `fn cliWidth(cfg: CliWidthConfig): u32` | 完整 npm 优先级：`cfg.output` TTY > `cfg.tty` TTY > `cfg.env_columns` > `cfg.default_width`（回退 `80`），最后钳制。 |

## 使用示例

### 1. 基础解析与上下限钳制

```cp
import { CliWidthOptions, resolveCliWidth } from "cli-width";

fn simpleWidth(): u32 {
    let mut opts = CliWidthOptions.defaults();
    opts.min_width = 40;
    opts.max_width = 120;
    return resolveCliWidth(0, "160", opts); // 120（被 max_width 钳制）
}
```

### 2. 使用 `CliWidthConfig` 多源优先级解析

```cp
import { CliWidthConfig, TtyStreamInfo, cliWidth } from "cli-width";

fn fullConfigWidth(): u32 {
    let mut cfg = CliWidthConfig.defaults();
    cfg.output = TtyStreamInfo.tty(100, 30);
    cfg.env_columns = "80";
    return cliWidth(cfg); // 100（output TTY 优先于 COLUMNS）
}
```

### 3. 从环境变量列表中提取 `COLUMNS`

```cp
import { parseColumnsFromEnvList } from "cli-width";

fn fromEnv(): u32 {
    const envs: [2]string = ["TERM=xterm-256color", "COLUMNS=132"];
    return parseColumnsFromEnvList(envs[0..]) ?? 80; // 132
}
```

## 与源包差异与安全亮点

- **纯函数可测性**：不隐式读取全局 `process.stdout` / `process.env`，而是通过 `CliWidthConfig` 显式传入 TTY 信息与环境变量，便于确定性单测。
- **原生范围钳制**：内置 `min_width`/`max_width` 钳制（`0` 表示关闭该侧边界）。
- **零堆分配**：直接基于只读切片 `string` / `[]const string` 解析，无任何动态内存分配。
