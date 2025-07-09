#include <cstring>
#include <curl/curl.h>
#include <cstdarg>
#include <string>

long simulated_http_code = 200;
CURLcode simulated_curl_result = CURLE_OK;
std::string simulated_response_body = R"({
  "parameters": [{
    "name": "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Airplay.Enable",
    "value": "true",
    "dataType": 3,
    "parameterCount": 1,
    "message": "Success"
  }],
  "statusCode": 0
})";

// Globals to store the user-provided write callback and userdata
static curl_write_callback g_write_callback = nullptr;
static void* g_write_data = nullptr;

extern "C" {

CURLcode curl_easy_setopt(CURL* curl, CURLoption option, ...) {
    va_list args;
    va_start(args, option);

    if (option == CURLOPT_WRITEFUNCTION) {
        g_write_callback = va_arg(args, curl_write_callback);
    } else if (option == CURLOPT_WRITEDATA) {
        g_write_data = va_arg(args, void*);
    } else {
        // Ignore other options for now
        (void)va_arg(args, void*);
    }

    va_end(args);
    return CURLE_OK;
}

CURLcode curl_easy_perform(CURL* curl) {
    if (g_write_callback && !simulated_response_body.empty()) {
        // Call the write callback exactly like libcurl would:
        // size=1, nmemb=length of data
        g_write_callback(simulated_response_body.c_str(), 1, simulated_response_body.size(), g_write_data);
    }
    return simulated_curl_result;
}

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
