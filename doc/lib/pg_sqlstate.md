# cap:pg_sqlstate

> 源码吸收自 `/data/dev/bun/src/sql/postgres/AnyPostgresError.cp`，提供纯 CP、零外部依赖、零堆分配的 PostgreSQL 5 字符 SQLSTATE 错误码分类器与可重试判定库。

## 导入方式

```cp
import { SqlStateClass, SqlStateCode } from "cap:pg_sqlstate";
```

## 核心类型与 API

### `SqlStateClass`

按 SQLSTATE 前 2 字符划分的 15 种官方类别：
`success` (`00`)、`warning` (`01`)、`no_data` (`02`)、`connection_exception` (`08`)、`data_exception` (`22`)、`integrity_constraint_violation` (`23`)、`invalid_transaction_state` (`25`)、`invalid_authorization` (`28`)、`transaction_rollback` (`40`)、`syntax_or_access_error` (`42`)、`insufficient_resources` (`53`)、`operator_intervention` (`57`)、`system_error` (`58`)、`internal_error` (`XX`)、`other`。

- `prefix(): []const u8`：返回该类别对应的 2 字符前缀。

### `SqlStateCode`

| 方法 | 签名 | 说明 |
| --- | --- | --- |
| `parse` | `fn parse(code: []const u8): SqlStateCode?` | 严格校验 5 字符大写字母/数字 SQLSTATE 码 |
| `class` | `fn class(): SqlStateClass` | 返回前 2 字符对应的 SQLSTATE 类别 |
| `conditionName` | `fn conditionName(): ([]const u8)?` | 查询 43 个高频官方条件名（如 `unique_violation`、`deadlock_detected`） |
| `isRetryable` | `fn isRetryable(): bool` | 是否适合自动重试（`40001` / `40P01` / `08xxx` / `53300` / `57P01-03`） |
| `isConstraintViolation` | `fn isConstraintViolation(): bool` | 是否为完整性约束冲突（`23xxx`） |
| `isConnectionError` | `fn isConnectionError(): bool` | 是否为连接或停机类错误（`08xxx` / `57P01-03`） |
| `isSuccess` | `fn isSuccess(): bool` | 是否为成功完成类（`00xxx`） |
| `eqlStr` / `eql` | `fn eqlStr(s: []const u8): bool` | 精确比较 5 字节错误码 |
| `format` | `fn format(out: []u8): ([]const u8)?` | 零堆分配写入 5 字节码到调用方缓冲 |

## 使用示例

```cp
import { SqlStateCode, SqlStateClass } from "cap:pg_sqlstate";

fn handlePgError(raw_code: []const u8) {
    if (code := SqlStateCode.parse(raw_code)) {
        if (code.isRetryable()) {
            // 触发指数退避事务重试（如 40001 / 40P01 / 08006 / 53300 / 57P01）
        } else if (code.isConstraintViolation()) {
            // 映射为业务层 409 Conflict（如 23505 unique_violation）
            * = code.conditionName();
        }
        * = code.class() == SqlStateClass.syntax_or_access_error;
    }
}
```
