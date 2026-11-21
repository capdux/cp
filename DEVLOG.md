## 2026-11-22: 新建 `cap:dct` Orthonormal 1D & 2D 8x8 DCT-II/IDCT-III, JPEG ZigZag scan & IJG quantization（5/5 PASS，文档直写 `doc/lib/dct.md`）
新增 cap:dct 纯 CP 零分配图像与信号变换库（能量守恒 1D DCT、可分离 8×8 块变换与 IJG 质量缩放量化），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:kalman` Discrete 1D & 2D constant-velocity Kalman filter (Joseph form) & Alpha-Beta tracker（5/5 PASS，文档直写 `doc/lib/kalman.md`）
新增 cap:kalman 纯 CP 零分配传感器融合与状态估计库（1D/2D 恒速模型、Joseph 正定协方差更新与 Mahalanobis 野值门控），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:pid` Discrete PID controller with derivative-on-measurement, anti-windup, LPF & bumpless transfer（5/5 PASS，文档直写 `doc/lib/pid.md`）
新增 cap:pid 纯 CP 零分配闭环控制库（含 Ziegler-Nichols 整定、斜率限幅与条件积分 Anti-Windup），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:gf256` Galois Field GF(2^8) (AES/RS) arithmetic, const-time mul, Lagrange & Gauss-Jordan（5/5 PASS，文档直写 `doc/lib/gf256.md`）
新增 cap:gf256 纯 CP 零分配有限域库（全 256×256 查表与无分支常量时间一致、RAID-6 矩阵求逆恢复与 Shamir 门限插值），通过 cpc check 与 5/5 cap test



## 2026-11-22: 新建 `cap:fnv` IETF FNV-1/FNV-1a (32/64/128-bit) streaming hashers + DJB2/SDBM/Jenkins OAAT（5/5 PASS，文档直写 `doc/lib/fnv.md`）
新增 cap:fnv 纯 CP 零分配经典非加密哈希套件（覆盖 IETF 32/64/128-bit 官方测试向量与流式分块增量更新），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:roman` Strict canonical Roman numerals (1..3999) & Bijective Base-26 spreadsheet column codec（5/5 PASS，文档直写 `doc/lib/roman.md`）
新增 cap:roman 纯 CP 零分配数字与列号转换库（1..3999 穷举双射、严格/宽松模式与 Bijective Base-26 A..XFD），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:minhash` Andrei Broder MinHash Jaccard estimator, union merge, cardinality & LSH band hashing（5/5 PASS，文档直写 `doc/lib/minhash.md`）
新增 cap:minhash 纯 CP 零分配集合相似度估计库（SplitMix64 盐哈希族、并集签名合并与 32×4 LSH 分带检索），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:simhash` Charikar 64-bit SimHash near-duplicate fingerprinting, shingles & 4x16-bit LSH（5/5 PASS，文档直写 `doc/lib/simhash.md`）
新增 cap:simhash 纯 CP 零分配近似重复文档指纹库（栈上 64 维权重向量、中英文 shingle 与 4 段鸽巢 LSH），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:porter` Martin Porter (1980) English stemming algorithm (Steps 1a-5b) zero-alloc implementation（5/5 PASS，文档直写 `doc/lib/porter.md`）
新增 cap:porter 纯 CP 零分配自然语言处理词干库（完整实现 1980 论文全部 5 步规则并通过 60+ 向量验证），通过 cpc check 与 6/6 cap test

## 2026-11-22: 新建 `cap:olc` Google Open Location Code / Plus Codes Base-20 geo-grid encoder, decoder & recovery（5/5 PASS，文档直写 `doc/lib/olc.md`）
新增 cap:olc 纯 CP 零分配全球网格地理编码库（高精度整数网格防浮点漂移、2..15 位编码与跨象限近邻恢复），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:lfu` Zero-alloc LFU cache (LRU tie-break) & 4-bit W-TinyLFU Count-Min Sketch admission filter（5/5 PASS，文档直写 `doc/lib/lfu.md`）
新增 cap:lfu 纯 CP 零分配高频缓存与准入控制库（最低频次+LRU 破平局淘汰与 4-bit 饱和周期减半 Sketch），通过 cpc check 与 5/5 cap test

## 2026-11-22: 新建 `cap:soundex` Phonetic indexing & fuzzy name matching: NARA Soundex, Metaphone, NYSIIS & Cologne（5/5 PASS，文档直写 `doc/lib/soundex.md`）
新增 cap:soundex 纯 CP 零分配语音编码与模糊姓名匹配库（覆盖全部 4 种经典标准向量），通过 cpc check 与 6/6 cap test

