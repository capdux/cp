// Origin: Pure-CP Stdlib (Zero-Alloc DICOM PS3.10 Medical Imaging Data Element Parser) | Created: 2026-11-22
// Unified exports for cap:dicom.

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
