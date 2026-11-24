#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

ogg_stream_state *cp_ogg_stream_init(int serialno) {
    ogg_stream_state *os = (ogg_stream_state *)calloc(1, sizeof(ogg_stream_state));
    if (!os) return NULL;
    if (ogg_stream_init(os, serialno) != 0) {
        free(os);
        return NULL;
    }
    return os;
}

int cp_ogg_stream_packetin(ogg_stream_state *os, const unsigned char *data, long len,
                           int bos, int eos, int64_t granulepos, int64_t packetno) {
    if (!os) return -1;
    ogg_packet op;
    op.packet = (unsigned char *)data;
    op.bytes = len;
    op.b_o_s = bos ? 1 : 0;
    op.e_o_s = eos ? 1 : 0;
    op.granulepos = granulepos;
    op.packetno = packetno;
    return ogg_stream_packetin(os, &op);
}

static int cp_copy_page(const ogg_page *og, unsigned char *out_buf, size_t out_cap, size_t *out_len) {
    size_t total = (size_t)og->header_len + (size_t)og->body_len;
    if (out_len) *out_len = total;
    if (!out_buf || out_cap < total) return -2;
    if (og->header_len > 0) memcpy(out_buf, og->header, (size_t)og->header_len);
    if (og->body_len > 0) memcpy(out_buf + og->header_len, og->body, (size_t)og->body_len);
    return 1;
}

int cp_ogg_stream_pageout(ogg_stream_state *os, unsigned char *out_buf, size_t out_cap, size_t *out_len) {
    if (!os) return -1;
    ogg_page og;
    int rc = ogg_stream_pageout(os, &og);
    if (rc == 0) return 0;
    return cp_copy_page(&og, out_buf, out_cap, out_len);
}

int cp_ogg_stream_flush(ogg_stream_state *os, unsigned char *out_buf, size_t out_cap, size_t *out_len) {
    if (!os) return -1;
    ogg_page og;
    int rc = ogg_stream_flush(os, &og);
    if (rc == 0) return 0;
    return cp_copy_page(&og, out_buf, out_cap, out_len);
}

void cp_ogg_stream_clear(ogg_stream_state *os) {
    if (!os) return;
    ogg_stream_clear(os);
    free(os);
}
