# CP 标准库：图形渲染、字体排版、计算几何与游戏引擎基础套件手册 (`cap:gfx`)

> 定位：纯 CP 零 C 依赖图形/字体/GPU 纹理/色彩/计算几何/碰撞检测/噪声/四元数/ECS/寻路/3D 资产解析统一标准库。

## 一、 架构总览

`cap:gfx` 将原本分散在根目录的 25 个纯 CP 图形与游戏引擎基础小模块统一收敛至 `lib/gfx/`，通过 `lib/gfx/index.cp` 提供零符号冲突的统一导出（`import { ... } from "cap:gfx"` 与 `import cap from "cap"; cap.gfx.*`）。

- **统一 2D 向量类型 (`Vec2`)**：`bezier.cp` 与 `gjk.cp` 共享同一 `Vec2` 结构体（含 `add`/`sub`/`scale`/`neg`/`dot`/`cross`/`lenSq`/`len`/`dist`/`lerp`/`normalize`/`perp`），支持贝塞尔曲线几何与 GJK/SAT 凸包碰撞检测无缝互操作。
- **零外部依赖、零 Bun 资产冲突**：全部为纯 CP 零分配或显式 Allocator 确定性实现。

---

## 二、 模块清单与核心 API 摘要

| 子模块文件 | 原目录 | 核心导出符号与职责 |
| :--- | :--- | :--- |
| `lib/gfx/ttf.cp` | `lib/ttf` | `SfntFlavor`, `SfntOffsetTable`, `SfntTableRecord`, `TtfHeadMetrics`, `parseSfntHeader`, `SfntTableIter`, `findSfntTable`, `parseTtfHead`, `parseTtfNumGlyphs`, `calcSfntChecksum`（TrueType / OpenType SFNT 字体解析） |
| `lib/gfx/woff2.cp` | `lib/woff2` | `Woff2Header`, `Woff2TableEntry`, `decodeUIntBase128`, `isWoff2`, `parseWoff2Header`, `Woff2TableIter`（W3C WOFF2 字体头与表目录解析） |
| `lib/gfx/eot.cp` | `lib/eot` | `EotHeader`, `isEot`, `parseEot` 等（Microsoft Embedded OpenType `.eot` 字体解析） |
| `lib/gfx/bdf.cp` | `lib/bdf` | `BdfHeader`, `BdfGlyph`, `parseBdfHeader`, `BdfGlyphIter` 等（Adobe BDF 2.1/2.2 点阵字体解析） |
| `lib/gfx/pcf.cp` | `lib/pcf` | `PcfHeader`, `PcfTableEntry`, `parsePcfHeader`, `PcfTableIter` 等（X11 Portable Compiled Format `.pcf` 字体解析） |
| `lib/gfx/psf.cp` | `lib/psf` | `PsfHeader`, `PsfVersion`, `parsePsfHeader`, `psfGlyphBitmap` 等（Linux Console PSF1/PSF2 点阵字体解析） |
| `lib/gfx/dds.cp` | `lib/dds` | `DdsHeader`, `DdsPixelFormat`, `DdsHeaderDx10`, `parseDds` 等（DirectDraw Surface DDS GPU 纹理容器解析） |
| `lib/gfx/ktx.cp` | `lib/ktx` | `Ktx1Header`, `Ktx2Header`, `Ktx2LevelIndex`, `parseKtx1`, `parseKtx2` 等（Khronos KTX 1.1 / KTX2 GPU 纹理容器解析） |
| `lib/gfx/icc.cp` | `lib/icc` | `IccHeader`, `IccTagEntry`, `parseIccHeader`, `IccTagIter` 等（ICC 色彩配置文件解析） |
| `lib/gfx/bezier.cp` | `lib/bezier` | `Vec2`, `BBox2`, `QuadBezier`, `CubicBezier` 等（2D 二次/三次贝塞尔曲线求值、切线、细分、包围盒与弧长） |
| `lib/gfx/earcut.cp` | `lib/earcut` | `EarcutPoint`, `earcutTriangulate` 等（2D 简单多边形耳切法三角剖分与面积计算） |
| `lib/gfx/gjk.cp` | `lib/gjk` | `SatContact`, `GjkResult`, `gjkIntersects`, `satConvexPolygons` 等（2D GJK 凸包碰撞检测、Minkowski 支撑与 SAT 穿透流形，共享 `bezier.cp` 的 `Vec2`） |
| `lib/gfx/perlin.cp` | `lib/perlin` | `PerlinNoise`, `perlin2d`, `perlin3d`, `fbm2d` 等（Perlin 程序化噪声与分形布朗运动） |
| `lib/gfx/quat.cp` | `lib/quat` | `Vec3`, `EulerAngles`, `Mat3`, `Quat`（3D 单位四元数旋转、欧拉角/矩阵互转与 SLERP 球面插值） |
| `lib/gfx/ecs.cp` | `lib/ecs` | `EcsError`, `Entity`, `World` 等（稀疏集 Sparse-Set Entity Component System） |
| `lib/gfx/astar.cp` | `lib/astar` | `GridPos`, `AstarResult`, `AstarWorkspace`, `manhattanHeuristic`, `octileHeuristic`, `astarFindPath`, `hasLineOfSight`（零分配 2D 网格 A* 寻路） |
| `lib/gfx/pathfinding.cp` | `lib/pathfinding` | `PathError`, `HeuristicKind`, `GridCoord`, `PathResult`, `GridMap`（动态 2D 网格地图、A* 搜索与 Dijkstra 距离场） |
| `lib/gfx/gltf.cp` | `lib/gltf` | `GlbHeader`, `GlbChunk`, `parseGlbHeader`, `GlbChunkIter` 等（glTF 2.0 / GLB 3D 资产容器解析） |
| `lib/gfx/fbx.cp` | `lib/fbx` | `FbxHeader`, `FbxNodeHeader`, `parseFbxHeader` 等（Autodesk FBX 7100-7700 二进制 3D 资产解析） |
| `lib/gfx/bvh.cp` | `lib/bvh` | `BvhHeader`, `BvhJoint`, `parseBvh` 等（Biovision Hierarchy BVH 骨骼动作捕捉解析） |
| `lib/gfx/dae.cp` | `lib/dae` | `DaeAsset`, `DaeGeometry`, `DaeFloatArray`, `DaeSceneNode`, `DaeStats`, `isCollada`, `parseDaeAsset`, `summarizeDae` 等（零分配 Khronos COLLADA 1.4/1.5 3D 资产 XML 解析） |
| `lib/gfx/usd.cp` | `lib/usd` | `UsdFormat`, `UsdcBootstrap`, `UsdcSection`, `UsdaHeader`, `detectUsdFormat`, `parseUsdcBootstrap`, `findUsdcSection`, `parseUsdaHeader` 等（零分配 Pixar OpenUSD `.usdc`/`.usda`/`.usdz` 解析） |
| `lib/gfx/vrml.cp` | `lib/vrml` | `VrmlFlavor`, `VrmlHeader`, `VrmlToken`, `VrmlNode`, `VrmlStats`, `VrmlLexer`, `VrmlNodeIter`, `summarizeVrml` 等（零分配 VRML97 & Classic X3D 场景解析） |
| `lib/gfx/x3d.cp` | `lib/x3d` | `X3dHeader`, `X3dMeta`, `X3dNode`, `X3dStats`, `isX3d`, `parseX3dHeader`, `X3dNodeIter`, `summarizeX3d` 等（零分配 Web3D X3D 3.x/4.0 XML 场景解析） |
| `lib/gfx/noise.cp` | `lib/noise` | `NoiseError`, `FractalConfig`, `NoiseGen`（确定性 2D/3D Perlin、fBm、Ridged、Worley 噪声与高度图网格生成） |

---

## 三、 使用示例

```typescript
import {
    Vec2,
    CubicBezier,
    gjkIntersects,
    Quat,
    Vec3,
    PerlinNoise,
    GridMap,
} from "cap:gfx";

pub fn demoGfx() {
    // 1. 贝塞尔曲线与 GJK 碰撞共享同一 Vec2
    const curve = CubicBezier {
        p0: Vec2 { x: 0.0, y: 0.0 },
        p1: Vec2 { x: 1.0, y: 2.0 },
        p2: Vec2 { x: 3.0, y: 2.0 },
        p3: Vec2 { x: 4.0, y: 0.0 },
    };
    const mid = curve.eval(0.5);

    // 2. 四元数 3D 旋转
    const q = Quat.fromAxisAngle(Vec3 { x: 0.0, y: 1.0, z: 0.0 }, 1.5707963267948966);
    const v = q.rotateVec3(Vec3 { x: 1.0, y: 0.0, z: 0.0 });

    * = mid;
    * = v;
}
```

---

## 收敛子模块深度文档

- [astar.md](gfx/astar.md)
- [bdf.md](gfx/bdf.md)
- [bezier.md](gfx/bezier.md)
- [bvh.md](gfx/bvh.md)
- [dae.md](gfx/dae.md)
- [dds.md](gfx/dds.md)
- [earcut.md](gfx/earcut.md)
- [ecs.md](gfx/ecs.md)
- [eot.md](gfx/eot.md)
- [fbx.md](gfx/fbx.md)
- [gjk.md](gfx/gjk.md)
- [gltf.md](gfx/gltf.md)
- [icc.md](gfx/icc.md)
- [ktx.md](gfx/ktx.md)
- [noise.md](gfx/noise.md)
- [pathfinding.md](gfx/pathfinding.md)
- [pcf.md](gfx/pcf.md)
- [perlin.md](gfx/perlin.md)
- [psf.md](gfx/psf.md)
- [quat.md](gfx/quat.md)
- [ttf.md](gfx/ttf.md)
- [usd.md](gfx/usd.md)
- [vrml.md](gfx/vrml.md)
- [woff2.md](gfx/woff2.md)
- [x3d.md](gfx/x3d.md)
