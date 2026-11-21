# cap:xdg_basedir

> 源码吸收自 `/data/dev/bun/src/env_loader.zig`（XDG Base Directory Spec 解析器），提供纯 CP、零外部依赖、零堆分配的 freedesktop.org XDG Base Directory Specification 路径推导器与系统目录列表迭代器。

## 导入方式

```cp
import {
    DEFAULT_CONFIG_DIRS,
    DEFAULT_DATA_DIRS,
    XdgEnv,
    XdgDirIter,
    resolveConfigHome,
    resolveDataHome,
    resolveCacheHome,
    resolveStateHome,
    resolveRuntimeDir,
} from "cap:xdg_basedir";
```

## 核心类型与 API

### `XdgEnv`

```cp
pub struct XdgEnv {
    home: ([]const u8)?;
    config_home: ([]const u8)?;
    data_home: ([]const u8)?;
    cache_home: ([]const u8)?;
    state_home: ([]const u8)?;
    runtime_dir: ([]const u8)?;
    config_dirs: ([]const u8)?;
    data_dirs: ([]const u8)?;

    fn empty(home: ([]const u8)?): XdgEnv;
}
```

### 路径推导函数

遵循 XDG Base Directory Specification：若 `XDG_*` 变量为空或非绝对路径（不以 `/` 开头），自动忽略并回退默认值；若 `app_name` 非空则拼接 `/<app_name>`（拦截 `..` 路径穿越与前导 `/`）。

| 函数 | 签名 | 默认回退 |
| --- | --- | --- |
| `resolveConfigHome` | `fn resolveConfigHome(env: XdgEnv, app_name: ([]const u8)?, out: []u8): ([]const u8)?` | `$HOME/.config[/<app>]` |
| `resolveDataHome` | `fn resolveDataHome(env: XdgEnv, app_name: ([]const u8)?, out: []u8): ([]const u8)?` | `$HOME/.local/share[/<app>]` |
| `resolveCacheHome` | `fn resolveCacheHome(env: XdgEnv, app_name: ([]const u8)?, out: []u8): ([]const u8)?` | `$HOME/.cache[/<app>]` |
| `resolveStateHome` | `fn resolveStateHome(env: XdgEnv, app_name: ([]const u8)?, out: []u8): ([]const u8)?` | `$HOME/.local/state[/<app>]` |
| `resolveRuntimeDir` | `fn resolveRuntimeDir(env: XdgEnv, app_name: ([]const u8)?, out: []u8): ([]const u8)?` | 无回退（仅 `XDG_RUNTIME_DIR` 为绝对路径时返回，否则 `null`） |

### `XdgDirIter`

| 方法 | 签名 | 说明 |
| --- | --- | --- |
| `init` | `fn init(dirs_env: ([]const u8)?, default_dirs: []const u8): XdgDirIter` | 初始化 `:` 分隔目录迭代器；当 `dirs_env` 为 `null`、空串或无任何合法绝对路径时回退 `default_dirs`（如 `DEFAULT_CONFIG_DIRS` / `DEFAULT_DATA_DIRS`） |
| `next` | `fn next(*_): ([]const u8)?` | 迭代下一个合法绝对路径，自动跳过空段与相对路径 |

## 使用示例

```cp
import {
    XdgEnv,
    XdgDirIter,
    DEFAULT_DATA_DIRS,
    resolveConfigHome,
} from "cap:xdg_basedir";

fn example() {
    const env = XdgEnv.empty("/home/alice");
    let buf: [256]u8 = undefined;

    if (cfg := resolveConfigHome(env, "cp", &buf)) {
        * = cfg; // "/home/alice/.config/cp"
    }

    let it = XdgDirIter.init(env.data_dirs, DEFAULT_DATA_DIRS);
    while (dir := it.next()) {
        * = dir; // "/usr/local/share", "/usr/share"
    }
}
```
