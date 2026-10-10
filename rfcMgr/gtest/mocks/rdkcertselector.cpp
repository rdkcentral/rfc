#include "rdkcertselector.h"

namespace {
rdkcertselectorStatus_t getCertStatus = certselectorOk;
char* certificateUri = nullptr;
char* certificatePassword = nullptr;
char* engineName = nullptr;
rdkcertselector_h freeResult = nullptr;
}

extern "C" {

rdkcertselector_h rdkcertselector_new(const char*, const char*, const char*)
{
    return reinterpret_cast<rdkcertselector_h>(0x1);
}

rdkcertselectorStatus_t rdkcertselector_getCert(rdkcertselector_h, char** certUri, char** certPass)
{
    *certUri = certificateUri;
    *certPass = certificatePassword;
    return getCertStatus;
}

void rdkcertselector_free(rdkcertselector_h* selector)
{
    *selector = freeResult;
}

char* rdkcertselector_getEngine(rdkcertselector_h)
{
    return engineName;
}

int rdkcertselector_setCurlStatus(rdkcertselector_h, int, const char*)
{
    return 0;
}

void rdkcertselector_mock_reset(void)
{
    getCertStatus = certselectorOk;
    certificateUri = nullptr;
    certificatePassword = nullptr;
    engineName = nullptr;
    freeResult = nullptr;
}

void rdkcertselector_mock_set_get_cert(rdkcertselectorStatus_t status, char* certUri, char* certPass)
{
    getCertStatus = status;
    certificateUri = certUri;
    certificatePassword = certPass;
}

void rdkcertselector_mock_set_engine(char* engine)
{
    engineName = engine;
}

void rdkcertselector_mock_set_free_result(rdkcertselector_h selectorAfterFree)
{
    freeResult = selectorAfterFree;
}

}
