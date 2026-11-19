## cap按需编译构建设计

### 正确的 cap 跨平台构建架构

#### 1. Zig 的启示：构建图的组合 (Build Graph Composition)

Zig 的构建系统确实强大，是因为它采用了**“可编程构建图”**。

- 在 Zig 中，build.zig 本质上是一个构建任务的 DAG（有向无环图）。
- 当你在一个 package 中添加 lib.linkSystemLibrary("z") 时，这个依赖只被挂载在当前包的节点上。
- 当你的应用项目 import 这个包时，build.zig 的 DAG 会递归遍历，自动把这个包需要的 zlib 链接参数（Include 路径、Lib 路径、Link 命令）沿 DAG 冒泡（Aggregate）上来，最终在根构建节点（Application）汇总。
- 完整覆盖了从配置、编译、链接到运行时的全链路安全。
- 尊重了不同操作系统间二进制库的不一致性，通过“显式映射”而非“隐式猜测”解决了最难的跨平台问题。
- 工程落地 (Practical)：与 npm/bun 的依赖语义一致，降低了迁移门槛，同时保留了 Native 构建追求的确定性。

#### 2. cap 的进化路径：从“全局硬编码”到“声明式依赖透传”

为了达到你“import 即可用，无需手动配置链接”，cap 需要执行以下架构重构：

- 元数据驱动依赖（Manifest-Driven）：cp下的几种形态：cab/lib 内置库（无需安装，内置）、外部库（需cap安装）和项目，三种形态都需要cap.json声明它自身的构建需求：

```json
{
  "name": "project_name",
  "version": "0.1.0",

  // 1. 系统/动态依赖 (用于构建期检查 + 运行期环境防护)
  "sys_dependencies": {
    "zlib": {
      "linux": { "lib": "libz.so", "version": ">=1.2.11" },
      "macos": { "lib": "libz.dylib", "version": ">=1.2.11" }
    }
  },
  // 2. 用于链接静态库 (.a) 或触发本地 C/C++ 源码编译
  "bin_dependencies": {
    "quickjs": {
      "type": "static_lib",
      "path": "vendor/quickjs",
      "build_cmd": "make -C vendor/quickjs libqjs.a"
    }
  },
  // 3. 外部依赖包 (npm 风格，通过 cap 安装)
  "dependencies": {
    "kitesurf": "^1.0.0",
    "lexbor": "^2.3.0"
  },
  // 4. 内部库路径定义 (默认指向 ~/.cap/lib)
  "cap_lib": "~/.cap/lib"
}
```

`cap_lib`: 内置库路径定义, 不定义, 走缺省路径, 当前 cap路径为 ~/.cap/bin, lib 就应该为 ~/.cap/lib, 没有这个路径, cap无法按需编译内部库。
`sys_dependencies`：动态链接，依赖操作系统，用于编译和运行期动态检查，编译期动态链接。
`bin_dependencies`：静态链接，通过定义，可实现c跨平台编译输出静态链接库，最终合并入 bin。这使得 cap.json 的配置逻辑彻底清晰：凡是会改变最终生成物大小、涉及二进制链接的行为，全部归类到 bin_dependencies 中。

`lib/cap.json`：是所有内置库的集中定义（需简洁），内置库通过 `cap:xx`引用， 内置引用时，读取`lib/cap.json`, 实现按需编译（动态、静态）。同时，可考虑在其中实现引用名称与实际文件的映射。无静态、动态编译和映射时，无需在其中定义，直接按 目录/index.cp或文件引用。

#### 静态编译（bin依赖）

- 构建编排 (cap)：
  - 解析 cap.json -> 建立 DAG -> 识别 sys 和 bin 边界。
  - 对每个节点触发 cpc (CP转译) 和 zig (原生链接)。
- cap 在跨平台编译时，通过匹配当前交叉编译时，查找 bin依赖 中是否含有对应的二进制预构建产物。
- 若有二进制：直接链接。
- 若无二进制但有源码：触发 build_cmd（zig编译）。
- 若两者皆无：cap 报错，禁止构建，强制要求开发者在 cap.json 中补齐该平台的编译规则。这杜绝了“猜谜式构建”。
- 结果输出：生成一个完全自包含的 bin/ 产物，或者一个只依赖极少量系统 libc 的精简二进制。

### Transitive Dependency Resolution (递归解析)：

当你的 src/main.cp 导入 `cap:zlib` 或 `http` 时：

1. cpc 解析到 import { zlib } from "cap:zlib"、 import { url } from "http" 。
2. cap 工具链在解析构建图时，自动读取 lib/cap.json 和 cap_modules/http/cap.json。
3. cap 会将 内部zlib和外部http库的 `bin sys` 加入到当前编译单元的编译任务中，将其 include 加入到 zig 的头文件搜索路径，或者需要静态链接的二进制库，或者操作系统库的动态引用，总之zig需要的，cap.json必须明确提供，避免猜测到处搜索。
4. 引用的引用，按此方法一直递归下去。
5. 最终产物：所有bin依赖最终编译为 .o 或.a 产物，用于静态链接。
6. sys动态引用，全部收集后， 用于编译和运行时跨操作系统检测，没有就报错退出运行，避免后续运行崩溃。
7. 构建优化：只有被 DAG 遍历到的节点（Library 或 C Vendor 映射）才会被触发编译。其他未引用的庞大 lib 库（即便在 lib/目录下）完全不会产生任何二进制产物，实现了极致的 Tree-Shaking。

### 这意味着什么？

1. 用户视角：再也不用维护 link_libs，只需要关注 import。
2. CP 内置库视角：cap 作为一个包管理器，应该内置 zlib 的 CP 原生实现（纯 CP 实现，无需链接 C 动态库） 或提供
   跨平台预编译的二进制静态包（包含所有必要的构建配置）或动态引用（编译和运行时，均需检查当前操作系统是否存在）。

### 库的“树摇”：按需构建 DAG

方案：编译即搜索，链接即汇总

目前的 cap 采用“先编译全部，再链接全部”的错误模式。我们需要将其切换为 “基于导入图的惰性链接（Lazy Linking）”：

1.  阶段一：静态分析导入图 (cpc 的职责)
    - 当运行 cap build 时，cpc 首先扫描 src/main.cp 的所有递归 import，生成一个完整的 “符号需求树（Symbol Request Tree）”。
2.  阶段二：动态构建构建图 (cap 的职责)
    - cap 根据 Import Tree，只激活在这个树上出现的节点（Modules）。
    - 如果应用没用到 cap:crypto，该库的所有 C 代码、Zig 代码、链接指令压根不会进入 zig 的构建命令行参数。
3.  阶段三：摇树
    类似es6、ts的摇树， 一个文件， 编译时只包含引用的export部分代码， 其他未用代码，不包含。

### 下一步行动建议

我不建议立刻动整个构建系统，我们应该增量替换：

1. 优先处理 capsurf 依赖：我们先为 kitesurf 和 quickjs 这两个最麻烦的 vendor，定义一套 “内置构建规则索引”。
2. 实现 cap 的依赖解析逻辑：将 cap build 从“遍历所有目录”改为“从 main.cp 出发，递归扫描导入路径，仅编译已引用的包”。
   这个思路是否符合你对 CP 系统“高效、严谨、确定性”的要求？如果是，我将先写一个原型脚本，测试 cap 递归解析导入图并生成最小构建集的可行性。
3. 引用库的时候, 需区分 内部库, 还是外部库, 内部库 cap:xxx, 带有 专属的 cap:前缀, 外部库没有, 可以区分
4. 内部库, 在 cp/lib 目录, 创建一个 cap.json 文件, 记录每个库编译时的依赖关系(编译好的,跨平台的二进制产物,
   需区分操作系统,c代码,zig完成跨平台编译)
5. 外部库, 需要cap安装，类似npm，有自己的 cap.json, 里面也有类似的编译依赖关系.
6. 无论内部库还是外部库, 均可以通过 import 引用,建立 编译构件图(重复引用,则去重), 按需编译, 确保生成的文件最小.
7. 对同一个依赖库, 可能存在多个版本, 不同库 依赖不同版本, 需引入`cap.lock`(Lockfile)机制：
   版本协调策略：

- 若不同依赖库需求版本存在重叠（SemVer 兼容），cap 自动提升到最高兼容版本。
- 若版本彻底不兼容，cap 必须在构建前 报错停止，禁止进行不确定性的构建，绝不尝试通过“多版本名混淆”来解决，因为那样会指数级膨胀二进制体积。

8. 若需生成的文件最小（small），cap 的库也要类似zig, 除被预先编译好的 .a库, 还需 zig/c源代码, 实现按需编译。
9. 以上规则主要是c、zig代码，cp则无需sys和bin，直接按需编译，动态摇树。
10. 很多知名库, 独立通过系统安装, cp可以动态引用, 不增加生成文件大小,这些库的封装者,
    需明确告知用户需安装什么库,什么版本,实现动态引用(非静态链接编译)，cap收集后，编译和运行时，进行检查，没有则报错退出运行，坚决不把崩溃留在运行期。

### 3. 源优先编译 (Source-First Build)

• 按需编译 (Tree-Shaking)：cpc 编译器必须将 cap 库视为“可被修剪的”。
• 实现：cap 工具在调用 cpc 编译前，应利用静态分析扫描出实际被引用的 export 符号，只对涉及的函数进行编译，未引用代码在编译期直接丢弃。
• 这完全依赖于你提到的“必须包含源码”这一前提，若包含的是预编译好的 .a，则无法完成 Tree-Shaking。

### 跨平台版本共存策略 (Lockfile)

• 引入 cap.lock 文件，由 cap 在执行 cap install 或 cap build 时自动更新。
• cap.lock 记录了当前 DAG 的完整依赖版本链，确保在任何机器上执行 cap build，得到的构建图完全一致。若发生版本冲突，cap 强制锁定版本或在构建前通过 cap update 让你人工决策。

### 构建逻辑的重新审核与确认

cap 的构建引擎将按照以下闭环流程运行：

1. 路径与解析：
   - cap 首先寻找 cap.json。若未显式指定 cap_lib_path，自动重定向至 ~/.cap/lib（此处存放了 CP 内置核心库的符号链接）。
2. 依赖全量聚合 (Dependency Flattening)：
   - cap 递归解析 import 图，并从 当前项目路径、`cap_modules/` 和 `cp/lib/` 载入各自的 `cap.json`。
   - 全量收集：它会将所有依赖项（项目 + 内部库 + 外部库）中的 `sys_dependencies` 和 `bin_dependencies` 进行去重、合并。
3. 确定性构建 (Deterministic Build)：
   - sys 检查：构建前，cap 遍历 `sys_dependencies`，检查当前宿主机环境是否满足需求，不满足直接中止，绝不尝试隐式链接。
   - bin 链接：优先检查 `bin_dependencies` 路径下是否已有预编译的二进制静态库（.a）。若存在，直接链接；若不存在（或设置了 force_build: true），则执行 build_cmd 编译源码。
4. 按需编译 (Tree-Shaking)：
   - 无论库多庞大，cap 仅将“构建图中明确被 import 的路径”对应的源码加入到 zig build 任务序列中。对于未引用的库及其对应的
   - `bin_dependencies`，即使其存在于 cap.json 中，也不会进入链接阶段。
5. 运行时保障：
   - 生成的最终二进制包含一个元数据头（由 cap 自动注入），记录了 `sys_dependencies` 的所有需求。应用启动时，CP 的 Runtime会优先核对这些库是否存在，从而在运行前完成环境自检。

### 总结

“确定性包管理 + 符号级按需编译” 的模型。这不仅能解决 zlib 的问题，更解决了 capsurf 作为复杂应用在不同平台上构建的一致性风险。

我们将不再修改现有的 main.zig 链接参数，而是开始撰写 cap 的 “依赖解析器逻辑原型”。这个原型将负责读取 main.cp，递归扫描 import路径，并生成一个该应用所需的 最小源码编译表。
