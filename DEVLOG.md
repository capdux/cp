## 2026-11-22: 新建 `cap:byterange` 吸收自 Bun runtime/server/RangeRequest.cp 的纯 CP 零分配 HTTP Range（RFC 9110）单/多段解析与 Content-Range 206/416 编解码库（5/5 PASS，文档直写 `doc/lib/byterange.md`）
从 /data/dev/bun/src/runtime/server/RangeRequest.cp 吸收纯 CP 零分配 HTTP Range 库 cap:byterange（lib/byterange/{byterange,index}.cp），cpc check 0 错误，cap test 5/5 PASS (761ms)

## 2026-11-22: 新建 `cap:catalog` 吸收自 Bun install/lockfile/CatalogMap.cp 的纯 CP 零分配 Monorepo 依赖目录协议库：catalog:/catalog:<group> 解析、默认与命名分组管理及确定性 Lockfile 输出（5/5 PASS，文档直写 `doc/lib/catalog.md`）
从 /data/dev/bun/src/install/lockfile/CatalogMap.cp 吸收纯 CP 零分配 Monorepo catalog: 协议库 cap:catalog（lib/catalog/{catalog,index}.cp），cpc check 0 错误，cap test 5/5 PASS (779ms)



## 2026-11-22: 新建 `cap:bloburl` 吸收自 Bun runtime/webcore/ObjectURLRegistry.cp 的纯 CP 零分配 W3C blob: Object URL 解析、RFC 4122 UUID v4 生成与固定池注册表库（5/5 PASS，文档直写 `doc/lib/bloburl.md`）
从 /data/dev/bun/src/runtime/webcore/ObjectURLRegistry.cp 吸收纯 CP 零分配 Object URL 注册表 cap:bloburl（lib/bloburl/{bloburl,index}.cp），cpc check 0 错误，cap test 5/5 PASS (804ms)

## 2026-11-22: 新建 `cap:textdecode` 吸收自 Bun runtime/webcore/TextDecoder.cp 的纯 CP 零分配 WHATWG 流式文本解码器：UTF-8/UTF-16LE/UTF-16BE/Latin-1 跨块重组、BOM 探测剥离与 fatal/U+FFFD 替换模式（5/5 PASS，文档直写 `doc/lib/textdecode.md`）
从 /data/dev/bun/src/runtime/webcore/TextDecoder.cp 吸收纯 CP 零分配流式文本解码器 cap:textdecode（lib/textdecode/{textdecode,index}.cp），cpc check 0 错误，cap test 5/5 PASS (770ms)

## 2026-11-22: 新建 `cap:cssmodules` 吸收自 Bun css/css_modules.cp 的纯 CP 零分配 CSS Modules 命名模板、Wyhash+Base64 确定性哈希、作用域类名编译与 composes 跨文件依赖解析库（5/5 PASS，文档直写 `doc/lib/cssmodules.md`）
从 /data/dev/bun/src/css/css_modules.cp 吸收纯 CP 零分配 CSS Modules 库 cap:cssmodules（lib/cssmodules/{cssmodules,index}.cp），cpc check 0 错误，cap test 5/5 PASS (708ms)

## 2026-11-22: 新建 `cap:mediaquery` 吸收自 Bun css/media_query.cp 的纯 CP 零分配 W3C CSS Media Queries Level 4 库：长度/分辨率/宽高比单位换算、单/双边区间范围、媒体特性求值与规范化（5/5 PASS，文档直写 `doc/lib/mediaquery.md`）
从 /data/dev/bun/src/css/media_query.cp 吸收纯 CP 零分配 CSS Media Queries Level 4 库 cap:mediaquery（lib/mediaquery/{mediaquery,index}.cp），cpc check 0 错误，cap test 6/6 PASS (710ms)

## 2026-11-22: 新建 `cap:textstream` 吸收自 Bun runtime/webcore/TextEncoderStreamEncoder.cp 的纯 CP 零分配 WHATWG 流式 UTF-8 编码器：跨分块 UTF-16 代理对拼接、孤立代理 U+FFFD 替换、Latin-1 转码与小缓冲原子保护（5/5 PASS，文档直写 `doc/lib/textstream.md`）
从 /data/dev/bun/src/runtime/webcore/TextEncoderStreamEncoder.cp 吸收纯 CP 零分配流式 UTF-8 编码器 cap:textstream（lib/textstream/{textstream,index}.cp），cpc check 0 错误，cap test 5/5 PASS (736ms)

## 2026-11-22: 新建 `cap:h2stream` 吸收自 Bun http/h2_client/Stream.cp 的纯 CP 零分配 HTTP/2（RFC 9113）流状态机与双向流控窗口库：半关闭生命周期、14 种错误码、RST_STREAM/WINDOW_UPDATE 帧编码与自动补窗（5/5 PASS，文档直写 `doc/lib/h2stream.md`）
从 /data/dev/bun/src/http/h2_client/Stream.cp 吸收纯 CP 零分配 HTTP/2 流状态机 cap:h2stream（lib/h2stream/{h2stream,index}.cp），cpc check 0 错误，cap test 6/6 PASS (789ms)

## 2026-11-22: 新建 `cap:qpack` 吸收自 Bun uws_sys/quic/Header.cp 与 http/h3_client/encode.cp 的纯 CP 零分配 HTTP/3 QPACK（RFC 9204）编解码库：静态表分类、多前缀整数编解码、请求伪头部编排与 Field Section 零拷贝往返（5/5 PASS，文档直写 `doc/lib/qpack.md`）
从 /data/dev/bun/src/uws_sys/quic/Header.cp 吸收纯 CP 零分配 HTTP/3 QPACK 库 cap:qpack（lib/qpack/{qpack,index}.cp），cpc check 0 错误，cap test 5/5 PASS (776ms)

## 2026-11-22: 新建 `cap:h2_encode` 吸收自 Bun http/h2_client/encode.cp 的纯 CP 零分配 HTTP/2 出站编码库：RFC 9113 请求头过滤、HPACK 动态表更新、客户端序言、HEADERS/CONTINUATION 分片与双窗口 DATA 帧编码（5/5 PASS，文档直写 `doc/lib/h2_encode.md`）
从 /data/dev/bun/src/http/h2_client/encode.cp 吸收纯 CP 零分配 HTTP/2 出站编码库 cap:h2_encode（lib/h2_encode/{h2_encode,index}.cp），cpc check 0 错误，cap test 6/6 PASS (886ms)

