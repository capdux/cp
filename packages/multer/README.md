[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# multer

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:multer` ([https://github.com/expressjs/multer](https://github.com/expressjs/multer))

零堆分配的纯 CP `multipart/form-data` 上传边界解析与策略校验库，Clean-Room MIT 实现对标 npm Top 55 包 `multer` v2（周下载 8000 万+），跨包复用 `@cap/mediatype` 与 `@cap/mime-types`。

## 特性

- **Boundary 解析**：`extractMultipartBoundary` 委托 `@cap/mediatype` 校验 `multipart/form-data` 并提取 `boundary` 参数
- **零堆上传校验器**：`MulterValidator` 支持最多 16 个文件记录与 16 个字段白名单，严格执行 `file_size`、`files`、`fields`、`field_size` 限额并返回标准化错误码
- **自动 MIME 推导**：客户端未提供 `content_type` 时自动委托 `@cap/mime-types` 按文件名推导 MIME 类型

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`expressjs/multer`](https://github.com/expressjs/multer)。
