#include <curl/curl.h>
#include <cstdarg>

extern "C" {

long simulated_http_code = 200;
CURLcode simulated_curl_result = CURLE_OK;

CURLcode curl_easy_perform(CURL* curl) {
    return simulated_curl_result;
}

// Correct variadic signature
CURLcode curl_easy_getinfo(CURL* curl, CURLINFO info, ...) {
    va_list args;
    va_start(args, info);

    if (info == CURLINFO_RESPONSE_CODE) {
        long* p = va_arg(args, long*);
        *p = simulated_http_code;
    }

    va_end(args);
    return CURLE_OK;
}

}
