# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:css_svg` SVG CSS 渲染属性族（SVGPaint/StrokeLinecap/StrokeLinejoin/StrokeDasharray/PaintOrder 与 5 大渲染提示枚举，吸收自 bun/src/css/properties/svg.cp）（5/5 PASS，自动动态挂载 `cap.css_svg`）
新增 cap:css_svg 标准库模块（lib/css_svg/{svg,index}.cp, doc/lib/css_svg.md, cap/test/test_css_svg_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_transition` CSS Transitions 1/2 过渡属性族与多过渡简写列表解析/序列化库（吸收自 bun/src/css/properties/transition.cp）（5/5 PASS，自动动态挂载 `cap.css_transition`）
新增 cap:css_transition 标准库模块（lib/css_transition/{transition,index}.cp, doc/lib/css_transition.md, cap/test/test_css_transition_production_deep.cp）

## [2026-11-22] - 新建 `cap:pkg_matcher` PNPM 兼容三态包名匹配器（全包含/全排除/混合排除优先）与线性时间 */? 通配符引擎（吸收自 bun/src/install/PnpmMatcher.cp）（5/5 PASS，自动动态挂载 `cap.pkg_matcher`）
新增 cap:pkg_matcher 标准库模块（lib/pkg_matcher/{matcher,index}.cp, doc/lib/pkg_matcher.md, cap/test/test_pkg_matcher_production_deep.cp）

## [2026-11-22] - 新建 `cap:clap_stream` 零堆分配流式命令行参数解析器（支持 --long/--long=val/-abc 串联/-oval/-- 位置参数分隔与精确错误码，吸收自 bun/src/clap/streaming.cp）（5/5 PASS，自动动态挂载 `cap.clap_stream`）
新增 cap:clap_stream 标准库模块（lib/clap_stream/{clap,index}.cp, doc/lib/clap_stream.md, cap/test/test_clap_stream_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_border_image` CSS Backgrounds 3 border-image 属性族与双斜杠简写（slice/width/outset/repeat/fill）解析与序列化库（吸收自 bun/src/css/properties/border_image.cp）（5/5 PASS，自动动态挂载 `cap.css_border_image`）
新增 cap:css_border_image 标准库模块（lib/css_border_image/{border_image,index}.cp, doc/lib/css_border_image.md, cap/test/test_css_border_image_production_deep.cp）

## [2026-11-22] - 新建 `cap:shell_util` 零堆分配 POSIX Shell 安全引用/双引号转义、命令行拼接、basename/dirname 路径运算与环境变量解析库（吸收自 bun/src/shell/util.cp 与 builtin/{basename,dirname}.cp）（5/5 PASS，自动动态挂载 `cap.shell_util`）
新增 cap:shell_util 标准库模块（lib/shell_util/{util,index}.cp, doc/lib/shell_util.md, cap/test/test_shell_util_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_layout_ctrl` CSS 布局控制属性库（overflow/text-overflow/position/container-type/contain/z-index，吸收自 bun/src/css/properties/{overflow,position,contain}.cp）（5/5 PASS，自动动态挂载 `cap.css_layout_ctrl`）
新增 cap:css_layout_ctrl 标准库模块（lib/css_layout_ctrl/{layout_ctrl,index}.cp, doc/lib/css_layout_ctrl.md, cap/test/test_css_layout_ctrl_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_effects` CSS Filter Effects 11 种标准滤镜函数链（blur/brightness/contrast/grayscale/hue-rotate/invert/opacity/saturate/sepia/drop-shadow/url）零堆分配解析与序列化（吸收自 bun/src/css/properties/effects.cp）（5/5 PASS，自动动态挂载 `cap.css_effects`）
新增 cap:css_effects 标准库模块（lib/css_effects/{effects,index}.cp, doc/lib/css_effects.md, cap/test/test_css_effects_production_deep.cp）

## [2026-11-22] - 新建 `cap:md_links` CommonMark §6.3-6.5 内联链接、图片、引用链接与 URI/Email 自动链接零堆分配解析器及 URL 百分号编码（吸收自 bun/src/md/links.cp）（5/5 PASS，自动动态挂载 `cap.md_links`）
新增 cap:md_links 标准库模块（lib/md_links/{links,index}.cp, doc/lib/md_links.md, cap/test/test_md_links_production_deep.cp）

## [2026-11-22] - 新建 `cap:semver_query` 包管理器多协议依赖版本描述符解析器（npm/workspace/catalog/github/git/file/link/tarball/npm_alias）与 npm 包名校验器（吸收自 bun/src/semver/SemverQuery.cp）（5/5 PASS，自动动态挂载 `cap.semver_query`）
新增 cap:semver_query 标准库模块（lib/semver_query/{query,index}.cp, doc/lib/semver_query.md, cap/test/test_semver_query_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_selector_builder` CSS 选择器构建器（Servo 右向左复合匹配序布局）与 Selectors Level 4 特异度计算库（吸收自 bun/src/css/selectors/builder.cp）（5/5 PASS，自动动态挂载 `cap.css_selector_builder`）
新增 cap:css_selector_builder 标准库模块（lib/css_selector_builder/{builder,index}.cp, doc/lib/css_selector_builder.md, cap/test/test_css_selector_builder_production_deep.cp）

## [2026-11-22] - 新建 `cap:ci_info` 零堆分配 CI/CD 31 供应商环境与 PR 构建检测器（遵循 ci-info v4，吸收自 bun/src/cli/ci_info.cp 与 codegen/ci_info.ts）（5/5 PASS，自动动态挂载 `cap.ci_info`）
新增 cap:ci_info 标准库模块（lib/ci_info/{ci_info,index}.cp, doc/lib/ci_info.md, cap/test/test_ci_info_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_gradient` CSS Images 4 线性/径向/锥形渐变（含 repeating-* 与双位置色标）零堆分配解析与规范化序列化库（吸收自 bun/src/css/values/gradient.cp）（5/5 PASS，自动动态挂载 `cap.css_gradient`）
新增 cap:css_gradient 标准库模块（lib/css_gradient/{gradient,index}.cp, doc/lib/css_gradient.md, cap/test/test_css_gradient_production_deep.cp）

## [2026-11-22] - 新建 `cap:errno_map` 跨平台 POSIX/OS Errno 69 项核心错误码双射映射、OS 特定错误消息与瞬态重试判定（吸收自 bun/src/sys/coreutils_error_map.cp 与 errno/{linux,darwin}_errno.cp）（5/5 PASS，自动动态挂载 `cap.errno_map`）
新增 cap:errno_map 标准库模块（lib/errno_map/{errno,index}.cp, doc/lib/errno_map.md, cap/test/test_errno_map_production_deep.cp）

## [2026-11-22] - 新建 `cap:progress_tree` 零堆分配层级进度树、节流刷新判定与终端状态行格式化引擎（吸收自 bun/src/bun_core/Progress.cp）（5/5 PASS，自动动态挂载 `cap.progress_tree`）
新增 cap:progress_tree 标准库模块（lib/progress_tree/{progress,index}.cp, doc/lib/progress_tree.md, cap/test/test_progress_tree_production_deep.cp）

## [2026-11-22] - 新建 `cap:md_line_analysis` CommonMark / GFM 块级行分析器（Setext/ATX 标题、HR、围栏代码块、HTML 块 Type 1-7、62 块级标签与 GFM 表格分隔行，吸收自 bun/src/md/line_analysis.cp）（5/5 PASS，自动动态挂载 `cap.md_line_analysis`）
新增 cap:md_line_analysis 标准库模块（lib/md_line_analysis/{line_analysis,index}.cp, doc/lib/md_line_analysis.md, cap/test/test_md_line_analysis_production_deep.cp）

## [2026-11-22] - 新建 `cap:grapheme_cluster` Unicode UAX #29 扩展字素簇边界判定（GB3-GB13/Hangul/ZWJ Emoji/国旗）与零分配 UTF-8 迭代器（吸收自 bun/src/string/immutable/grapheme.cp）（5/5 PASS，自动动态挂载 `cap.grapheme_cluster`）
新增 cap:grapheme_cluster 标准库模块（lib/grapheme_cluster/{grapheme,index}.cp, doc/lib/grapheme_cluster.md, cap/test/test_grapheme_cluster_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_font` CSS Fonts 4 属性族与完整 Font 简写解析/序列化库（吸收自 bun/src/css/properties/font.cp）（5/5 PASS，自动动态挂载 `cap.css_font`）
新增 cap:css_font 标准库模块（lib/css_font/{font,index}.cp, doc/lib/css_font.md, cap/test/test_css_font_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_supports` CSS @supports 条件表达式零分配 Arena 解析、求值与规范化序列化库（吸收自 bun/src/css/rules/supports.cp）（5/5 PASS，自动动态挂载 `cap.css_supports`）
新增 cap:css_supports 标准库模块（lib/css_supports/{supports,index}.cp, doc/lib/css_supports.md, cap/test/test_css_supports_production_deep.cp）

## [2026-11-22] - 新建 `cap:md_ref_defs` CommonMark §4.7 链接引用定义解析、零分配标签规范化与首定义优先查找表（吸收自 bun/src/md/ref_defs.cp）（5/5 PASS，自动动态挂载 `cap.md_ref_defs`）
新增 cap:md_ref_defs 标准库模块（lib/md_ref_defs/{ref_defs,index}.cp, doc/lib/md_ref_defs.md, cap/test/test_md_ref_defs_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_transform` CSS 2D/3D Transform 21 种函数、4x4 齐次矩阵合成、Rodrigues 旋转与原点/样式属性库（吸收自 bun/src/css/properties/transform.cp）（5/5 PASS，自动动态挂载 `cap.css_transform`）
新增 cap:css_transform 标准库模块（lib/css_transform/{transform,index}.cp, doc/lib/css_transform.md, cap/test/test_css_transform_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_background` CSS Background 属性族与多图层简写解析/序列化（吸收自 bun/src/css/properties/background.cp）（5/5 PASS，自动动态挂载 `cap.css_background`）
新增 cap:css_background 标准库模块（lib/css_background/{background,index}.cp, doc/lib/css_background.md, cap/test/test_css_background_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_sizing` CSS Sizing（width/height/max-size/aspect-ratio/fit-content）与盒模型 Insets（margin/padding/inset）属性库（吸收自 bun/src/css/properties/{size,margin_padding}.cp）（5/5 PASS，自动动态挂载 `cap.css_sizing`）
新增 cap:css_sizing 标准库模块（lib/css_sizing/{sizing,index}.cp, doc/lib/css_sizing.md, cap/test/test_css_sizing_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_border` CSS Border 简写、四边宽/样式/颜色与逻辑边属性解析和压缩序列化（吸收自 bun/src/css/properties/border.cp）（5/5 PASS，自动动态挂载 `cap.css_border`）
新增 cap:css_border 标准库模块（lib/css_border/{border,index}.cp, doc/lib/css_border.md, cap/test/test_css_border_production_deep.cp）

## [2026-11-22] - 新建 `cap:semver_range` SemVer 版本比较器与范围匹配引擎（吸收自 bun/src/semver/{SemverRange,Version}.cp）（5/5 PASS，自动动态挂载 `cap.semver_range`）
新增 cap:semver_range 标准库模块（lib/semver_range/{range,index}.cp, doc/lib/semver_range.md, cap/test/test_semver_range_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_flex_grid` CSS Flexbox / Box Alignment / Grid 布局属性解析与序列化（吸收自 bun/src/css/properties/{flex,align,grid}.cp）（5/5 PASS，自动动态挂载 `cap.css_flex_grid`）
新增 cap:css_flex_grid 标准库模块（lib/css_flex_grid/{layout,index}.cp, doc/lib/css_flex_grid.md, cap/test/test_css_flex_grid_production_deep.cp）

## [2026-11-22] - 新建 `cap:css_border_radius` CSS border-radius 椭圆/四值简写解析与规范化压缩序列化（吸收自 bun/src/css/properties/border_radius.cp）（5/5 PASS，自动动态挂载 `cap.css_border_radius`）
新增 cap:css_border_radius 标准库模块（lib/css_border_radius/{radius,index}.cp, doc/lib/css_border_radius.md, cap/test/test_css_border_radius_production_deep.cp）

## [2026-11-22] - CSS box-shadow 与 border/outline 线型属性（5/5 PASS，自动动态挂载 `cap.css_box_shadow`）
新增 cap:css_box_shadow 标准库（lib/css_box_shadow/、doc/lib/css_box_shadow.md、cap/test/test_css_box_shadow_production_deep.cp），2/2 回归通过。

## [2026-11-22] - PostgreSQL v3 COPY 协议与 SASL/Password 前端报文编解码（5/5 PASS，自动动态挂载 `cap.pg_copy_sasl`）
新增 cap:pg_copy_sasl 标准库（lib/pg_copy_sasl/、doc/lib/pg_copy_sasl.md、cap/test/test_pg_copy_sasl_production_deep.cp），5/5 回归通过（零泄漏），PostgreSQL v3 线协议全栈齐备。

## [2026-11-22] - CSS Animations & Transitions 简写解析与序列化（5/5 PASS，自动动态挂载 `cap.css_animation`）
新增 cap:css_animation 标准库（lib/css_animation/、doc/lib/css_animation.md、cap/test/test_css_animation_production_deep.cp），2/2 回归通过。

## [2026-11-22] - MySQL 认证密码学原语与 SSLRequest 编解码（5/5 PASS，自动动态挂载 `cap.mysql_auth`）
新增 cap:mysql_auth 标准库（lib/mysql_auth/、doc/lib/mysql_auth.md、cap/test/test_mysql_auth_production_deep.cp），6/6 回归通过。

## [2026-11-22] - PostgreSQL v3 后端控制与诊断消息解码器（5/5 PASS，自动动态挂载 `cap.pg_messages`）
新增 cap:pg_messages 标准库（lib/pg_messages/、doc/lib/pg_messages.md、cap/test/test_pg_messages_production_deep.cp），5/5 回归通过。

## [2026-11-22] - MySQL 协议服务端握手与控制报文解码器（5/5 PASS，自动动态挂载 `cap.mysql_control`）
新增 cap:mysql_control 标准库（lib/mysql_control/、doc/lib/mysql_control.md、cap/test/test_mysql_control_production_deep.cp），6/6 回归通过。

## [2026-11-22] - PostgreSQL v3 扩展查询协议前端编码器（5/5 PASS，自动动态挂载 `cap.pg_extended_query`）
新增 cap:pg_extended_query 标准库（lib/pg_extended_query/、doc/lib/pg_extended_query.md、cap/test/test_pg_extended_query_production_deep.cp），2/2 回归通过（零泄漏）。

## [2026-11-22] - CSS Basic UI Level 4 属性集（5/5 PASS，自动动态挂载 `cap.css_ui`）
新增 cap:css_ui 标准库（lib/css_ui/、doc/lib/css_ui.md、cap/test/test_css_ui_production_deep.cp），2/2 回归通过。

## [2026-11-22] - CSS Media Queries Level 4 媒体查询解析与求值引擎（5/5 PASS，自动动态挂载 `cap.css_media`）
新增 cap:css_media 标准库（lib/css_media/、doc/lib/css_media.md、cap/test/test_css_media_production_deep.cp），5/5 回归通过。

## [2026-11-22] - CSS Properties & Values API <syntax> 解析引擎（5/5 PASS，自动动态挂载 `cap.css_syntax`）
新增 cap:css_syntax 标准库（lib/css_syntax/、doc/lib/css_syntax.md、cap/test/test_css_syntax_production_deep.cp），6/6 回归通过（零泄漏）。

## [2026-11-22] - CSS @font-face Unicode 码点区间与通配符优化器（5/5 PASS，自动动态挂载 `cap.unicode_range`）
新增 cap:unicode_range 标准库（lib/unicode_range/、doc/lib/unicode_range.md、cap/test/test_unicode_range_production_deep.cp），2/2 回归通过。

## [2026-11-22] - CSS 基础值类型（Alpha / Percentage / URL 分类）（5/5 PASS，自动动态挂载 `cap.css_primitives`）
新增 cap:css_primitives 标准库（lib/css_primitives/、doc/lib/css_primitives.md、cap/test/test_css_primitives_production_deep.cp），2/2 回归通过。

## [2026-11-22] - CSS Container Queries @container 规则与求值引擎（5/5 PASS，自动动态挂载 `cap.css_container`）
新增 cap:css_container 标准库（lib/css_container/、doc/lib/css_container.md、cap/test/test_css_container_production_deep.cp），5/5 回归通过。

## [2026-11-22] - CSS <angle> 与 <resolution> 单位解析与换算库（5/5 PASS，自动动态挂载 `cap.css_units`）
新增 cap:css_units 标准库（lib/css_units/、doc/lib/css_units.md、cap/test/test_css_units_production_deep.cp），6/6 回归通过。

## [2026-11-22] - MySQL 4.1 握手响应报文编解码器（5/5 PASS，自动动态挂载 `cap.mysql_handshake`）
新增 cap:mysql_handshake 标准库（lib/mysql_handshake/、doc/lib/mysql_handshake.md、cap/test/test_mysql_handshake_production_deep.cp），2/2 回归通过（零泄漏）。

## [2026-11-22] - ECMAScript/Web/Node/Bun 已知全局符号与纯度分析器（5/5 PASS，自动动态挂载 `cap.known_globals`）
新增 cap:known_globals 标准库（lib/known_globals/、doc/lib/known_globals.md、cap/test/test_known_globals_production_deep.cp），5/5 回归通过。

## [2026-11-22] - 二分查找源码行列偏移表 LineOffsetTable（5/5 PASS，自动动态挂载 `cap.line_offset_table`）
新增 cap:line_offset_table 标准库（lib/line_offset_table/、doc/lib/line_offset_table.md、cap/test/test_line_offset_table_production_deep.cp），6/6 回归通过（零泄漏）。

## [2026-11-22] - CSS @keyframes/@layer/@counter-style/@property 规则集（5/5 PASS，自动动态挂载 `cap.css_at_rules`）
新增 cap:css_at_rules 标准库（lib/css_at_rules/、doc/lib/css_at_rules.md、cap/test/test_css_at_rules_production_deep.cp），5/5 回归通过。

## [2026-11-22] - CSS <time> 时间值解析、运算与规范序列化（5/5 PASS，自动动态挂载 `cap.css_time`）
新增 cap:css_time 标准库（lib/css_time/、doc/lib/css_time.md、cap/test/test_css_time_production_deep.cp），2/2 回归通过。

## [2026-11-22] - ThumbHash 超紧凑图片占位符编解码器（5/5 PASS，自动动态挂载 `cap.thumbhash`）
新增 cap:thumbhash 标准库（lib/thumbhash/、doc/lib/thumbhash.md、cap/test/test_thumbhash_production_deep.cp），3/3 回归通过（零泄漏）。

## [2026-11-22] - 零解码极速图片头格式与尺寸探测器（5/5 PASS，自动动态挂载 `cap.image_probe`）
新增 cap:image_probe 标准库（lib/image_probe/、doc/lib/image_probe.md、cap/test/test_image_probe_production_deep.cp），6/6 回归通过。
