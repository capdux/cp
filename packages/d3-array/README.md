[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-array` ([https://github.com/d3/d3-array](https://github.com/d3/d3-array)) | 许可：`ISC`

# `@cap/d3-array`

零堆分配纯 CP 数组统计、R-7 分位数、Welford 稳定方差/标准差、二分查找与直方图分箱库，对标 npm **#345** `d3-array`（真实周下载 **1.53 亿/周**）。

## 核心 API

- 统计：`sumF64`, `meanF64`, `extentF64`, `varianceF64`, `deviationF64`, `cumsumF64`
- 分位数与查找：`quantileSorted`, `medianSorted`, `bisectLeft`, `bisectRight`
- 直方图：`binCounts`
