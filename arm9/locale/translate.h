#pragma once
#include <stdint.h>
#include <uchar.h>   // for char16_t

#ifdef __cplusplus
extern "C" {
#endif

// Set the current language by language code (e.g. "en", "ja", "fr")
int SetLanguage(const char* lang);

// Translation lookup function
const char16_t* _(const char16_t* msgid);

#ifdef __cplusplus
}
#endif
