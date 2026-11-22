# `cap:js` — JavaScript 词法标识符与运算符元信息标准库

> 参考来源：/data/dev/bun/src/js_parser/ast/Op.cp, /data/dev/bun/src/js_parser/lexer/ | 吸收日期：2026-11-22

## 概述

`cap:js` 吸收自 Bun JS/TS 词法分析器与 AST 运算符元信息表（`src/js_parser/ast/Op.cp` 与 `src/js_parser/lexer/`），统一聚合 ECMAScript 2024 / TypeScript 关键字分类、保留字判定、ASCII 标识符与私有字段（`#field`）校验、对象属性引号判定、JS 字符串转义，以及完整运算符优先级、结合性与赋值目标语义查询。

## 上游语法契约与分类体系

- **四级关键字分类（`KeywordKind`）**：
  - `reserved`：无条件 ECMAScript 保留关键字（如 `class`、`return`、`function`、`await`、`import`、`export` 等）；
  - `strict_reserved`：严格模式保留字（如 `implements`、`interface`、`package`、`private`、`protected`、`public`、`static`）；
  - `contextual`：上下文关键字（如 `async`、`from`、`of`、`type`、`declare` 等）；
  - `none`：普通标识符（长度 `< 2` 或 `> 10` 直接 $O(1)$ 短路返回 `.none`）。
- **运算符元信息表（吸收自 Bun `Op.cp`）**：
  - 完整覆盖前缀一元（`un_pos`/`un_neg`/`un_cpl`/`un_not`/`un_void`/`un_typeof`/`un_delete`）、前后缀自增自减（`un_pre_dec`/`un_pre_inc`/`un_post_dec`/`un_post_inc`）、左结合二元（`bin_add`..`bin_bitwise_xor`）、非结合逗号（`bin_comma`）以及右结合复合赋值（`bin_assign`、`bin_add_assign` 等）；
  - 通过 `AssignTarget`（`none` / `replace` / `update`）精准区分普通表达式、纯替换赋值（`=`）与读-改-写复合赋值（`+=`、`++`），供 AST 压缩器与静态分析器判断左值读写语义。

## 目录结构

- `lib/js/ident.cp`：JS/TS 关键字分类、保留字判定、ASCII 标识符与私有字段校验、属性引号判定及字符串转义
- `lib/js/op.cp`：JS AST 运算符枚举、优先级层级、赋值目标类型及元信息表
- `lib/js/useDirective.cp`：JS/RSC 顶层指令序言（`"use client"` / `"use server"` / `"use strict"`）解析器与边界切换判定（吸收自 `/data/dev/bun/src/js_parser/ast/UseDirective.cp`）
- `lib/js/foldString.cp`：JS/TS 编译期字符串与模板字面量加法常量折叠引擎（吸收自 `/data/dev/bun/src/js_parser/ast/foldStringAddition.cp`）
- `lib/js/index.cp`：`cap:js` 统一聚合导出入口

## 核心 API

### 标识符与关键字 (`ident.cp`)

- `KeywordKind`：`none` / `reserved` / `strict_reserved` / `contextual`
- `classifyKeyword(s: []const u8): KeywordKind`
- `isReservedWord(s: []const u8): bool`
- `isValidAsciiIdent(s: []const u8): bool`
- `isValidPrivateIdent(s: []const u8): bool`
- `needsPropertyQuoting(s: []const u8): bool` — 判断对象字面量键名是否需要加引号（如含连字符或以数字开头）
- `escapeJsString(alloc, s, quote)` — 按指定引号（`'` 或 `"`）转义 JS 字符串（含换行、反斜杠、控制字符）

### 运算符元信息 (`op.cp`)

- `AssignTarget`：`none` / `replace` / `update`
- `Code`：完整 JS/TS 运算符枚举（如 `Code.bin_add`、`Code.bin_nullish_coalescing`、`Code.bin_assign`）
- `Level`：运算符优先级层级枚举
- `OpInfo`：包含运算符文本、优先级 `Level`、结合性与 `AssignTarget`
- `opInfo(code: Code): OpInfo` — $O(1)$ 查表返回运算符元信息

## 边界注意事项

1. **ASCII 标识符范围**：`isValidAsciiIdent` 面向高频 ASCII 快速路径（首字符 `[a-zA-Z_$]`，后续 `[a-zA-Z0-9_$]`），不包含完整 Unicode ID_Start/ID_Continue 表。
2. **私有标识符格式**：`isValidPrivateIdent` 要求以 `#` 开头且后续为合法标识符（如 `#secret`），单独 `"#"` 返回 `false`。

## 使用示例

```cp
import { isReservedWord, isValidAsciiIdent, Code, opInfo } from "cap:js";

fn demo() {
    const reserved = isReservedWord("class");     // true
    const valid = isValidAsciiIdent("foo_bar");   // true
    const info = opInfo(Code.bin_add);
    * = reserved;
    * = valid;
    * = info;
}
```

### 顶层指令序言与 RSC 边界判定 (`useDirective.cp`)

吸收自 `/data/dev/bun/src/js_parser/ast/UseDirective.cp`（参考 `/data/dev/bun/docs/bundler/index.mdx` 与 `/data/dev/bun/docs/snippets/cli/build.mdx` 中 `--banner '"use client";'` 打包器实战指南）：

- **导出符号**：`UseDirectiveBoundering`（`Client`/`Server`）、`UseDirectiveFlags`（`record`/`hasAnyBoundary`）、`UseDirective`（`None`/`Client`/`Server`/`Strict`；方法 `isBoundary`/`boundering`/`asLiteral`/`asBanner`/`parse`）、`scanUseDirectives(contents)`；
- **打包器实战最佳实践**：
  - React Server Components 模块可通过文件顶部 `"use client";` 或 `"use server";` 声明执行边界；当打包输出需要保留顶层指令时，可配合 `UseDirective.Client.asBanner()`（`'"use client";'`）注入 `--banner`；
  - `UseDirective.parse` 自动跳过前导 UTF-8 BOM、空白、分号与 `//` / `/* ... */` 注释，区分非指令代码（`.None`）与未知引号指令（`null`）；`scanUseDirectives` 可扫描连续指令序言（如 `"use strict";\n"use client";`）。

```cp
import { UseDirective, scanUseDirectives } from "cap:js";

const dir = UseDirective.parse("\"use client\";\nexport default function Btn() {}");
const crosses = UseDirective.Server.isBoundary(.Client); // true
const flags = scanUseDirectives("\"use strict\";\n'use client';");
* = dir; * = crosses; * = flags;
```

### 编译期字符串与模板字面量加法折叠 (`foldString.cp`) 与 Bun Minifier 指南

吸收自 `/data/dev/bun/src/js_parser/ast/foldStringAddition.cp`，并同步 `/data/dev/bun/docs/bundler/minifier.mdx` 中的 `--minify-syntax` 常量折叠实战最佳实践：

- **导出符号**：`FOLD_MAX_STR_LEN` (512)、`FOLD_MAX_PART_TAIL_LEN` (128)、`FOLD_MAX_TEMPLATE_PARTS` (16)、`FoldStringKind`（`Normal`/`NestedLeft`）、`JsPrimitiveType`、`JsFoldExprKind`、`JsTemplatePart`、`JsFoldExpr`、`formatJsInt`、`foldStringAddition`；
- **关键编译优化规则与安全守卫**：
  1. **字符串拼接与无副作用原始值强转**：`"a" + "b" -> "ab"`、`"x" + 123 -> "x123"`、`true + "!" -> "true!"`、`null + "x" -> "nullx"`、`undefined + "x" -> "undefinedx"`；
  2. **左结合嵌套保护 (`FoldStringKind.NestedLeft`)**：在 `(a + 1) + "y"` 中，绝不将右侧 `1 + "y"` 误折叠为 `"1y"`（因 `a` 为数字时 `a + 1` 执行算术加法），但 `(a + "x") + "y" -> a + "xy"` 安全可折叠；
  3. **模板字面量合并与降级**：支持 `"bar" + \`baz${x}\``、`` `foo${x}` + 0 ``、`` `foo${x}` + `a${y}b` `` 零分配合并，以及无插值无标签模板降级为普通字符串（`simplifyTemplateToString()`）；**带标签模板（`tag\`...\``）严禁折叠或降级**；
  4. **空字符串恒等消除**：仅当动态操作数的 `knownPrimitive()` 确定为 `.String` 时，才允许将 `"" + x` 或 `x + ""` 化简为 `x`（避免丢失对数字/对象的 `ToString` 副作用）；
  5. **Bun Minifier 粒度控制实战**：在生产构建中可按需组合 `--minify-whitespace`（去空白）、`--minify-syntax`（常量折叠/死代码消除/模板简化）与 `--minify-identifiers`（局部变量重命名），在减小包体积的同时减少打印器输出开销、提升整体打包速度。

```cp
import { JsFoldExpr, FoldStringKind, foldStringAddition } from "cap:js";

const lhs = JsFoldExpr.stringLit("v") ?? JsFoldExpr.undefinedLit();
if (folded := foldStringAddition(lhs, JsFoldExpr.numberInt(2), FoldStringKind.Normal)) {
    const out = folded.strSlice(); // "v2"
    * = out;
}
```

### JS/TS 词法作用域树与符号合并裁决 (`scope.cp`) 与直接 `eval()` 重命名防护

吸收自 `/data/dev/bun/src/js_parser/ast/Scope.cp`，并同步 `/data/dev/bun/docs/bundler/minifier.mdx` 中的 `--minify-identifiers` 安全重命名实践：

- **导出符号**：`JS_SCOPE_MAX_SCOPES` (`64`)、`JS_SCOPE_MAX_MEMBERS` (`256`)、`JS_SCOPE_NO_PARENT`、`JsScopeError`、`JsScopeKind`（`stopsHoisting` / `asStr`）、`JsStrictModeKind`（`isStrict`）、`JsSymbolKind`（`isHoisted` / `isHoistedOrFunction`）、`JsSymbolMergeResult`、`canMergeJsSymbols`、`JsScopeMember`、`JsScopeNode`、`JsScopeTree`（`init`/`pushScope`/`getScope`/`declareSymbol`/`lookupInScope`/`resolveSymbol`/`findHoistTargetScope`/`markDirectEval`/`canMinifyIdentifiersInScope`/`recursiveSetStrictMode`）；
- **核心语义与 Bun Minifier 实战要点**：
  1. **ECMAScript + TypeScript 符号合并裁决**：完整支持 TS `namespace` / `enum` / `function` / `class` 声明合并、私有字段 `get #x` + `set #x` 配对升级、`catch (e) { var e }` 与 `arguments` 遮蔽规则，静态拦截非法重声明（`MergeForbidden`）；
  2. **`var` 提升边界与严格模式传播**：`JsScopeKind.stopsHoisting()` 界定 `entry` / `function_args` / `function_body` / `class_static_init` 提升边界；`class_body` 自动开启 `implicit_strict_mode_class` 与 `forbid_arguments`，`recursiveSetStrictMode` 零递归向所有 `sloppy_mode` 后代传播严格模式；
  3. **Direct `eval()` 标识符保护**：若某作用域包含直接 `eval()` 调用，调用 `markDirectEval(scope_id)` 会沿父链标记整条可达作用域链，`canMinifyIdentifiersInScope` 返回 `false`，防止 minifier 误重命名可能被 `eval` 动态引用的局部变量。

```cp
import { JsScopeTree, JsScopeKind, JsSymbolKind } from "cap:js";

let tree = JsScopeTree.init(true); // TypeScript mode
const entry = tree.pushScope(JsScopeKind.entry, null)?;
const blk = tree.pushScope(JsScopeKind.block, entry)?;
* = tree.declareSymbol(entry, "Foo", JsSymbolKind.ts_enum, 1, 0)?;
* = tree.declareSymbol(entry, "Foo", JsSymbolKind.ts_namespace, 2, 16)?; // keep_existing
* = tree.markDirectEval(blk)?;
assert(!tree.canMinifyIdentifiersInScope(entry));
```

### 跨模块符号链接表与并查集压缩 (`symbolTable.cp`)

吸收自 `/data/dev/bun/src/js_parser/ast/Symbol.cp`，并结合 `/data/dev/bun/docs/bundler/minifier.mdx` 中的标识符压缩分槽实践：

- **导出符号**：`JS_SYMBOL_MAX_ENTRIES` (`256`)、`JS_SYMBOL_INVALID_INDEX`、`JsSymbolTableError`、`JsSlotNamespace`（`must_not_be_renamed`/`default_ns`/`label_ns`/`private_name`/`mangled_prop`）、`JsImportItemStatus`、`JsSymbolEntryKind`（24 种符号分类，含 `isPrivate`/`isHoisted`/`isHoistedOrFunction`/`isFunction`）、`JsSymbolRef`、`JsSymbolRecord`、`JsSymbolTable`（`init`/`len`/`addSymbol`/`get`/`follow`/`getResolved`/`merge`/`followAll`/`assignChunkIndex`/`addUseCount`/`markMustNotBeRenamed`/`countCanonicalByNamespace`）；
- **核心链接器特性**：
  1. **迭代路径压缩并查集 (`merge` / `follow` / `followAll`)**：跨模块 import/export 符号合并时零递归完成根追踪与路径压缩，同步聚合 `use_count_estimate` 并保持 `must_not_be_renamed` 与 `original_name` 锁定不变量；
  2. **5 槽位重命名命名空间**：将 `unbound`/`must_not_be_renamed`、普通标识符、`label`、私有成员 `#name` 与属性混淆分桶隔离，配合 `countCanonicalByNamespace()` 统计待重命名活跃符号数，供 minifier 按使用频次分配最短标识符。

```cp
import { JsSymbolTable, JsSymbolEntryKind, JsSlotNamespace } from "cap:js";

let tbl = JsSymbolTable.init();
const imp = tbl.addSymbol(0, "helper", JsSymbolEntryKind.import_sym)?;
const exp = tbl.addSymbol(1, "helperImpl", JsSymbolEntryKind.hoisted_function)?;
* = tbl.addUseCount(imp, 5)?;
* = tbl.addUseCount(exp, 3)?;
const root = tbl.merge(imp, exp)?;
if (rec := tbl.getResolved(imp)) {
    assert(rec.use_count_estimate == 8);
    assert(rec.slotNamespace() == JsSlotNamespace.default_ns);
}
* = root;
```

### TypeScript `namespace` / `enum` 兄弟块成员合并与隐式访问代理 (`tsNamespace.cp`)

吸收自 `/data/dev/bun/src/js_parser/ast/TS.cp`：

- **导出符号**：`TS_NS_MAX_SCOPES` (`32`)、`TS_NS_MAX_MEMBERS` (`128`)、`TS_NS_MAX_PROXIES` (`128`)、`TsNamespaceError`、`TsNamespaceMemberKind`（`property`/`nested_namespace`/`enum_number`/`enum_string`/`enum_property`，含 `isEnum`）、`TsNamespaceMember`、`TsNamespaceBlock`、`TsPropertyAccessProxy`、`TsNamespaceRegistry`（`init`/`createGroup`/`openBlock`/`exportProperty`/`exportNestedNamespace`/`exportEnumNumber`/`exportEnumString`/`exportEnumProperty`/`lookupMember`/`lookupImplicitInBlock`/`getOrCreatePropertyProxy`/`formatPropertyAccess`）；
- **关键 TypeScript 语义守卫**：
  1. **跨块共享导出表**：多个同名 `namespace Foo {}` 或 `enum Foo {}` 共享同一 `group_id`，前一个块导出的成员可被后一个兄弟块直接解析；
  2. **enum/namespace 隐式查找隔离**：`lookupImplicitInBlock` 严格执行 `block.is_enum_scope == member.isEnum()`，仅允许 `enum -> enum` 与 `namespace -> namespace` 隐式跨块引用，阻断跨界泄漏；
  3. **每块独立 IIFE 属性代理**：降级为 `(function(ns3) { ns3.z = ns3.y; })(ns || (ns = {}))` 时，`getOrCreatePropertyProxy` 为每个块懒缓存独立的代理符号 ref，`formatPropertyAccess` 零分配格式化 `"ns3.y"`。

```cp
import { TsNamespaceRegistry } from "cap:js";

let reg = TsNamespaceRegistry.init();
const gid = reg.createGroup();
const b1 = reg.openBlock(gid, 10, "ns2", false)?;
const b2 = reg.openBlock(gid, 11, "ns3", false)?;
* = reg.exportProperty(gid, "y", 12)?;
if (m := reg.lookupImplicitInBlock(b2, "y")) {
    let buf: [32]u8 = [0; 32];
    const n = reg.formatPropertyAccess(b2, m.name, buf[0..])?; // "ns3.y"
    * = n;
}
* = b1;
```

### TypeScript 装饰器元数据归约与泛型实参前瞻 (`tsMetadata.cp`)

吸收自 `/data/dev/bun/src/js_parser/ast/TypeScript.cp`：

- **导出符号**：`TsMetadataError`、`TsMetadataKind`（17 种 `emitDecoratorMetadata` 元数据类型）、`TsDecoratorMetadata`（`fromKind`/`identifier`/`eql`/`finishUnion`/`mergeUnion`/`finishIntersection`/`mergeIntersection`/`foldUnion`/`foldIntersection`/`runtimeRefStr`/`emitRuntimeRef`）、`TsTypeArgLookaheadToken`、`canFollowTsTypeArguments`；
- **关键 TypeScript 语义对齐**：
  1. **联合类型归约 (`foldUnion`)**：`string | null | undefined -> String`、`string | number -> Object`、`MyClass | MyClass -> MyClass`、`MyClass | OtherClass -> Object`、`any | string -> Object`；
  2. **交叉类型归约 (`foldIntersection`)**：`never & string -> void 0`（`never` 主导）、`unknown & unknown -> void 0`（全 `unknown` 交集不退化为 `Object`）、`string & unknown -> String`、`MyClass & OtherClass -> Object`；
  3. **表达式泛型实参前瞻 (`canFollowTsTypeArguments`)**：精确区分 `foo<T>(x)` / ``foo<T>`tpl` `` 与比较运算 `a < b > c` / `a < b + c`。

```cp
import { TsDecoratorMetadata, TsMetadataKind, TsTypeArgLookaheadToken, canFollowTsTypeArguments } from "cap:js";

const branches: [3]TsDecoratorMetadata = [
    TsDecoratorMetadata.fromKind(TsMetadataKind.m_string),
    TsDecoratorMetadata.fromKind(TsMetadataKind.m_null),
    TsDecoratorMetadata.fromKind(TsMetadataKind.m_undefined),
];
const meta = TsDecoratorMetadata.foldUnion(branches[0..]);
// meta.runtimeRefStr() == "String"
assert(canFollowTsTypeArguments(TsTypeArgLookaheadToken.open_paren, false));
assert(!canFollowTsTypeArguments(TsTypeArgLookaheadToken.plus, false));
* = meta;
```

### JS/TS 词法 Token 分类表与 JSX 实体解码 (`lexerTables.cp`)

吸收自 `/data/dev/bun/src/js_parser/lexer_tables.cp`：

- **导出符号**：`JsToken`（完整 107 项 JS/TS 词法 Token 枚举，含 `isAssign`/`isReservedWord`/`isString`/`isCloseBraceOrEOF`/`asStr`）、`lookupJsKeyword`（36 个保留关键字精确匹配）、`isStrictModeReserved` / `remapStrictModeReserved`（9 个严格模式保留字识别与 `_xxx` 降级重映射）、`JsPropertyModifier`（12 个类成员修饰符，含 `parse`/`isTsAccessibility`）、`isTsAccessibilityModifier`、`lookupJsxEntity`（高频 JSX HTML 命名实体到 Unicode 码点零分配查表）。

```cp
import { JsToken, lookupJsKeyword, remapStrictModeReserved, lookupJsxEntity } from "cap:js";

if (tok := lookupJsKeyword("instanceof")) {
    assert(tok.isReservedWord());
}
if (safe := remapStrictModeReserved("implements")) {
    // safe == "_implements"
    * = safe;
}
if (cp := lookupJsxEntity("rarr")) {
    assert(cp == 0x2192);
}
assert(JsToken.t_question_question_equals.isAssign());
```

### ESM / TypeScript 未使用导入裁剪决策 (`importTrim.cp`)

吸收自 `/data/dev/bun/src/js_parser/ast/ImportScanner.cp`：

- **导出符号**：`IMPORT_TRIM_MAX_ITEMS` (`32`)、`TsImportsNotUsedAsValues`（`remove`/`preserve`/`error_mode`）、`ImportTrimPolicy`（`tsDefault`/`tsBundler`/`shouldTrimUnusedImports`）、`ImportBindingUsage`（`runtimeUsed`/`typeOnly`/`unused`）、`ImportStmtInput`、`ImportStmtDecision`（`keep_with_bindings`/`keep_side_effect_only`/`omit_completely`/`disable_macro`）、`ImportTrimResult`、`evaluateImportStmt`；
- **关键裁剪策略**：
  1. **宏导入与裸副作用导入**：`with { type: "macro" }` 宏导入总是返回 `disable_macro` 并置 `mark_record_unused = true`；裸 `import "./polyfill"` 始终返回 `keep_side_effect_only`；
  2. **Svelte / 局部模块兼容**：非打包且未开启 `minify_identifiers` 且 `importsNotUsedAsValues != remove` 时关闭自动裁剪，保护模板中隐式引用的导入；
  3. **命名空间属性访问保活**：`* as ns` 即使 `runtime_use_count == 0`，若打包器检测到 `has_namespace_prop_access == true` 仍保持存活。

```cp
import { ImportTrimPolicy, ImportStmtInput, ImportBindingUsage, ImportStmtDecision, evaluateImportStmt } from "cap:js";

let stmt = ImportStmtInput.sideEffectOnly();
* = stmt.addNamed(ImportBindingUsage.typeOnly("User"));
* = stmt.addNamed(ImportBindingUsage.runtimeUsed("fetchUser", 2));
const res = evaluateImportStmt(stmt, ImportTrimPolicy.tsDefault());
assert(res.decision == ImportStmtDecision.keep_with_bindings);
assert(res.kept_named_count == 1 && res.removed_binding_count == 1);
```
