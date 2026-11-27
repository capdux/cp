[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `commander`

CP 零堆分配命令行框架 —— 对齐 npm [`commander`](https://github.com/tj/commander.js)（v12）的纯 CP MIT 净室实现，底层组合 `yargs-parser`（参数解析）与 `cliui`（多列帮助排版）。

## 安装与导入

```bash
cap add commander
```

```cp
import {
    ARG_BOOL,
    ARG_INT,
    ARG_STRING,
    FlagSpec,
    OptionDef,
    ArgDef,
    SubcommandDef,
    CommandParseResult,
    Command,
    parseOptionFlags,
    parseArgSpec,
} from "commander";
```

## 核心 API

### 类型与解析辅助函数

| 符号 | 签名 / 说明 | 用途 |
| :--- | :--- | :--- |
| `ARG_BOOL` / `ARG_INT` / `ARG_STRING` | `u8` 常量（重导出自 `yargs-parser`） | 选项值类型标记 |
| `parseOptionFlags` | `(flags: string): FlagSpec` | 解析选项声明，如 `"-p, --port <num>"`、`"--no-color"`、`"-v, --verbose"` |
| `parseArgSpec` | `(spec: string, desc: string): ArgDef` | 解析位置参数声明，如 `"<input>"`、`"[output]"`、`"<files...>"` |
| `FlagSpec` / `OptionDef` / `ArgDef` / `SubcommandDef` | 值结构体 | 选项、位置参数与子命令的解析元数据 |

### `Command` 构建器方法

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `Command.new` | `(name: string): Command` | 创建命令对象（栈上固定容量：16 选项、16 子命令、8 位置参数） |
| `version` / `description` | `(*_, v: string)` / `(*_, d: string)` | 设置版本号（支持 `-V`/`--version`）与命令描述 |
| `option` | `(*_, flags: string, desc: string, default_str: string): bool` | 注册可选选项（无默认值传 `""`）；满容量返回 `false` |
| `requiredOption` | `(*_, flags: string, desc: string): bool` | 注册必填选项（缺失时解析失败） |
| `argument` | `(*_, spec: string, desc: string): bool` | 注册位置参数（`<必填>`、`[可选]`、`<变长...>`） |
| `subcommand` | `(*_, name: string, desc: string): bool` | 注册子命令 |
| `parse` | `(argv: []const string): CommandParseResult` | 解析参数切片并返回结构化结果 |
| `helpInformation` | `(width: usize, out: []u8): usize` | 将对齐排版后的帮助文本写入调用方缓冲区 `out`，返回写入字节数 |

### `CommandParseResult` 结果查询方法

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `hasSubcommand` / `subcommandName` | `(): bool` / `(out: []u8): usize` | 判断并拷贝匹配到的子命令名称 |
| `errorMessage` | `(out: []u8): usize` | 当 `!res.ok` 时拷贝错误诊断信息 |
| `hasOption` | `(name: string): bool` | 判断选项是否已传入或存在默认值 |
| `getBool` / `getInt` | `(name: string, default_val): bool / i64` | 读取布尔或整型选项值（带默认值回退） |
| `getString` | `(name: string, out: []u8): i64` | 拷贝字符串选项到 `out`，返回字节长度（未命中返回 `-1`） |
| `argCount` / `argAt` | `(): usize` / `(idx: usize, out: []u8): usize` | 获取位置参数个数并按索引拷贝 |

## 代码示例

### 1. 定义选项、子命令并解析参数

```cp
import { print } from "cap:io";
import { Command } from "commander";

fn main() {
    let cmd = Command.new("deploy");
    cmd.version("1.2.0");
    cmd.description("零停机部署工具");
    cmd.option("-p, --port <num>", "服务端口", "8080");
    cmd.option("-v, --verbose", "输出详细日志", "");
    cmd.requiredOption("-e, --env <name>", "目标环境");
    cmd.subcommand("start", "启动服务");
    cmd.subcommand("status", "检查健康状态");

    const argv: [5]string = ["start", "--env", "prod", "-p", "9000"];
    const res = cmd.parse(&argv);

    if (res.ok) {
        let sub_buf: [64]u8 = undefined;
        const sub_len = res.subcommandName(sub_buf[0..64]);
        let env_buf: [64]u8 = undefined;
        const env_len = res.getString("env", env_buf[0..64]);
        print("sub:", sub_buf[0..sub_len], "port:", res.getInt("port", 8080), "env_len:", env_len);
    }
}
```

### 2. 生成自动对齐的帮助文档

```cp
import { print } from "cap:io";
import { Command } from "commander";

fn printHelp(cmd: Command) {
    let help_buf: [2048]u8 = undefined;
    const n = cmd.helpInformation(80, help_buf[0..2048]);
    print(help_buf[0..n]);
}
```

## 与源包差异及性能亮点

- **零堆内存分配**：所有选项（上限 16）、子命令（上限 16）、位置参数定义（上限 8）、实参（上限 16）与错误缓冲区均内联在栈结构体中，原生 CLI 启动零开销。
- **无 `process.exit()` 副作用**：遇到 `--help`、`--version` 或非法参数时不强行退出进程，而是通过 `res.show_help`、`res.show_version` 与 `res.ok = false` 交由调用方控制，易于单元测试。
- **生态模块化组合**：参数词法解析复用 `yargs-parser`，多列终端对齐排版复用 `cliui`。
