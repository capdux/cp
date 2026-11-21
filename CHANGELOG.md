# CP 变更日志 (CHANGELOG) - 2026-11-22 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-22] - 新建 `cap:dct` Orthonormal 1D & 2D 8x8 DCT-II/IDCT-III, JPEG ZigZag scan & IJG quantization（5/5 PASS，自动动态挂载 `cap.dct`）
新增纯 CP 零分配标准库 cap:dct（正交 1D/2D 8×8 DCT-II/IDCT-III、JPEG ZigZag 与 IJG 量化）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:kalman` Discrete 1D & 2D constant-velocity Kalman filter (Joseph form) & Alpha-Beta tracker（5/5 PASS，自动动态挂载 `cap.kalman`）
新增纯 CP 零分配标准库 cap:kalman（离散 1D/2D 卡尔曼滤波器、Joseph 稳定协方差与 Alpha-Beta 滤波）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:pid` Discrete PID controller with derivative-on-measurement, anti-windup, LPF & bumpless transfer（5/5 PASS，自动动态挂载 `cap.pid`）
新增纯 CP 零分配标准库 cap:pid（工业离散 PID 控制器：微分先行、抗积分饱和、一阶低通与无扰切换）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:gf256` Galois Field GF(2^8) (AES/RS) arithmetic, const-time mul, Lagrange & Gauss-Jordan（5/5 PASS，自动动态挂载 `cap.gf256`）
新增纯 CP 零分配标准库 cap:gf256（伽罗瓦域 GF(2^8) AES/RS 算术、常量时间乘法、Lagrange 插值与矩阵求逆）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:fnv` IETF FNV-1/FNV-1a (32/64/128-bit) streaming hashers + DJB2/SDBM/Jenkins OAAT（5/5 PASS，自动动态挂载 `cap.fnv`）
新增纯 CP 零分配标准库 cap:fnv（IETF FNV-1/FNV-1a 32/64/128-bit 流式哈希与 DJB2/SDBM/Jenkins OAAT）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:roman` Strict canonical Roman numerals (1..3999) & Bijective Base-26 spreadsheet column codec（5/5 PASS，自动动态挂载 `cap.roman`）
新增纯 CP 零分配标准库 cap:roman（严格规范罗马数字 1..3999 与电子表格 Base-26 列号编解码）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:minhash` Andrei Broder MinHash Jaccard estimator, union merge, cardinality & LSH band hashing（5/5 PASS，自动动态挂载 `cap.minhash`）
新增纯 CP 零分配标准库 cap:minhash（Andrei Broder MinHash Jaccard 估计、签名单调合并与 LSH 分带哈希）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:simhash` Charikar 64-bit SimHash near-duplicate fingerprinting, shingles & 4x16-bit LSH（5/5 PASS，自动动态挂载 `cap.simhash`）
新增纯 CP 零分配标准库 cap:simhash（64-bit SimHash 近似去重指纹、k-gram 切片与 4×16-bit LSH 分段键）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:porter` Martin Porter (1980) English stemming algorithm (Steps 1a-5b) zero-alloc implementation（5/5 PASS，自动动态挂载 `cap.porter`）
新增纯 CP 零分配标准库 cap:porter（Martin Porter 1980 英文词干提取算法 Steps 1a–5b）与 6/6 深度测试

## [2026-11-22] - 新建 `cap:olc` Google Open Location Code / Plus Codes Base-20 geo-grid encoder, decoder & recovery（5/5 PASS，自动动态挂载 `cap.olc`）
新增纯 CP 零分配标准库 cap:olc（Google Open Location Code / Plus Codes 编解码、缩短与近邻恢复）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:lfu` Zero-alloc LFU cache (LRU tie-break) & 4-bit W-TinyLFU Count-Min Sketch admission filter（5/5 PASS，自动动态挂载 `cap.lfu`）
新增纯 CP 零分配标准库 cap:lfu（固定容量 LFU 缓存与 4-bit W-TinyLFU Count-Min Sketch 准入过滤器）与 5/5 深度测试

## [2026-11-22] - 新建 `cap:soundex` Phonetic indexing & fuzzy name matching: NARA Soundex, Metaphone, NYSIIS & Cologne（5/5 PASS，自动动态挂载 `cap.soundex`）
新增纯 CP 零分配标准库 cap:soundex（NARA Soundex、Metaphone、NYSIIS 与 Cologne Phonetics 语音索引套件）与 6/6 深度测试

