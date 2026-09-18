#include "core/http.h"
#include <curl/curl.h>
#include <fstream>
#include <cstdio>

namespace core {

static std::string last_err;

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

struct CurlInit {
    CurlInit() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlInit() { curl_global_cleanup(); }
};

static CURL* make_handle() {
    static CurlInit init;
    CURL* h = curl_easy_init();
    curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(h, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(h, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(h, CURLOPT_USERAGENT, "Lightcraft/0.1");
    curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 1L);
    return h;
}

static std::string curl_error(CURLcode code) {
    return std::string(curl_easy_strerror(code));
}

std::string http_get(const std::string& url) {
    std::string result;
    CURL* h = make_handle();
    curl_easy_setopt(h, CURLOPT_URL, url.c_str());
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, write_string);
    curl_easy_setopt(h, CURLOPT_WRITEDATA, &result);
    CURLcode res = curl_easy_perform(h);

    if (res != CURLE_OK) {
        last_err = curl_error(res) + " (" + url + ")";
        curl_easy_cleanup(h);
        // Retry without SSL verification
        CURL* h2 = make_handle();
        curl_easy_setopt(h2, CURLOPT_URL, url.c_str());
        curl_easy_setopt(h2, CURLOPT_WRITEFUNCTION, write_string);
        curl_easy_setopt(h2, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(h2, CURLOPT_SSL_VERIFYPEER, 0L);
        res = curl_easy_perform(h2);
        if (res != CURLE_OK) {
            last_err = curl_error(res) + " (" + url + ")";
        }
        curl_easy_cleanup(h2);
    } else {
        last_err.clear();
    }
    return (res == CURLE_OK) ? result : "";
}

bool http_download(const std::string& url, const std::string& dest,
                   ProgressCallback progress) {
    CURL* h = make_handle();
    std::ofstream file(dest, std::ios::binary);
    if (!file) {
        last_err = "Cannot create file: " + dest;
        curl_easy_cleanup(h);
        return false;
    }

    curl_easy_setopt(h, CURLOPT_URL, url.c_str());
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(h, CURLOPT_WRITEDATA, &file);

    CURLcode res = curl_easy_perform(h);
    file.close();

    if (res != CURLE_OK) {
        last_err = curl_error(res) + " (" + url + ")";
        curl_easy_cleanup(h);
        // Retry without SSL
        std::ofstream file2(dest, std::ios::binary);
        if (!file2) return false;
        CURL* h2 = make_handle();
        curl_easy_setopt(h2, CURLOPT_URL, url.c_str());
        curl_easy_setopt(h2, CURLOPT_WRITEFUNCTION, write_file_cb);
        curl_easy_setopt(h2, CURLOPT_WRITEDATA, &file2);
        curl_easy_setopt(h2, CURLOPT_SSL_VERIFYPEER, 0L);
        res = curl_easy_perform(h2);
        file2.close();
        if (res != CURLE_OK) {
            last_err = curl_error(res) + " (" + url + ")";
            curl_easy_cleanup(h2);
            return false;
        }
        curl_easy_cleanup(h2);
        last_err.clear();
        return true;
    }

    last_err.clear();
    curl_easy_cleanup(h);
    return true;
}

std::string http_last_error() {
    return last_err;
}

}
