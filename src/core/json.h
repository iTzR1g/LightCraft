#pragma once
#include <string>
#include <memory>
#include "../../vendor/cJSON.h"

namespace core {

struct JsonDeleter {
    void operator()(cJSON* p) { cJSON_Delete(p); }
};
using JsonPtr = std::unique_ptr<cJSON, JsonDeleter>;

JsonPtr json_parse(const std::string& text);
std::string json_stringify(const cJSON* obj, bool fmt = false);

std::string json_get_string(const cJSON* obj, const char* key, const char* def = "");
int         json_get_int(const cJSON* obj, const char* key, int def = 0);
double      json_get_number(const cJSON* obj, const char* key, double def = 0.0);
bool        json_get_bool(const cJSON* obj, const char* key, bool def = false);
cJSON*      json_get_object(const cJSON* obj, const char* key);
cJSON*      json_get_array(const cJSON* obj, const char* key);

}
