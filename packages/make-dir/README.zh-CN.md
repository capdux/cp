[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `make-dir`

> **源包溯源**：`npm:make-dir` ([sindresorhus/make-dir](https://github.com/sindresorhus/make-dir)) — 协议：MIT

纯 CP 实现的零堆分配递归目录创建规划器（`mkdir -p`），提供 POSIX/Windows 非法字符结构化校验、路径规范化、八进制 `umask` 权限计算及父目录链迭代器。

## 零成本迁移对照（TS → CP）

直接沿用原包名 `make-dir`，无需改名：

```typescript
// TypeScript / Node.js (npm: make-dir)
import { makeDirectorySync } from "make-dir";
makeDirectorySync("/var/data/cache/v1", { mode: 0o755 });
```

```cp
// CP（零堆分配，生成确定性有序创建计划）
import { MakeDirOptions, planMakeDir, applyUmask } from "make-dir";

let existing: [2]string = ["/var", "/var/data"];
let opts = MakeDirOptions.defaults();
opts.mode = applyUmask(0o777, 0o022); // 0o755

let plan = planMakeDir("/var/data/cache/v1", existing[0..2], opts);
// plan.valid == true, plan.count == 2
// plan.dirs[0] == "/var/data/cache", plan.dirs[1] == "/var/data/cache/v1"
```

## 安装配置

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "make-dir": "1.0.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `MakeDirOptions` | `class { mode: u32, is_windows: bool, fn defaults(): MakeDirOptions }` | 权限模式（默认 `0o777` / `511`）与 Windows 校验开关 |
| `MakeDirPlan` | `class { valid: bool, mode: u32, dirs: [32]string, count: usize, fn empty(valid: bool, mode: u32): MakeDirPlan }` | 待创建的缺失目录前缀有序列表（最多 32 层） |
| `DirPathValidation` | `class { valid: bool, err_code: i32, bad_index: usize }` | 结构化校验结果（`0`=合法，`1`=空，`2`=含 NUL，`3`=非法字符，`4`=非法冒号） |
| `ParentDirIter` | `class { path: string, cursor: usize, done: bool, fn next(_: auto): string? }` | 零分配父目录前缀迭代器 |
| `planMakeDir` / `makeDirPlan` | `(target: string, existing_dirs: []string, opts: MakeDirOptions): MakeDirPlan` | 按父到子顺序计算需要创建的目录前缀 |
| `isValidDirPath` | `(path: string, is_windows: bool): bool` | 快速检查路径是否含 NUL 或 Windows 禁用字符 |
| `validateDirPath` | `(path: string, is_windows: bool): DirPathValidation` | 返回详细 `err_code` 与出错字节下标 `bad_index` |
| `normalizeDirPath` | `(path: string, is_windows: bool, out: []u8): usize` | 规范化分隔符、折叠连续 `/` 并去尾斜杠（保留 `/` 与 `C:/`） |
| `applyUmask` | `(mode: u32, umask: u32): u32` | 计算有效八进制权限 `mode & ~umask` |
| `iterParentDirs` | `(path: string): ParentDirIter` | 创建父目录前缀迭代器 |
| `collectParentDirs` | `(path: string, out: []string): usize` | 一次性收集全部父目录前缀到 `out` |
| `mkdStrEq` / `mkdExists` / `mkdIsAlpha` | 底层辅助函数 | 字符串比较与 ASCII 字母判断 |

## 实战示例

### 1. 路径校验、规范化与创建计划生成

```cp
import {
    MakeDirOptions,
    validateDirPath,
    normalizeDirPath,
    makeDirPlan,
    applyUmask,
} from "make-dir";

let check = validateDirPath("C:\\work\\bad*dir", true);
// check.valid == false, check.err_code == 3, check.bad_index == 11

let norm_buf: [128]u8 = undefined;
let n = normalizeDirPath("C:\\work\\\\out\\bin\\", true, norm_buf[0..128]);
let clean = norm_buf[0..n] as string; // "C:/work/out/bin"

let existing: [1]string = ["C:/work"];
let opts = MakeDirOptions.defaults();
opts.mode = applyUmask(0o777, 0o022);
let plan = makeDirPlan(clean, existing[0..1], opts);
// plan.dirs[0] == "C:/work/out", plan.dirs[1] == "C:/work/out/bin"
```

### 2. 遍历与收集父目录链

```cp
import { iterParentDirs, collectParentDirs } from "make-dir";

let it = iterParentDirs("/a/b/c/d");
while (true) {
    let p = it.next();
    if (p == null) { break; }
    // 依次产出 "/a", "/a/b", "/a/b/c"
}

let parents: [8]string = [""; 8];
let count = collectParentDirs("/a/b/c/d", parents[0..8]); // 3
```

## 注意事项与边界说明

- **最大层级**：`MakeDirPlan.dirs` 固定容量为 32 层，零堆分配。
- **Windows 校验规则**：禁止 `< > " | ? *`、NUL 字节，且冒号 `:` 仅允许出现在盘符下标 `1` 处（如 `C:\...`）。
- **纯计算无副作用**：只规划缺失目录列表，调用方可安全对接系统调用或执行 dry-run 预览。
