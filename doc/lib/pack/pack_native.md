# Pack Subsystem (cap:pack / cap:compress)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:pack`，导入请使用 `import { ... } from "cap:pack"`。

CP 标准库压缩、解压与归档子系统。

- **Zlib / Gzip / Deflate 详细开发手册**：[docs/pack/zlib.md](file:///data/dev/cp/docs/pack/zlib.md)
- **底层 FFI 声明**：`lib/pack/_codecs.cp`
- **上层核心实现**：`lib/pack/gzip.cp`
- **TAR / ZIP 归档支持**：`lib/pack/tar.cp`, `lib/pack/zip.cp`
