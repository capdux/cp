// Source: Absorbed from /data/dev/bun/src/runtime/image/Image.cp | Synced: 2026-11-22T09:26:14Z
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/
// Upstream Sync: Refer to bun/src/runtime/image/ for future upstream updates.
// ============================================================================
export * from "./codecs.cp";
export * from "./png.cp";
export * from "./bmp.cp";
export * from "./quantize.cp";
export * from "./gif.cp";
export * from "./thumbhash.cp";
export * from "./exif.cp";
export * from "./jpeg.cp";
export * from "./webp.cp";
export * from "./pipeline.cp";

// ============================================================================
// Pure-CP Extended Image & Vector/Codecs Formats (Zero-Conflict Consolidation)
// ============================================================================
export {
    AVIF_BOX_HEADER_SIZE,
    AVIF_EXT_BOX_HEADER_SIZE,
    AvifBrandInfo,
    Av1Config,
    AvifColr,
    AvifInfo,
    isAvif,
    parseAvifBrand,
    parseAv1C,
    parseAvifInfo,
} from "./avif.cp";

export {
    EXR_MAGIC,
    EXR_FLAG_TILED,
    EXR_FLAG_LONG_NAMES,
    EXR_FLAG_NON_IMAGE_PARTS,
    EXR_FLAG_MULTIPART,
    ExrCompression,
    ExrBox2i,
    ExrAttr,
    ExrHeader,
    compressionFromByte,
    isExr,
    parseExrBox2i,
    ExrAttrIter,
    parseExrHeader,
} from "./exr.cp";

export {
    HEIF_BOX_HEADER_SIZE,
    HEIF_EXT_BOX_HEADER_SIZE,
    HEIF_HVCC_MIN_SIZE,
    HeifBrandInfo,
    HevcConfig,
    HeifColr,
    HeifInfo,
    isHeif,
    parseHeifBrand,
    parseHvcC,
    parseHeifInfo,
} from "./heif.cp";

export {
    IcoKind,
    IcoHeader,
    IcoEntry,
    isIcoOrCur,
    parseIcoHeader,
    IcoEntryIter,
    findBestIcoEntry,
} from "./ico.cp";

export {
    PnmFormat,
    PnmHeader,
    parsePnmHeader,
    pnmRawPixels,
    encodePpmP6,
    encodePgmP5,
} from "./pnm.cp";

export {
    PSD_HEADER_SIZE,
    PSD_VERSION,
    PSB_VERSION,
    PSD_RES_RESOLUTION_INFO,
    PSD_RES_ICC_PROFILE,
    PSD_RES_XMP_METADATA,
    PsdColorMode,
    PsdCompression,
    psdColorModeFromInt,
    psdCompressionFromInt,
    PsdHeader,
    PsdSections,
    PsdResource,
    isPsd,
    parsePsdHeader,
    parsePsdSections,
    PsdResourceIter,
    findPsdResource,
} from "./psd.cp";

export {
    QoiHeader,
    isQoi,
    parseQoiHeader,
    encodeQoi,
    decodeQoi,
} from "./qoi.cp";

export {
    SvgError,
    SvgPoint,
    SvgStyle,
    SvgPathBuilder,
    SvgCanvas,
} from "./svg_canvas.cp";

export {
    ChartDatum,
    LineSeries,
    SvgCharts,
} from "./svg_charts.cp";

export {
    TgaImageType,
    TgaHeader,
    parseTgaHeader,
    encodeTgaTrueColor,
    decodeTgaToRgba,
} from "./tga.cp";

export {
    TiffError,
    TiffEndian,
    TiffCompression,
    TiffPixelFormat,
    TiffPage,
    TiffDocument,
    encodeSingle,
    encodeMultiPage,
    decode,
    decodeWithLimits,
} from "./tiff.cp";

export {
    DICOM_PREAMBLE_SIZE,
    DICOM_MAGIC,
    DICOM_DATA_OFFSET,
    DICOM_UNDEFINED_LEN,
    TS_IMPLICIT_VR_LE,
    TS_EXPLICIT_VR_LE,
    TS_EXPLICIT_VR_BE,
    DicomTag,
    TAG_TRANSFER_SYNTAX,
    TAG_PATIENT_NAME,
    TAG_PATIENT_ID,
    TAG_MODALITY,
    TAG_ROWS,
    TAG_COLUMNS,
    TAG_BITS_ALLOCATED,
    TAG_PIXEL_DATA,
    DicomElement,
    DicomIter,
    DicomSummary,
    isDicom,
    parseDicomSummary,
} from "./dicom.cp";

export {
    BarcodeError,
    BarcodeFormat,
    BarcodeBitPattern,
    BarcodeEncoder,
    BarcodeRenderer,
} from "./barcode.cp";

export {
    QR_GF_PRIMITIVE,
    ReedSolomon,
} from "./qrcode_ecc.cp";

export {
    QR_MIN_VERSION,
    QR_MAX_VERSION,
    QrError,
    QrEccLevel,
    QrMatrix,
    QrCode,
} from "./qrcode_encoder.cp";

export {
    QrRenderer,
} from "./qrcode_render.cp";

export {
    JxlContainerKind,
    JxlSizeHeader,
    JxlBox,
    JxlInfo,
    isJxlCodestream,
    isJxlContainer,
    detectJxlKind,
    isJxl,
    parseJxlSizeHeader,
    JxlBoxIter,
    parseJxlInfo,
} from "./jxl.cp";

export {
    XCF_MAGIC,
    PROP_END,
    PROP_COLORMAP,
    PROP_ACTIVE_LAYER,
    PROP_OPACITY,
    PROP_VISIBLE,
    PROP_COMPRESSION,
    PROP_RESOLUTION,
    PROP_PARASITES,
    PROP_UNIT,
    PROP_PATHS,
    XcfBaseType,
    XcfCompression,
    XcfPrecision,
    XcfHeader,
    XcfProperty,
    XcfPropIter,
    XcfPointerIter,
    XcfLayerInfo,
    parseXcfCompression,
    isXcf,
    parseXcfHeader,
    parseXcfLayerHeader,
} from "./xcf.cp";
