// Source: cp/lib/graph/index.cp (Unified Exports for cap:graph)
// Unified public exports for DirectedGraph, DAG/SCC/Shortest-Path Algorithms,
// Weighted GraphRAG Engine, Network Flow, Matching, and Computational Geometry (cap:graph & cap.graph).

export {
    GraphError,
    GraphEdge,
    Graph,
} from "./graph.cp";

export {
    WeightedEdge,
    DirectedGraph,
} from "./directedGraph.cp";

export {
    NO_PREV,
    ReachabilityMatrix,
    TarjanSccResult,
    PathResult,
    topologicalSort,
    findCycle,
    reachableFrom,
    transitiveClosure,
    tarjanScc,
    dijkstra,
    bellmanFord,
    reconstructPath,
} from "./dagAlgorithms.cp";

export {
    ShortestPathResult,
    SccResult,
    MstEdge,
    MstResult,
    PageRankEntry,
    GraphAlgo,
} from "./algorithms.cp";

export {
    NO_PARENT,
    isReachable,
    Edge,
    WeightedCsrGraph,
    DijkstraWorkspace,
    DijkstraResult,
    BellmanFordResult,
    FloydWarshallResult,
    dijkstra as csrDijkstra,
    reconstructPath as reconstructCsrPath,
    bellmanFord as csrBellmanFord,
    floydWarshall,
    reconstructFloydPath,
} from "./dijkstra.cp";

export {
    MST_INF_WEIGHT,
    WeightedEdge as MstWeightedEdge,
    PrimEdge,
    MstResult as DenseMstResult,
    SpanningTreeCheck,
    DisjointSet,
    kruskalMst,
    maxSpanningTree,
    primDenseMst,
    isValidSpanningTree,
} from "./mst.cp";

export {
    NO_EDGE,
    INF_FLOW,
    FlowEdge,
    FlowGraph,
    MinCostResult,
    dinicMaxFlow,
    minCutReachable,
    minCostMaxFlow,
} from "./maxflow.cp";

export {
    CsrGraph,
    TarjanWorkspace,
    TwoSatClause,
    tarjanScc as csrTarjanScc,
    topoSort,
    isDag,
    dagLongestPath,
    solve2Sat,
} from "./scc.cp";

export {
    UNMATCHED,
    BipartiteGraph,
    isBipartite,
    hopcroftKarp,
    konigMinVertexCover,
} from "./bipartite.cp";

export {
    AssignmentResult,
    AssignmentResultF64,
    AssignmentResultI64,
    HungarianWorkspace,
    HungarianWorkspaceF64,
    HungarianWorkspaceI64,
    solveMinF64,
    solveMaxF64,
    solveMinI64,
    solveMaxI64,
} from "./hungarian.cp";

export {
    Point2,
    Triangle,
    Edge2,
    Circumcircle,
    DelaunayWorkspace,
    delaunayMinTriPool,
    delaunayMinEdgePool,
    orient2d,
    circumcircle,
    inCircumcircle,
    pointInTriangle,
    triangulate,
    verifyDelaunayEmptyCircle,
    locateTriangle,
} from "./delaunay.cp";

export {
    Point2 as VoronoiPoint2,
    Bounds2 as VoronoiBounds2,
    CellMetrics,
    clipPolygonHalfPlane,
    polygonArea,
    polygonCentroid,
    computeVoronoiCell,
    lloydRelaxationStep,
    nearestSite,
} from "./voronoi.cp";

export {
    OCTREE_NO_INDEX,
    OCTREE_DEFAULT_LEAF_CAP,
    OCTREE_DEFAULT_MAX_DEPTH,
    OCTREE_MAX_DEPTH_LIMIT,
    Vec3,
    Aabb3,
    Ray3,
    RayHitRange,
    rayIntersectsAabb,
    OctreePoint,
    OctreeEntry,
    OctreeNode,
    NearestResult,
    Octree,
} from "./octree.cp";

export {
    RRT_NO_PARENT,
    Point2 as RrtPoint2,
    Bounds2 as RrtBounds2,
    CircleObstacle,
    RectObstacle,
    RrtNode,
    RrtConfig,
    RrtResult,
    pointInCircle,
    pointInRect,
    segmentIntersectsCircle,
    segmentIntersectsRect,
    isSegmentCollisionFree,
    planRrt2D,
    smoothPath,
    pathLength,
} from "./rrt.cp";

export {
    DagNodeCost,
    DagEdgeSpec,
    DagWaveAssignment,
    CriticalPathMetrics,
    WorkflowPlanSummary,
    planParallelWaves,
    analyzeCriticalPath,
    findCycleNodes,
} from "./workflowDag.cp";

export {
    FLOW_MAX_NODES,
    FLOW_MAX_EDGES,
    FLOW_NO_EDGE,
    FlowEdge as DinicFlowEdge,
    MaxFlowResult,
    BipartiteMatchPair,
    DinicFlowNetwork,
    solveBipartiteMatching,
} from "./maxFlow.cp";

export {
    SCC_MAX_NODES,
    SCC_MAX_EDGES,
    SCC_NO_EDGE,
    CondensedDagEdge,
    SccCondensationResult,
    IterativeTarjanGraph,
} from "./sccDag.cp";

export {
    HUNGARIAN_MAX_DIM,
    HUNGARIAN_UNASSIGNED,
    HUNGARIAN_FORBIDDEN_COST,
    HungarianStatus,
    HungarianAssignment,
    solveMinCostAssignment,
    solveMaxProfitAssignment,
} from "./hungarianAssign.cp";

export {
    YEN_MAX_NODES,
    YEN_MAX_EDGES,
    YEN_MAX_CANDIDATES,
    YEN_NO_EDGE,
    YEN_INF_COST,
    YenEdge,
    YenPath,
    YenGraph,
} from "./yenKPaths.cp";

export {
    MCMF_MAX_NODES,
    MCMF_MAX_EDGES,
    MCMF_NO_EDGE,
    MCMF_INF_COST,
    MCMF_INF_FLOW,
    McmfStatus,
    McmfEdge,
    McmfResult,
    McmfTransportResult,
    MinCostFlowNetwork,
    solveMinCostTransport,
} from "./minCostFlow.cp";

export {
    ARTIC_MAX_NODES,
    ARTIC_MAX_EDGES,
    ARTIC_NO_EDGE,
    ARTIC_NO_BCC,
    ArticEdge,
    ArticulationResult,
    ArticulationGraph,
} from "./articulation.cp";

export {
    DOM_MAX_NODES,
    DOM_MAX_EDGES,
    DOM_NO_NODE,
    DOM_NO_EDGE,
    DomEdge,
    DominatorResult,
    DominatorGraph,
} from "./dominator.cp";

export {
    ASTAR_MAX_NODES,
    ASTAR_MAX_EDGES,
    ASTAR_NO_NODE,
    AStarHeuristic,
    AStarStatus,
    AStarNodeCoord,
    AStarEdge,
    AStarConfig,
    AStarResult,
    computeAStarHeuristic,
    AStarGraph,
} from "./aStar.cp";
