#include "core/json.h"
#include <memory>

namespace core {

JsonPtr json_parse(const std::string& text) {
    return JsonPtr(cJSON_Parse(text.c_str()));
}

std::string json_stringify(const cJSON* obj, bool fmt) {
    char* s = fmt ? cJSON_Print(obj) : cJSON_PrintUnformatted(obj);
    std::string result(s ? s : "");
    cJSON_free(s);
    return result;
}

std::string json_get_string(const cJSON* obj, const char* key, const char* def) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(item) && item->valuestring) return item->valuestring;
    return def ? def : "";
}

int json_get_int(const cJSON* obj, const char* key, int def) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item)) return item->valueint;
    return def;
}

double json_get_number(const cJSON* obj, const char* key, double def) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item)) return item->valuedouble;
    return def;
}

bool json_get_bool(const cJSON* obj, const char* key, bool def) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsBool(item)) return cJSON_IsTrue(item);
    return def;
}

cJSON* json_get_object(const cJSON* obj, const char* key) {
    return cJSON_GetObjectItemCaseSensitive(obj, key);
}

cJSON* json_get_array(const cJSON* obj, const char* key) {
    return cJSON_GetObjectItemCaseSensitive(obj, key);
}

}
