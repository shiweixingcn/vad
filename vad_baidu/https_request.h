
#ifndef HTTPS_REQUEST_HEADER
#define HTTPS_REQUEST_HEADER
#include <curl/curl.h>

int https_doPost(char *url, char *fields,
                    struct curl_slist *headers, char *cookie, char *response, char* pClientCert, char* pRootCert);
int https_doGet(char *url, char *fields,
                    struct curl_slist *headers, char *cookie, char *response, char* pClientCert, char* pRootCert);
int https_getHeader(char *header);
#endif  //!HTTPS_REQUEST_HEADER
