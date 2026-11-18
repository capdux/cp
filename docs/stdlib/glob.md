# `cap:glob` — 高性能文件通配符匹配与目录遍历引擎

`cap:glob` 是 CP 官方标准库中的文件通配匹配与多线程遍历引擎，完全吸收自 Bun 的 `GlobWalker.cp` 与 `matcher.cp`，提供极致毫秒级的模式匹配和递归文件扫描能力。

## 核心组件与特性

1. **工业级通配符语法匹配器 (`Matcher`)**：
   - 通配单字符 `?` 与通配任意序列 `*`；
   - 跨目录多级递归匹配 `**`；
   - 字符类范围 `[a-z]`、集合 `[abc]` 与取反 `[^0-9]`；
   - 嵌套花括号展开扩展 `{src,test}/**/*.{cp,zig}`；
   - 否定排除模式 `!*.tmp`。
2. **零分配高速目录遍历器 (`GlobWalker`)**：
   - 原生支持基于 libc `opendir`/`readdir` 的极速系统目录迭代；
   - 支持前缀提取加速（自动跳过无关目录树）；
   - 回调驱动扫描 `Glob.scan(pattern, root, callback)`。

---

## API 使用示例

### 1. 通配符模式匹配

```typescript
import { Glob, Matcher } from "cap:glob";

// 单级通配
assert(Glob.match("*.cp", "main.cp") == true);
assert(Glob.match("src/*.cp", "src/hello.cp") == true);

// 跨目录递归通配
assert(Glob.match("**/*.cp", "a/b/c/deep.cp") == true);

// 花括号扩展与组合
assert(Glob.match("src/**/*.{cp,zig}", "src/utils/math.zig") == true);

// 字符类与排除
assert(Glob.match("[a-z]*.log", "test.log") == true);
assert(Glob.match("!*.tmp", "data.json") == true);
assert(Glob.match("!*.tmp", "cache.tmp") == false);
```

### 2. 递归目录遍历 (`Glob.scan`)

```typescript
import { Glob } from "cap:glob";

// 扫描当前目录下所有 .cp 文件
Glob.scan("**/*.cp", ".", (path: []const u8) => {
    // 实时流式接收匹配的物理路径
    print(`Found source file: ${path}\n`);
});
```

---

## 溯源关系
本模块代码完全吸收自 Bun 生产源码：
- `lib/glob/matcher.cp`: `// Source: bun/src/glob/matcher.cp`
- `lib/glob/walker.cp`: `// Source: bun/src/glob/GlobWalker.cp`
- `lib/glob/index.cp`: `// Source: bun/src/glob/`
