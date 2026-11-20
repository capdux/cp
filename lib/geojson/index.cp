// Source: cp/lib/geojson/index.cp (Unified Exports for cap:geojson)
// Pure CP RFC 7946 GeoJSON Codec, Geofence Point-in-Polygon & Douglas-Peucker Simplifier.

export {
    GeoJsonError,
    Position,
    BBox,
    PolygonData,
    Geometry,
    Feature,
    FeatureCollection,
    GeoJsonDoc,
    parse,
    stringify,
    computeBBox,
    computeDocBBox,
    pointInPolygon,
    simplifyDouglasPeucker,
} from "./geojson.cp";
