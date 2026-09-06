#include "core/http.h"
#include <curl/curl.h>
#include <fstream>

namespace core {

static size_t write_string(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* s = static_cast<std::string*>(userdata);
    s->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

static size_t write_file_cb(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* f = static_cast<std::ofstream*>(userdata);
    f->write(static_cast<char*>(ptr), size * nmemb);
    return f->good() ? size * nmemb : 0;
}

static int progress_cb(void* userdata, curl_off_t dltotal, curl_off_t dlnow,
                       curl_off_t, curl_off_t) {
    auto* cb = static_cast<ProgressCallback*>(userdata);
    if (cb && *cb) (*cb)(dlnow, dltotal);
    return 0;
}

struct CurlInit {
    CurlInit() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlInit() { curl_global_cleanup(); }
};

static CURL* make_handle() {
    static CurlInit init;
    CURL* h = curl_easy_init();
    curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(h, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(h, CURLOPT_TIMEOUT, 300L);
    curl_easy_setopt(h, CURLOPT_USERAGENT, "MineLaunch/0.1");
    return h;
}

std::string http_get(const std::string& url) {
    std::string result;
    CURL* h = make_handle();
    curl_easy_setopt(h, CURLOPT_URL, url.c_str());
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, write_string);
    curl_easy_setopt(h, CURLOPT_WRITEDATA, &result);
    CURLcode res = curl_easy_perform(h);
    curl_easy_cleanup(h);
    return (res == CURLE_OK) ? result : "";
}

bool http_download(const std::string& url, const std::string& dest,
                   ProgressCallback progress) {
    CURL* h = make_handle();
    std::ofstream file(dest, std::ios::binary);
    if (!file) { curl_easy_cleanup(h); return false; }

    curl_easy_setopt(h, CURLOPT_URL, url.c_str());
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(h, CURLOPT_WRITEDATA, &file);
    if (progress) {
        curl_easy_setopt(h, CURLOPT_XFERINFOFUNCTION, progress_cb);
        curl_easy_setopt(h, CURLOPT_XFERINFODATA, &progress);
    }

    CURLcode res = curl_easy_perform(h);
    file.close();
    curl_easy_cleanup(h);
    return (res == CURLE_OK);
}

}
