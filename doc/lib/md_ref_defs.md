# `cap:md_ref_defs`

纯 CP 零堆分配 CommonMark §4.7 链接引用定义（Link Reference Definitions）解析与规范化查表库，吸收自 `/data/dev/bun/src/md/ref_defs.cp`。

## 导出符号

- `RefDef` / `ParseRefDefResult` / `ParseDestResult` / `ParseTitleResult`
- `normalizeLabel(raw, out)`：按 CommonMark §6.7 折叠空白与大小写折叠
- `labelsEqual(a, b)`：零分配规范化标签等价比较
- `parseRefDefDest(text, start)`：支持 `<...>` 与括号平衡裸 URL 目标
- `parseRefDefTitle(text, start)`：支持 `"..."`、`'.*'`、`(...)` 标题与空行拦截
- `parseRefDef(text, pos)`：完整解析 `[label]: dest "title"`（含跨行回溯语义）
- `RefDefTable`：首定义优先（first-definition-wins）查找表与连续批量解析 `parseAll`
