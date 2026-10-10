#include "rfc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PLACEHOLDER_STRING
#define PLACEHOLDER_STRING "sample"
#endif

#ifndef PLACEHOLDER_MAC
#define PLACEHOLDER_MAC "00:11:22:33:44:55"
#endif

#ifndef PLACEHOLDER_INTERFACE
#define PLACEHOLDER_INTERFACE "wan0"
#endif

#if defined(RDKB_SUPPORT)
std::string getWanInterfaceName()
{
    return PLACEHOLDER_INTERFACE;
}

std::string getWanMacInterfaceName()
{
    return PLACEHOLDER_MAC;
}

std::string getBoxTypeFromFile()
{
    return PLACEHOLDER_STRING;
}

std::string getErouterMac()
{
    return PLACEHOLDER_MAC;
}

std::string geteCMMac()
{
    return PLACEHOLDER_MAC;
}
#endif

#ifdef __cplusplus
}
#endif
