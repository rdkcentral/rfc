#ifndef RDKCERTSELECTOR_H
#define RDKCERTSELECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* rdkcertselector_h;

typedef enum {
    certselectorOk = 0,
    certselectorError = -1
} rdkcertselectorStatus_t;

#ifndef TRY_ANOTHER
#define TRY_ANOTHER 1
#endif
#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG ""
#endif
#ifndef DEFAULT_HROT
#define DEFAULT_HROT ""
#endif

rdkcertselector_h rdkcertselector_new(const char* config, const char* hrot, const char* certGroup);
rdkcertselectorStatus_t rdkcertselector_getCert(rdkcertselector_h selector, char** certUri, char** certPass);
void rdkcertselector_free(rdkcertselector_h* selector);
char* rdkcertselector_getEngine(rdkcertselector_h selector);
int rdkcertselector_setCurlStatus(rdkcertselector_h selector, int curlStatus, const char* url);

void rdkcertselector_mock_reset(void);
void rdkcertselector_mock_set_get_cert(rdkcertselectorStatus_t status, char* certUri, char* certPass);
void rdkcertselector_mock_set_engine(char* engine);
void rdkcertselector_mock_set_free_result(rdkcertselector_h selectorAfterFree);

#ifdef __cplusplus
}
#endif

#endif
