[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/db

Official unified database context and request-scoped automatic transaction plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/db` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/db
```

```cp
import {
    DbDialect,
    DbPluginConfig,
    DbTxStats,
    dbDefaultTxAction,
    dbRunTxAction,
    dbStrEql,
    dbIsWriteMethod,
    dbTxStats,
} from "@web/db";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `DbDialect` | `Sqlite, Postgres, Mysql` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DbPluginConfig` | `class` | 字段: `dialect: DbDialect, dsn: string, auto_tx: bool, tx_header: string` |
| `DbTxStats` | `class` | 字段: `begins: u64, commits: u64, rollbacks: u64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `dbDefaultTxAction` | `(action: u8): bool` | 调用 `dbDefaultTxAction` 并返回 `bool`。 |
| `dbRunTxAction` | `(action: u8): bool` | 调用 `dbRunTxAction` 并返回 `bool`。 |
| `dbStrEql` | `(a: string, b: string): bool` | 调用 `dbStrEql` 并返回 `bool`。 |
| `dbIsWriteMethod` | `(m: AppMethod): bool` | 调用 `dbIsWriteMethod` 并返回 `bool`。 |
| `dbTxStats` | `(): DbTxStats` | 调用 `dbTxStats` 并返回 `DbTxStats`。 |
| `dbResetStats` | `()` | 就地执行 `dbResetStats` 操作。 |
| `dbBeforeHandle` | `(ctx: &mut Context): bool` | 调用 `dbBeforeHandle` 并返回 `bool`。 |
| `dbAfterHandle` | `(ctx: &mut Context, resp: &mut Response)` | 就地执行 `dbAfterHandle` 操作。 |
| `createDbPlugin` | `(cfg: DbPluginConfig, action_fn: DbTxActionFn): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerDbWithHook` | `(app: &mut App, cfg: DbPluginConfig, action_fn: DbTxActionFn): &mut App` | 将插件或中间件注册到目标应用实例。 |
| `registerDb` | `(app: &mut App, cfg: DbPluginConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { dbStrEql } from "@web/db";

pub fn main() {
    let result = dbStrEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
