# cap:npmrc_config

> 源码吸收自 `/data/dev/bun/src/install/npmrc.zig`（`.npmrc` 配置与作用域仓库路由解析器），提供纯 CP、零外部依赖、零堆分配的 `.npmrc` 键值流式解析、布尔配置查询、作用域仓库路由解析与仓库级配置项查找库。

## 导入方式

```cp
import {
    DEFAULT_NPM_REGISTRY,
    NpmrcEntry,
    NpmrcIter,
    getNpmrcValue,
    getNpmrcBool,
    resolveRegistryForScope,
    findRegistrySetting,
} from "cap:npmrc_config";
```

## 核心类型与 API

### 常量

- `DEFAULT_NPM_REGISTRY: []const u8 = "https://registry.npmjs.org/"`

### `NpmrcEntry`

```cp
pub struct NpmrcEntry {
    key: []const u8;
    value: []const u8;
    line_no: usize;
}
```

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `key` | `[]const u8` | 去首尾空白后的键名（零拷贝指向原文本） |
| `value` | `[]const u8` | 去首尾空白并剥离配对外层单/双引号后的值 |
| `line_no` | `usize` | 配置行号（从 `1` 开始） |

### `NpmrcIter`

| 方法 | 签名 | 说明 |
| --- | --- | --- |
| `init` | `fn init(content: []const u8): NpmrcIter` | 初始化零分配迭代器（自动跳过 UTF-8 BOM） |
| `next` | `fn next(*_): NpmrcEntry?` | 迭代下一条有效配置：跳过空行与 `#`/`;` 注释行，按首个 `=` 切分并剥离外层引号 |

### 查询与路由函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `getNpmrcValue` | `fn getNpmrcValue(content: []const u8, key: []const u8): ([]const u8)?` | 查询指定键的值（同名后者覆盖前者） |
| `getNpmrcBool` | `fn getNpmrcBool(content: []const u8, key: []const u8): bool?` | 大小写无关解析 `true`/`false` 布尔配置 |
| `resolveRegistryForScope` | `fn resolveRegistryForScope(content: []const u8, scope: ([]const u8)?): []const u8` | 优先查找 `@<scope>:registry`（兼容带或不带 `@` 前缀），再回退 `registry`，最后回退 `DEFAULT_NPM_REGISTRY` |
| `findRegistrySetting` | `fn findRegistrySetting(content: []const u8, registry_url: []const u8, suffix: []const u8): ([]const u8)?` | 剥离 `https://`/`http://` 协议前缀后匹配 `//<host_and_path>/:<suffix>`（兼容端口与有无尾斜杠），查询仓库级配置项 |

## 使用示例

```cp
import {
    resolveRegistryForScope,
    findRegistrySetting,
    getNpmrcBool,
} from "cap:npmrc_config";

fn example() {
    const cfg =
        "registry=https://registry.example.com/\n"
        ++ "@acme:registry=https://npm.acme.internal/pkg/\n"
        ++ "//npm.acme.internal/pkg/:always-auth=true\n"
        ++ "save-exact=true\n";

    * = resolveRegistryForScope(cfg, "@acme"); // "https://npm.acme.internal/pkg/"
    * = findRegistrySetting(cfg, "https://npm.acme.internal/pkg/", "always-auth"); // "true"
    * = getNpmrcBool(cfg, "save-exact"); // true
}
```
