# `cap:clap_stream`

> 参考来源：/data/dev/bun/src/clap/streaming.cp | 吸收日期：2026-11-22

纯 CP 零堆分配流式命令行参数解析器。

## 导出类型

- `TakesValue`：`none | one | one_optional`
- `ParamSpec`：参数规格 `{ id, short, long, takes_value, is_positional }`
- `ParseError`：`none | doesnt_take_value | missing_value | unknown_flag | unexpected_positional`
- `ParsedArg`：`{ id, value, err, raw }` + `isOk()`
- `StreamingClap`：支持 `--long`、`--long=val`、`--long val`、短选项串联 `-abc`、`-o=val`、`-oval`、`-o val`、`--` 后的纯位置参数与可选 `ignore_unknown`

## 上游行为契约与 CLI 解析边界

1. **短选项串联与黏连值消歧（源自 Bun `clap/streaming.cp`）**：
   - 若 `-a` 与 `-b` 为无值开关（`TakesValue.none`）而 `-o` 接受值（`TakesValue.one`），则 `-abofile.txt` 依次解析为 `-a`、`-b` 与 `-o "file.txt"`；`-` 单横杠作为位置参数（如标准输入占位符）透传。
2. **`--` 选项终止符契约**：
   - 遇到独立 `--` 后，后续所有参数（即使以 `-` 或 `--` 开头）一律作为位置参数产出，避免误将负数值或子命令透传参数当作当前命令选项。
3. **无堆分配错误报告**：
   - 遇到未知标志、缺少必选值或多余赋值（如 `--verbose=1` 传给 `TakesValue.none`）时，通过 `ParsedArg.err` 与 `ParsedArg.raw` 零拷贝返回错误上下文，`ignore_unknown = true` 适用于透明转发未知标志的包装 CLI。
