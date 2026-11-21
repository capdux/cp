# `cap:clap_stream`

纯 CP 零堆分配流式命令行参数解析器，吸收自 `/data/dev/bun/src/clap/streaming.cp`。

## 导出类型

- `TakesValue`：`none | one | one_optional`
- `ParamSpec`：参数规格 `{ id, short, long, takes_value, is_positional }`
- `ParseError`：`none | doesnt_take_value | missing_value | unknown_flag | unexpected_positional`
- `ParsedArg`：`{ id, value, err, raw }` + `isOk()`
- `StreamingClap`：支持 `--long`、`--long=val`、`--long val`、短选项串联 `-abc`、`-o=val`、`-oval`、`-o val`、`--` 后的纯位置参数与可选 `ignore_unknown`
