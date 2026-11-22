// Origin: Pure-CP Stdlib (Zero-Alloc Unidata NetCDF CDF-1/CDF-2/CDF-5 Parser) | Created: 2026-11-22

export {
    NC_MAGIC,
    NC_ABSENT,
    NC_DIMENSION,
    NC_VARIABLE,
    NC_ATTRIBUTE,
    NC_UNLIMITED,
    NetcdfFormat,
    NetcdfType,
    netcdfTypeSize,
    decodeNetcdfType,
    NetcdfDim,
    NetcdfAttr,
    NetcdfVar,
    NetcdfHeader,
    detectNetcdfFormat,
    isNetcdf,
    parseNetcdfHeader,
    NetcdfDimIter,
    NetcdfAttrIter,
    NetcdfVarIter,
} from "./netcdf.cp";
