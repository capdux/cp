// Source: cp/lib/science/index.cp (Unified Exports for cap:science)
// Unified Public Exports for Bioinformatics, Computational Chemistry, Biosignal, LiDAR, Control & 3D Scientific Mesh Formats (cap:science & cap.science)

// --- Bioinformatics & Genomics (FASTA, FASTQ, SAM, VCF, GFF3/GTF2, BED) ---

export {
    FastaRecord,
    FastaStats,
    FastaIter,
    isFasta,
    dnaComplement,
    reverseComplementDna,
    summarizeFasta,
} from "./fasta.cp";

export {
    PHRED_OFFSET_33,
    PHRED_OFFSET_64,
    FastqRecord,
    FastqStats,
    FastqIter,
    isFastq,
    summarizeFastq,
} from "./fastq.cp";

export {
    SAM_FLAG_PAIRED,
    SAM_FLAG_PROPER_PAIR,
    SAM_FLAG_UNMAPPED,
    SAM_FLAG_MATE_UNMAPPED,
    SAM_FLAG_REVERSE,
    SAM_FLAG_MATE_REVERSE,
    SAM_FLAG_READ1,
    SAM_FLAG_READ2,
    SAM_FLAG_SECONDARY,
    SAM_FLAG_QC_FAIL,
    SAM_FLAG_DUPLICATE,
    SAM_FLAG_SUPPLEMENTARY,
    CigarOp,
    SamHeaderLine,
    SamRecord,
    SamStats,
    CigarIter,
    SamHeaderIter,
    SamRecordIter,
    cigarRefLen,
    cigarQueryLen,
    findSamHeaderTag,
    parseSamLine,
    summarizeSam,
} from "./sam.cp";

export {
    VcfHeader,
    VcfRecord,
    VcfStats,
    isVcf,
    parseVcfHeader,
    VcfRecordIter,
    summarizeVcf,
} from "./vcf.cp";

export {
    GFF_PHASE_NONE,
    GffRecord,
    GffStats,
    GffIter,
    isGff3,
    parseGffLine,
    summarizeGff,
} from "./gff.cp";

export {
    BedStrand,
    BedRecord,
    BedStats,
    BedIter,
    parseBedLine,
    summarizeBed,
} from "./bed.cp";

// --- Computational Chemistry & Structural Biology (PDB, CIF/mmCIF, MOL2, SDF, InChI) ---

export {
    PdbHeader,
    PdbAtom,
    PdbConect,
    PdbStats,
    PdbAtomIter,
    PdbConectIter,
    isPdb,
    parsePdbHeader,
    parsePdbAtomLine,
    parsePdbConectLine,
    summarizePdb,
} from "./pdb.cp";

export {
    CifTokenKind,
    CifToken,
    CifEntry,
    CifStats,
    isCif,
    CifLexer,
    CifPairIter,
    findCifTag,
    CifRow,
    CifRowIter,
    CifLoop,
    CifLoopIter,
    summarizeCif,
} from "./cif.cp";

export {
    MOL2_RTI_MOLECULE,
    MOL2_RTI_ATOM,
    MOL2_RTI_BOND,
    MOL2_RTI_SUBSTRUCTURE,
    Mol2Header,
    Mol2Atom,
    Mol2Bond,
    Mol2Stats,
    Mol2AtomIter,
    Mol2BondIter,
    Mol2RecordIter,
    isMol2,
    parseMol2Header,
    parseMol2AtomLine,
    parseMol2BondLine,
    summarizeMol2,
} from "./mol2.cp";

export {
    SdfMolHeader,
    SdfAtom,
    SdfBond,
    SdfProperty,
    SdfStats,
    SdfAtomIter,
    SdfBondIter,
    SdfPropIter,
    SdfRecordIter,
    isSdf,
    parseMolHeader,
    findSdfProp,
    summarizeSdf,
} from "./sdf.cp";

export {
    INCHI_PREFIX,
    INCHI_KEY_LEN,
    InchiLayerKind,
    InchiLayer,
    InchiParsed,
    FormulaElement,
    InchiKey,
    InchiLayerIter,
    FormulaIter,
    isInchi,
    parseInchi,
    parseInchiKey,
    isValidInchiKey,
} from "./inchi.cp";

// --- Biosignals, LiDAR Point Clouds & Control Systems (EDF, LAS, PID, G-Code) ---

export {
    EDF_MAIN_HEADER_SIZE,
    EDF_SIGNAL_HEADER_SIZE,
    EdfHeader,
    EdfSignal,
    isEdf,
    parseEdfHeader,
    parseEdfSignal,
    edfRecordByteSize,
    readEdfDigitalSample,
} from "./edf.cp";

export {
    LAS_MAGIC,
    LAS_HEADER_SIZE_1_2,
    LAS_HEADER_SIZE_1_3,
    LAS_HEADER_SIZE_1_4,
    LAS_VLR_HEADER_SIZE,
    LasHeader,
    LasVlr,
    LasPoint,
    isLas,
    parseLasHeader,
    LasVlrIter,
    LasPointIter,
} from "./las.cp";

export {
    PidConfig,
    PidTerms,
    PidController,
    zieglerNicholsPid,
    rateLimit,
} from "./pid.cp";

export {
    GcodeWord,
    GcodeLine,
    GcodeStats,
    gcodeChecksum,
    verifyGcodeChecksum,
    GcodeWordIter,
    parseGcodeLine,
    GcodeIter,
    summarizeGcode,
} from "./gcode.cp";

// --- 3D Scientific Mesh & Geometry Formats (STL, OBJ, PLY) ---

export {
    StlFormat,
    StlVec3,
    StlTriangle,
    StlBinaryHeader,
    StlBounds,
    detectStlFormat,
    parseStlBinaryHeader,
    StlBinaryIter,
    writeStlBinary,
    computeStlBinaryBounds,
    countAsciiStlFacets,
} from "./stl.cp";

export {
    ObjRecordKind,
    ObjVec3,
    ObjFaceVertex,
    ObjStats,
    ObjRecord,
    ObjLineIter,
    parseObjVec3,
    parseObjFaceVertex,
    summarizeObj,
} from "./obj.cp";

export {
    PlyFormat,
    PlyHeader,
    PlyElement,
    isPly,
    parsePlyHeader,
    PlyElementIter,
    plyScalarSize,
} from "./ply.cp";
