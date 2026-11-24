#include <curl/curl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t *data;
    size_t len;
    size_t cap;
} CpCurlBuf;

static size_t cp_curl_write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    CpCurlBuf *buf = (CpCurlBuf *)userp;
    if (realsize == 0) {
        return 0;
    }
    if (buf->len + realsize + 1 > buf->cap) {
        size_t new_cap = buf->cap == 0 ? 4096 : buf->cap * 2;
        while (new_cap < buf->len + realsize + 1) {
            new_cap *= 2;
        }
        uint8_t *new_data = (uint8_t *)realloc(buf->data, new_cap);
        if (!new_data) {
            return 0;
        }
        buf->data = new_data;
        buf->cap = new_cap;
    }
    memcpy(buf->data + buf->len, contents, realsize);
    buf->len += realsize;
    buf->data[buf->len] = 0;
    return realsize;
}

const char *cp_curl_version(void) {
    return curl_version();
}

int32_t cp_curl_global_init(void) {
    return (int32_t)curl_global_init(CURL_GLOBAL_DEFAULT);
}

void cp_curl_global_cleanup(void) {
    curl_global_cleanup();
}

uint8_t *cp_curl_http_get(const char *url, int64_t *out_status, size_t *out_len) {
    if (!url || !out_status || !out_len) {
        return NULL;
    }
    *out_status = 0;
    *out_len = 0;

    CURL *curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }

    CpCurlBuf buf = { NULL, 0, 0 };
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cp_curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        free(buf.data);
        curl_easy_cleanup(curl);
        return NULL;
    }

    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    curl_easy_cleanup(curl);

    if (!buf.data) {
        buf.data = (uint8_t *)calloc(1, 1);
        if (!buf.data) {
            return NULL;
        }
    }

    *out_status = (int64_t)status_code;
    *out_len = buf.len;
    return buf.data;
}

void cp_curl_free(void *ptr) {
    free(ptr);
}
