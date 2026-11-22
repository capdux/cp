// Source: cp/lib/gfx/index.cp (Unified Public Exports for cap:gfx)
// Pure CP graphics, fonts, GPU textures, ICC color, computational geometry, collision, procedural noise, quaternions, ECS, pathfinding & 3D asset parsers.

// --- ttf ---
export {
    SfntFlavor,
    SfntOffsetTable,
    SfntTableRecord,
    TtfHeadMetrics,
    parseSfntHeader,
    SfntTableIter,
    findSfntTable,
    parseTtfHead,
    parseTtfNumGlyphs,
    calcSfntChecksum,
} from "./ttf.cp";

// --- woff2 ---
export {
    WOFF2_HEADER_SIZE,
    WOFF2_SIGNATURE,
    WOFF2_FLAVOR_TRUETYPE,
    WOFF2_FLAVOR_CFF,
    WOFF2_FLAVOR_COLLECTION,
    Woff2Header,
    UIntBase128Result,
    Woff2TableEntry,
    decodeUIntBase128,
    isWoff2,
    parseWoff2Header,
    Woff2TableIter,
} from "./woff2.cp";

// --- eot ---
export {
    EOT_MAGIC,
    EOT_FIXED_HEADER_SIZE,
    EOT_VERSION_1,
    EOT_VERSION_2_1,
    EOT_VERSION_2_2,
    EOT_FLAG_COMPRESSED,
    EOT_FLAG_SUBSET,
    EOT_FLAG_XOR_ENCRYPTED,
    EotHeader,
    isEot,
    parseEotHeader,
    decodeUtf16LeName,
} from "./eot.cp";

// --- bdf ---
export {
    BdfBBox,
    BdfHeader,
    BdfProperty,
    BdfGlyph,
    BdfPropIter,
    BdfGlyphIter,
    isBdf,
    parseBdfHeader,
    bdfGlyphPixel,
    findBdfGlyph,
} from "./bdf.cp";

// --- pcf ---
export {
    PCF_MAGIC,
    PCF_TOC_ENTRY_SIZE,
    PCF_PROPERTIES,
    PCF_ACCELERATORS,
    PCF_METRICS,
    PCF_BITMAPS,
    PCF_INK_METRICS,
    PCF_BDF_ENCODINGS,
    PCF_SWIDTHS,
    PCF_GLYPH_NAMES,
    PCF_BDF_ACCELERATORS,
    PCF_DEFAULT_FORMAT,
    PCF_COMPRESSED_METRICS,
    PCF_BYTE_MASK,
    PCF_BIT_MASK,
    PCF_GLYPH_PAD_MASK,
    PcfTocEntry,
    PcfMetrics,
    PcfEncodingsHeader,
    PcfProperty,
    PcfTocIter,
    PcfPropIter,
    isPcf,
    findPcfTable,
    parsePcfMetricsCount,
    readPcfMetric,
    parsePcfEncodings,
    lookupPcfGlyphIndex,
} from "./pcf.cp";

// --- psf ---
export {
    PSF1_MAGIC0,
    PSF1_MAGIC1,
    PSF1_HEADER_SIZE,
    PSF1_MODE512,
    PSF1_MODEHASTAB,
    PSF1_MODEHASSEQ,
    PSF1_SEPARATOR,
    PSF1_STARTSEQ,
    PSF2_MAGIC,
    PSF2_HEADER_SIZE,
    PSF2_HAS_UNICODE_TABLE,
    PSF2_SEPARATOR,
    PSF2_STARTSEQ,
    PsfVersion,
    PsfHeader,
    detectPsfVersion,
    isPsf,
    parsePsfHeader,
    psfGlyphBitmap,
    psfGlyphPixel,
    findPsfGlyphByCodepoint,
} from "./psf.cp";

// --- dds ---
export {
    DdsPixelFormat,
    DdsDx10Header,
    DdsHeader,
    isDds,
    parseDdsHeader,
    calcDdsMipSize,
} from "./dds.cp";

// --- ktx ---
export {
    KtxVersion,
    Ktx1Header,
    Ktx2Header,
    Ktx2Level,
    isKtx1,
    isKtx2,
    detectKtxVersion,
    parseKtx1Header,
    parseKtx2Header,
    readKtx2Level,
} from "./ktx.cp";

// --- icc ---
export {
    ICC_HEADER_SIZE,
    ICC_TAG_TABLE_OFFSET,
    ICC_TAG_ENTRY_SIZE,
    IccHeader,
    IccTagEntry,
    iccTagEquals,
    isIccProfile,
    parseIccHeader,
    IccTagIter,
    findIccTag,
} from "./icc.cp";

// --- bezier ---
export {
    Vec2,
    BBox2,
    QuadPair,
    CubicPair,
    QuadBezier,
    CubicBezier,
    tAtArcLength,
} from "./bezier.cp";

// --- earcut ---
export {
    Point2D,
    TriangleIdx,
    signedArea,
    polygonArea,
    isCounterClockwise,
    isConvex,
    pointInTriangle,
    pointInPolygon,
    triangulate,
    triangulationTotalArea,
} from "./earcut.cp";

// --- gjk ---
export {
    SatContact,
    GjkResult,
    polygonCentroid,
    translatePolygon,
    polygonSupport,
    minkowskiSupport,
    gjkIntersects,
    satCollide,
    circlePolygonCollide,
} from "./gjk.cp";

// --- perlin ---
export {
    fade,
    lerp,
    PerlinNoise,
} from "./perlin.cp";

// --- quat ---
export {
    Vec3,
    EulerAngles,
    Mat3,
    Quat,
} from "./quat.cp";

// --- ecs ---
export {
    EcsError,
    EntityId,
    EntityRegistry,
    ComponentPool,
    EcsQuery,
} from "./ecs.cp";

// --- astar ---
export {
    COST_CARDINAL,
    COST_DIAGONAL,
    GridPos,
    AstarResult,
    AstarWorkspace,
    manhattanHeuristic,
    octileHeuristic,
    astarFindPath,
    hasLineOfSight,
} from "./astar.cp";

// --- pathfinding ---
export {
    PathError,
    HeuristicKind,
    GridCoord,
    PathResult,
    GridMap,
} from "./pathfinding.cp";

// --- gltf ---
export {
    GLB_MAGIC,
    GLB_VERSION_2,
    CHUNK_TYPE_JSON,
    CHUNK_TYPE_BIN,
    COMPONENT_BYTE,
    COMPONENT_UNSIGNED_BYTE,
    COMPONENT_SHORT,
    COMPONENT_UNSIGNED_SHORT,
    COMPONENT_UNSIGNED_INT,
    COMPONENT_FLOAT,
    ChunkType,
    AccessorType,
    GlbHeader,
    GlbChunk,
    GlbDocument,
    isGlb,
    parseGlbHeader,
    GlbChunkIter,
    extractJsonChunk,
    extractBinChunk,
    parseGlb,
    componentByteSize,
    accessorComponentCount,
    parseAccessorType,
    accessorElementStride,
} from "./gltf.cp";

// --- fbx ---
export {
    FBX_MAGIC,
    FBX_HEADER_SIZE,
    FBX_V7500_THRESHOLD,
    FbxHeader,
    FbxNode,
    FbxProp,
    FbxNodeIter,
    FbxPropIter,
    isFbxBinary,
    parseFbxHeader,
} from "./fbx.cp";

// --- bvh ---
export {
    BvhChannel,
    BvhJoint,
    BvhHeader,
    parseBvhChannel,
    isBvh,
    BvhJointIter,
    parseBvhHeader,
    BvhFloatIter,
    BvhFrameIter,
} from "./bvh.cp";

// --- dae ---
export {
    DaeAsset,
    DaeGeometry,
    DaeFloatArray,
    DaeSceneNode,
    DaeStats,
    DaeGeometryIter,
    DaeFloatArrayIter,
    DaeFloatIter,
    DaeSceneNodeIter,
    isCollada,
    parseDaeAsset,
    summarizeDae,
} from "./dae.cp";

// --- usd ---
export {
    USDC_MAGIC,
    USDC_BOOTSTRAP_SIZE,
    USDC_SECTION_NAME_LEN,
    USDC_SECTION_ENTRY_SIZE,
    USDA_MAGIC,
    UsdFormat,
    UsdcBootstrap,
    UsdcSection,
    UsdaHeader,
    UsdcSectionIter,
    isUsdc,
    isUsda,
    isUsdz,
    detectUsdFormat,
    parseUsdcBootstrap,
    findUsdcSection,
    parseUsdaHeader,
} from "./usd.cp";

// --- vrml ---
export {
    VrmlFlavor,
    VrmlTokenKind,
    VrmlHeader,
    VrmlToken,
    VrmlNode,
    VrmlStats,
    parseVrmlHeader,
    isVrml,
    VrmlLexer,
    VrmlNodeIter,
    findVrmlField,
    summarizeVrml,
} from "./vrml.cp";

// --- x3d ---
export {
    X3dHeader,
    X3dMeta,
    X3dNode,
    X3dStats,
    isX3d,
    parseX3dHeader,
    X3dMetaIter,
    findX3dMeta,
    X3dNodeIter,
    X3dFloatIter,
    X3dIntIter,
    summarizeX3d,
} from "./x3d.cp";

// --- noise ---
export {
    NoiseError,
    FractalConfig,
    NoiseGen,
} from "./noise.cp";

