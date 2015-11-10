
#ifndef HTTP_REQUEST_HEADER
#define HTTP_REQUEST_HEADER
#include <curl/curl.h>

int http_doPost(char *url, char *fields,
                    struct curl_slist *headers, char *cookie, char *response);
int http_doGet(char *url, char *fields,
                    struct curl_slist *headers, char *cookie, char *response);
int http_getHeader(char *header);
#endif  //!HTTP_REQUEST_HEADER
