#include <curl/curl.h>

extern "C" {

CURLcode curl_easy_perform(CURL* curl) {
    return CURLE_OK; // simulate successful request
}

CURLcode curl_easy_getinfo(CURL* curl, CURLINFO info, void* ptr) {
    if (info == CURLINFO_RESPONSE_CODE) {
        *(long*)ptr = 200;  // simulate HTTP 200
    }
    return CURLE_OK;
}

}
