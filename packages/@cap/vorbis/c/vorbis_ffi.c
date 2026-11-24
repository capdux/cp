#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"
#include <stddef.h>

const char *cp_vorbis_version_string(void) {
    const char *v = vorbis_version_string();
    return v ? v : "unknown";
}

int cp_vorbis_encoder_test_init(int channels, long rate, float quality) {
    vorbis_info vi;
    vorbis_comment vc;
    vorbis_dsp_state vd;
    vorbis_block vb;

    vorbis_info_init(&vi);
    int rc = vorbis_encode_init_vbr(&vi, (long)channels, rate, quality);
    if (rc != 0) {
        vorbis_info_clear(&vi);
        return rc;
    }

    vorbis_comment_init(&vc);
    vorbis_comment_add_tag(&vc, "ENCODER", "cap-vorbis");

    rc = vorbis_analysis_init(&vd, &vi);
    if (rc != 0) {
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);
        return rc;
    }

    rc = vorbis_block_init(&vd, &vb);
    if (rc == 0) {
        vorbis_block_clear(&vb);
    }
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
    return rc;
}
