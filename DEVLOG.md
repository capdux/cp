## 2026-11-22: 新建 `cap:hosted_git_info` GitHub/GitLab/Bitbucket/Gist 托管 Git URL 规范化解析与 https/ssh/sshurl/shortcut/tarball/browse/bugs 多格式生成器（吸收自 bun/src/install/hosted_git_info.cp）（5/5 PASS，文档直写 `doc/lib/hosted_git_info.md`）
子智能体 2 吸收 bun/src/install/hosted_git_info.cp，实现 4 大 Git 平台多协议 URL 解析与 7 种规范输出，5/5 单测通过

## 2026-11-22: 新建 `cap:css_cascade_rules` CSS Cascade 5/6 与作用域规则头（@import layer/supports/media、@scope start/to、@layer 列表）零堆分配解析与序列化（吸收自 bun/src/css/rules/{import,scope,viewport}.cp）（5/5 PASS，文档直写 `doc/lib/css_cascade_rules.md`）
吸收 bun/src/css/rules/{import,scope,viewport}.cp，实现 @import/@scope/@layer 规则头解析与序列化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_position` CSS <position> 1/2/3/4 值二维坐标解析与最短规范化序列化库（吸收自 bun/src/css/values/position.cp）（5/5 PASS，文档直写 `doc/lib/css_position.md`）
吸收 bun/src/css/values/position.cp，实现完整 1-4 值 CSS 位置语法与反向关键字偏移规范化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_svg` SVG CSS 渲染属性族（SVGPaint/StrokeLinecap/StrokeLinejoin/StrokeDasharray/PaintOrder 与 5 大渲染提示枚举，吸收自 bun/src/css/properties/svg.cp）（5/5 PASS，文档直写 `doc/lib/css_svg.md`）
子智能体 1 吸收 bun/src/css/properties/svg.cp，实现完整 SVG 描边/绘制顺序/渲染提示属性解析与规范化序列化，6/6 单测通过

## 2026-11-22: 新建 `cap:css_transition` CSS Transitions 1/2 过渡属性族与多过渡简写列表解析/序列化库（吸收自 bun/src/css/properties/transition.cp）（5/5 PASS，文档直写 `doc/lib/css_transition.md`）
子智能体 2 吸收 bun/src/css/properties/transition.cp，实现单/多过渡简写、cubic-bezier/steps 与默认值省略序列化，5/5 单测通过

## 2026-11-22: 新建 `cap:pkg_matcher` PNPM 兼容三态包名匹配器（全包含/全排除/混合排除优先）与线性时间 */? 通配符引擎（吸收自 bun/src/install/PnpmMatcher.cp）（5/5 PASS，文档直写 `doc/lib/pkg_matcher.md`）
吸收 bun/src/install/PnpmMatcher.cp，实现零堆分配三态包名过滤与线性通配符匹配，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:clap_stream` 零堆分配流式命令行参数解析器（支持 --long/--long=val/-abc 串联/-oval/-- 位置参数分隔与精确错误码，吸收自 bun/src/clap/streaming.cp）（5/5 PASS，文档直写 `doc/lib/clap_stream.md`）
吸收 bun/src/clap/streaming.cp，实现零堆分配流式 CLI 参数解析器，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_border_image` CSS Backgrounds 3 border-image 属性族与双斜杠简写（slice/width/outset/repeat/fill）解析与序列化库（吸收自 bun/src/css/properties/border_image.cp）（5/5 PASS，文档直写 `doc/lib/css_border_image.md`）
子智能体 1 吸收 bun/src/css/properties/border_image.cp，实现完整 border-image 子属性与双斜杠简写，6/6 单测通过

## 2026-11-22: 新建 `cap:shell_util` 零堆分配 POSIX Shell 安全引用/双引号转义、命令行拼接、basename/dirname 路径运算与环境变量解析库（吸收自 bun/src/shell/util.cp 与 builtin/{basename,dirname}.cp）（5/5 PASS，文档直写 `doc/lib/shell_util.md`）
子智能体 2 吸收 bun/src/shell/util.cp 与 basename/dirname，实现全套 POSIX shell 转义与路径/环境变量工具，5/5 单测通过

## 2026-11-22: 新建 `cap:css_layout_ctrl` CSS 布局控制属性库（overflow/text-overflow/position/container-type/contain/z-index，吸收自 bun/src/css/properties/{overflow,position,contain}.cp）（5/5 PASS，文档直写 `doc/lib/css_layout_ctrl.md`）
吸收 bun/src/css/properties/{overflow,position,contain}.cp，实现完整布局控制属性与 contain strict/content 合并规范化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_effects` CSS Filter Effects 11 种标准滤镜函数链（blur/brightness/contrast/grayscale/hue-rotate/invert/opacity/saturate/sepia/drop-shadow/url）零堆分配解析与序列化（吸收自 bun/src/css/properties/effects.cp）（5/5 PASS，文档直写 `doc/lib/css_effects.md`）
吸收 bun/src/css/properties/effects.cp，实现 11 种 CSS 滤镜解析、范围裁剪与规范化序列化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:md_links` CommonMark §6.3-6.5 内联链接、图片、引用链接与 URI/Email 自动链接零堆分配解析器及 URL 百分号编码（吸收自 bun/src/md/links.cp）（5/5 PASS，文档直写 `doc/lib/md_links.md`）
子智能体 1 吸收 bun/src/md/links.cp，实现 CommonMark 全类型链接/图片/自动链接解析与 percentEncodeUrl，6/6 单测通过

## 2026-11-22: 新建 `cap:semver_query` 包管理器多协议依赖版本描述符解析器（npm/workspace/catalog/github/git/file/link/tarball/npm_alias）与 npm 包名校验器（吸收自 bun/src/semver/SemverQuery.cp）（5/5 PASS，文档直写 `doc/lib/semver_query.md`）
子智能体 2 吸收 bun/src/semver/SemverQuery.cp，实现 11 种依赖协议解析与标准 npm 包名校验，5/5 单测通过

## 2026-11-22: 新建 `cap:css_selector_builder` CSS 选择器构建器（Servo 右向左复合匹配序布局）与 Selectors Level 4 特异度计算库（吸收自 bun/src/css/selectors/builder.cp）（5/5 PASS，文档直写 `doc/lib/css_selector_builder.md`）
吸收 bun/src/css/selectors/builder.cp，实现零堆分配 SelectorBuilder、addNestingPrefix、特异度计算与源序/匹配序双输出，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:ci_info` 零堆分配 CI/CD 31 供应商环境与 PR 构建检测器（遵循 ci-info v4，吸收自 bun/src/cli/ci_info.cp 与 codegen/ci_info.ts）（5/5 PASS，文档直写 `doc/lib/ci_info.md`）
吸收 bun/src/cli/ci_info.cp，实现 31 家 CI 平台与 PR 状态确定性检测，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_gradient` CSS Images 4 线性/径向/锥形渐变（含 repeating-* 与双位置色标）零堆分配解析与规范化序列化库（吸收自 bun/src/css/values/gradient.cp）（5/5 PASS，文档直写 `doc/lib/css_gradient.md`）
子智能体 1 吸收 bun/src/css/values/gradient.cp，实现 6 种 CSS 渐变解析与默认值省略规范化序列化，6/6 单测通过

## 2026-11-22: 新建 `cap:errno_map` 跨平台 POSIX/OS Errno 69 项核心错误码双射映射、OS 特定错误消息与瞬态重试判定（吸收自 bun/src/sys/coreutils_error_map.cp 与 errno/{linux,darwin}_errno.cp）（5/5 PASS，文档直写 `doc/lib/errno_map.md`）
子智能体 2 吸收 coreutils_error_map 与 linux/darwin errno，实现 69 变体跨平台双射映射与零 std.posix 依赖查表，5/5 单测通过

## 2026-11-22: 新建 `cap:progress_tree` 零堆分配层级进度树、节流刷新判定与终端状态行格式化引擎（吸收自 bun/src/bun_core/Progress.cp）（5/5 PASS，文档直写 `doc/lib/progress_tree.md`）
吸收 bun/src/bun_core/Progress.cp，实现 32 节点层级进度树、initial_delay/refresh_rate 节流与字节/文件进度行渲染，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:md_line_analysis` CommonMark / GFM 块级行分析器（Setext/ATX 标题、HR、围栏代码块、HTML 块 Type 1-7、62 块级标签与 GFM 表格分隔行，吸收自 bun/src/md/line_analysis.cp）（5/5 PASS，文档直写 `doc/lib/md_line_analysis.md`）
吸收 bun/src/md/line_analysis.cp，实现零堆分配 CommonMark/GFM 块级行分析器，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:grapheme_cluster` Unicode UAX #29 扩展字素簇边界判定（GB3-GB13/Hangul/ZWJ Emoji/国旗）与零分配 UTF-8 迭代器（吸收自 bun/src/string/immutable/grapheme.cp）（5/5 PASS，文档直写 `doc/lib/grapheme_cluster.md`）
子智能体 1 吸收 bun/src/string/immutable/grapheme.cp，实现完整 UAX #29 15 类码点分类、状态机与容错 UTF-8 字素迭代器，6/6 单测通过

## 2026-11-22: 新建 `cap:css_font` CSS Fonts 4 属性族与完整 Font 简写解析/序列化库（吸收自 bun/src/css/properties/font.cp）（5/5 PASS，文档直写 `doc/lib/css_font.md`）
子智能体 2 吸收 bun/src/css/properties/font.cp，实现完整 FontWeight/Style/Stretch/Size/LineHeight/VariantCaps/GenericFontFamily 与 Font 简写，5/5 单测通过

## 2026-11-22: 新建 `cap:css_supports` CSS @supports 条件表达式零分配 Arena 解析、求值与规范化序列化库（吸收自 bun/src/css/rules/supports.cp）（5/5 PASS，文档直写 `doc/lib/css_supports.md`）
吸收 bun/src/css/rules/supports.cp，实现 not/and/or/declaration/selector/unknown 递归下降解析、自定义谓词求值与括号规范化序列化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:md_ref_defs` CommonMark §4.7 链接引用定义解析、零分配标签规范化与首定义优先查找表（吸收自 bun/src/md/ref_defs.cp）（5/5 PASS，文档直写 `doc/lib/md_ref_defs.md`）
吸收 bun/src/md/ref_defs.cp，实现 CommonMark 链接引用定义解析、目标/标题解析器与首定义优先查找表，通过 cpc check 与 cap test

# CP 研发日志 (DEVLOG) - 2026-11-22 (热数据)

## 2026-11-22: 新建 `cap:css_transform` CSS 2D/3D Transform 21 种函数、4x4 齐次矩阵合成、Rodrigues 旋转与原点/样式属性库（吸收自 bun/src/css/properties/transform.cp）（5/5 PASS，文档直写 `doc/lib/css_transform.md`）
子智能体 1 吸收 bun/src/css/properties/transform.cp，实现 21 种 2D/3D Transform、4x4 矩阵乘法/伴随行列式与 RAII TransformList，6/6 单测通过

## 2026-11-22: 新建 `cap:css_background` CSS Background 属性族与多图层简写解析/序列化（吸收自 bun/src/css/properties/background.cp）（5/5 PASS，文档直写 `doc/lib/css_background.md`）
子智能体 2 吸收 bun/src/css/properties/background.cp，实现完整背景子属性、双 box 规则与多图层简写，5/5 单测通过

## 2026-11-22: 新建 `cap:css_sizing` CSS Sizing（width/height/max-size/aspect-ratio/fit-content）与盒模型 Insets（margin/padding/inset）属性库（吸收自 bun/src/css/properties/{size,margin_padding}.cp）（5/5 PASS，文档直写 `doc/lib/css_sizing.md`）
吸收 bun/src/css/properties/{size,margin_padding}.cp，实现完整固有尺寸、厂商前缀、aspect-ratio 与 1-4 值盒模型边距简写，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:css_border` CSS Border 简写、四边宽/样式/颜色与逻辑边属性解析和压缩序列化（吸收自 bun/src/css/properties/border.cp）（5/5 PASS，文档直写 `doc/lib/css_border.md`）
吸收 bun/src/css/properties/border.cp，实现任意顺序 border 简写与 1-4 值四边压缩序列化，通过 cpc check 与 cap test

## 2026-11-22: 新建 `cap:semver_range` SemVer 版本比较器与范围匹配引擎（吸收自 bun/src/semver/{SemverRange,Version}.cp）（5/5 PASS，文档直写 `doc/lib/semver_range.md`）
子智能体 1 吸收 bun/src/semver/{SemverRange,Version}.cp，实现 ^/~/>=/<=/-/x-range/|| 与 prerelease 匹配引擎，6/6 单测通过

## 2026-11-22: 新建 `cap:css_flex_grid` CSS Flexbox / Box Alignment / Grid 布局属性解析与序列化（吸收自 bun/src/css/properties/{flex,align,grid}.cp）（5/5 PASS，文档直写 `doc/lib/css_flex_grid.md`）
子智能体 2 吸收 bun/src/css/properties/{flex,align,grid}.cp，实现完整 Flexbox/Alignment/Grid 布局属性解析与序列化，5/5 单测通过

## 2026-11-22: 新建 `cap:css_border_radius` CSS border-radius 椭圆/四值简写解析与规范化压缩序列化（吸收自 bun/src/css/properties/border_radius.cp）（5/5 PASS，文档直写 `doc/lib/css_border_radius.md`）
吸收 bun/src/css/properties/border_radius.cp，实现纯 CP 椭圆斜杠与 1-4 值压缩 border-radius 解析器与序列化器，通过 cpc check 与 cap test

## 2026-11-22: CSS box-shadow 与 border/outline 线型属性（5/5 PASS，文档直写 `doc/lib/css_box_shadow.md`）
从 Bun css/properties/box_shadow.cp 与 outline.cp 吸收 box-shadow 简写与线型枚举，自动化简零 blur/spread 与 currentcolor 并拦截负模糊半径。

## 2026-11-22: PostgreSQL v3 COPY 协议与 SASL/Password 前端报文编解码（5/5 PASS，文档直写 `doc/lib/pg_copy_sasl.md`）
从 Bun sql/postgres/protocol 吸收 COPY/SASL/Password/NegotiateProtocolVersion 全套编解码器，采用 ~init() 确定性 RAII 零泄漏释放，补齐 PG v3 协议全栈。

## 2026-11-22: CSS Animations & Transitions 简写解析与序列化（5/5 PASS，文档直写 `doc/lib/css_animation.md`）
从 Bun css/properties/animation.cp 与 transition.cp 吸收 CSS 动画与过渡简写解析器，跨模块复用 cap:css_time 与 cap:css_easing。

## 2026-11-22: MySQL 认证密码学原语与 SSLRequest 编解码（5/5 PASS，文档直写 `doc/lib/mysql_auth.md`）
从 Bun sql/mysql/protocol/Auth.cp 与 SSLRequest.cp 吸收 MySQL 认证密码学原语，补齐 MySQL 线协议全栈。

## 2026-11-22: PostgreSQL v3 后端控制与诊断消息解码器（5/5 PASS，文档直写 `doc/lib/pg_messages.md`）
从 Bun sql/postgres/protocol 吸收完整 PG v3 后端控制与 18 字段诊断消息零拷贝解码器，支持 SQLSTATE 分类与 CommandComplete 影响行数解析。

## 2026-11-22: MySQL 协议服务端握手与控制报文解码器（5/5 PASS，文档直写 `doc/lib/mysql_control.md`）
从 Bun sql/mysql/protocol/{HandshakeV10,OKPacket,ErrorPacket,EOFPacket,AuthSwitchRequest,CommandType}.cp 吸收 MySQL 服务端控制报文零拷贝解码器。

## 2026-11-22: PostgreSQL v3 扩展查询协议前端编码器（5/5 PASS，文档直写 `doc/lib/pg_extended_query.md`）
从 Bun sql/postgres/protocol/{Parse,Describe,Execute,Close,PortalOrPreparedStatement}.cp 吸收 PG v3 扩展查询协议前端流水线编码器，支持 ~init() 确定性零泄漏释放。

## 2026-11-22: CSS Basic UI Level 4 属性集（5/5 PASS，文档直写 `doc/lib/css_ui.md`）
从 Bun css/properties/ui.cp 吸收 CSS UI Level 4 六大交互属性模型，纯 CP 零堆分配实现规范化解析与输出。

## 2026-11-22: CSS Media Queries Level 4 媒体查询解析与求值引擎（5/5 PASS，文档直写 `doc/lib/css_media.md`）
从 Bun css/rules/media.cp 吸收 CSS Media Queries Level 4 解析与视口/用户偏好求值引擎，纯 CP 零堆分配内联存储。

## 2026-11-22: CSS Properties & Values API <syntax> 解析引擎（5/5 PASS，文档直写 `doc/lib/css_syntax.md`）
从 Bun css/values/syntax.cp 吸收 CSS <syntax> 解析器，支持 * 零分配、+/# 乘数与 <transform-list> 约束校验及 ~init() 零泄漏。

## 2026-11-22: CSS @font-face Unicode 码点区间与通配符优化器（5/5 PASS，文档直写 `doc/lib/unicode_range.md`）
从 Bun css/rules/font_face.cp 吸收 UnicodeRange 解析与十六进制边界 ? 通配符自动压缩算法，纯 CP 零堆分配。

## 2026-11-22: CSS 基础值类型（Alpha / Percentage / URL 分类）（5/5 PASS，文档直写 `doc/lib/css_primitives.md`）
从 Bun css/values/{alpha,percentage,url}.cp 吸收 CSS 基础值类型与 URL 绝对/相对/Data URI 分类器，纯 CP 零堆分配。

## 2026-11-22: CSS Container Queries @container 规则与求值引擎（5/5 PASS，文档直写 `doc/lib/css_container.md`）
从 Bun css/rules/container.cp 吸收 CSS Container Queries Level 3 解析与求值引擎，支持传统与现代范围语法及混合逻辑歧义拦截。

## 2026-11-22: CSS <angle> 与 <resolution> 单位解析与换算库（5/5 PASS，文档直写 `doc/lib/css_units.md`）
从 Bun css/values/angle.cp 与 resolution.cp 吸收 CSS 角度与分辨率单位库，支持无单位 0 角度、0..360 归一化、x 别名与负分辨率拦截。

## 2026-11-22: MySQL 4.1 握手响应报文编解码器（5/5 PASS，文档直写 `doc/lib/mysql_handshake.md`）
从 Bun sql/mysql/protocol/HandshakeResponse41.cp 吸收 MySQL 4.1 客户端握手响应编解码器，支持能力位自动切换与 ~init() 零泄漏释放。

## 2026-11-22: ECMAScript/Web/Node/Bun 已知全局符号与纯度分析器（5/5 PASS，文档直写 `doc/lib/known_globals.md`）
从 Bun js_parser/ast/KnownGlobal.cp 吸收 69 项 ECMAScript/Web/Node/Bun 全局符号纯度表，零堆分配支持静态属性访问与纯调用 DCE 判定。

## 2026-11-22: 二分查找源码行列偏移表 LineOffsetTable（5/5 PASS，文档直写 `doc/lib/line_offset_table.md`）
从 Bun sourcemap/LineOffsetTable.cp 吸收源码行列偏移表，单遍扫描识别 LF/CRLF/CR 并支持 O(log N) 二分定位与 ~init() 确定性零泄漏释放。

## 2026-11-22: CSS @keyframes/@layer/@counter-style/@property 规则集（5/5 PASS，文档直写 `doc/lib/css_at_rules.md`）
从 Bun css/rules/{keyframes,layer,counter_style,property}.cp 吸收四大 CSS @ 规则元数据与校验模型，零堆分配内联实现。

## 2026-11-22: CSS <time> 时间值解析、运算与规范序列化（5/5 PASS，文档直写 `doc/lib/css_time.md`）
从 Bun css/values/time.cp 吸收 CSS <time> 单位解析与最短规范序列化（0.05s→50ms、500ms→0.5s、0ms→0s），纯 CP 零堆分配。

## 2026-11-22: ThumbHash 超紧凑图片占位符编解码器（5/5 PASS，文档直写 `doc/lib/thumbhash.md`）
从 Bun runtime/image/thumbhash.cp 吸收完整 ThumbHash DCT/IDCT 编解码引擎，补充 approxAspectRatio/averageRGBA 零分配查询与 ~init() 确定性释放。

## 2026-11-22: 零解码极速图片头格式与尺寸探测器（5/5 PASS，文档直写 `doc/lib/image_probe.md`）
从 Bun runtime/image/{codec_png,codec_jpeg,codec_webp,codec_bmp}.cp 吸收零解码五格式图片头探测器，纯 CP 零堆分配且内置严密畸形头边界防护。
