

#include <cstring>
#include <curl/curl.h>
#include <cstdarg>

long simulated_http_code = 200;
CURLcode simulated_curl_result = CURLE_OK;
//std::string simulated_response_body = R"({"parameters":[{"name":"Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Airplay.Enable","value":"true"}]})";

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

extern "C" {

CURLcode curl_easy_perform(CURL* curl) {
    // Simulate writing the response into the buffer via WRITEFUNCTION
    void* writeFunc = nullptr;
    void* writeData = nullptr;

    curl_easy_getinfo(curl, CURLINFO_PRIVATE, &writeData);  // Not used in real curl like this, just illustrative

    // Simulate calling the CURLOPT_WRITEFUNCTION if it was set
    // You'll need to intercept the setopt calls to properly store the callback and userdata

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
