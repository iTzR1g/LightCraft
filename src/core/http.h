#pragma once
#include <string>
#include <functional>

namespace core {

using ProgressCallback = std::function<void(size_t downloaded, size_t total)>;

std::string http_get(const std::string& url);
bool http_download(const std::string& url, const std::string& dest,
                   ProgressCallback progress = nullptr);
std::string http_last_error();

}
