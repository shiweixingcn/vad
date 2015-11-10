
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "https_request.h"
//#include "log.h"

#define MAX_BYTES_RECV    (10240)
static char header_buffer[CURL_ERROR_SIZE]; 

static size_t https_write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    if(strlen((char *)stream) + strlen((char *)ptr) > MAX_BYTES_RECV)
        return 0;
    strcat(stream, (char *)ptr);
    return size*nmemb;
}

static size_t https_get_header(void *ptr, size_t size, size_t nmemb, void *stream)
{
    if(strstr((char*)(ptr), "HTTP") != NULL)
    	sprintf(header_buffer,"%s",(char*)(ptr));
    return size*nmemb;
}

static int https_request(int dopost, char *url, char *fields,
            struct curl_slist *headers, char *cookie, char *response, char* pClientCert, char* pRootCert)
{
    CURL *curl;
    CURLcode res;
    int ret = -1;
    char curl_errbuf[CURL_ERROR_SIZE];

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(!curl)
        goto failed;

    if(dopost)
        curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if(headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    if(fields) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
        curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE, strlen(fields));
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, https_write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    // set https connection timeout
    curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, 60);
    // set https receive timeout
    curl_easy_setopt( curl, CURLOPT_TIMEOUT, 60); 
    // for https only
    curl_easy_setopt(curl,CURLOPT_SSLCERT,pClientCert);
    curl_easy_setopt(curl,CURLOPT_CAINFO,pRootCert);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);
    curl_easy_setopt(curl,CURLOPT_VERBOSE,0L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, *https_get_header);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);

    if(cookie) {
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookie);
    }
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    memset(header_buffer,0,sizeof(header_buffer));
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if(res == CURLE_OK)
        ret = 0;
    else
	sprintf(header_buffer,"%s",curl_errbuf);
failed:
    curl_global_cleanup();
    return ret;
}

int https_doPost(char *url, char *fields,
            struct curl_slist *headers, char *cookie, char *response, char *pClientCert, char *pRootCert)
{
    //log_debug2("url:%s\ndata:%s\n", (url)?url:"null", (fields)?fields:"null");
    return https_request(1, url, fields, headers, cookie, response, pClientCert, pRootCert);
}

int https_doGet(char *url, char *fields,
            struct curl_slist *headers, char *cookie, char *response, char *pClientCert, char *pRootCert)
{
    return https_request(0, url, fields, headers, cookie, response, pClientCert, pRootCert);
}

int https_getHeader(char *header)
{
	return sprintf(header,"%s",header_buffer);
}
