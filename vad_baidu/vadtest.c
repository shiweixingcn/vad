#include <stdio.h>   
#include "wb_vad.h"
/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include <stdlib.h>

#include <string.h>
#include "cJSON.h"
#include "http_request.h"
#include "https_request.h"
#include "base64.h"
#define MAX_BUFFER_SIZE 512
#define BUFFER_MAX_LENGTH FRAME_LEN*1024

int getToken(char* cuid, char* apikey, char* secretkey, char* token)
{
    int ret = -1;

    char host[MAX_BUFFER_SIZE];
    char buffer[512];

    cJSON *root = NULL;
    cJSON *item = NULL;

    memset(host,0,sizeof(host));
    memset(buffer,0,sizeof(buffer));
    snprintf(host, sizeof(host), 
            "https://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id=%s&client_secret=%s", 
            apikey, secretkey);
    https_doGet(host, NULL, NULL, NULL, buffer,NULL,NULL);
   
    root = cJSON_Parse(buffer);
    if(root == NULL)
        goto failed;
    item = cJSON_GetObjectItem(root, "access_token");
    if(item != NULL){
        strcpy(token, item->valuestring);
        ret = 0;
    }

failed:
    return ret;
}

int getRecognitionResult(char* cuid, char* token, int rate, char* lan, char* audiodata, int content_len, char* response)
{
    char host[MAX_BUFFER_SIZE];
    memset(host, 0, sizeof(host));
    sprintf(host,  "%s", "http://vop.baidu.com/server_api");

    char* encode_data = base64_encode((const char *)audiodata, content_len);
    if (0 == strlen(encode_data))
    {
       printf("base64 encoded data is empty.\n");
       return -1;
    }

    cJSON *root = NULL;
    char  *body = NULL;
    int   body_len;
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "format", "pcm");
    cJSON_AddNumberToObject(root, "rate", rate);
    cJSON_AddStringToObject(root, "lan", lan);
    cJSON_AddNumberToObject(root, "channel", 1);
    cJSON_AddStringToObject(root, "token", token);
    cJSON_AddStringToObject(root, "cuid", cuid);
    cJSON_AddNumberToObject(root, "len", content_len);
    cJSON_AddStringToObject(root, "speech", encode_data);
    body = cJSON_Print(root);
    body_len = strlen(body);
    cJSON_Delete(root);
    root = NULL;

    char tmp[MAX_BUFFER_SIZE];
    memset(tmp, 0, sizeof(tmp));
    struct curl_slist *headerlist = NULL;
    sprintf(tmp,"%s","application/json; charset=utf-8");
    headerlist = curl_slist_append(headerlist, tmp);
    sprintf(tmp,"Content-Length: %d", body_len);
    headerlist = curl_slist_append(headerlist, tmp);

    http_doPost(host,body,headerlist,NULL,response);
    curl_slist_free_all(headerlist);
    return 0;
}


void main(int argc,char* argv[])
{
char* pcm_device_name="hw:1,0";
if(argc == 1)
{
    printf("use default pcm device: %s\n",pcm_device_name);
}
else if(argc == 2)
{
   pcm_device_name= argv[1];
}
else
{
    printf("usage: alsa_vad_yuyin pcm_device\n");
    return;
}

// for baidu yuyin, change it by your username and password
    char *cuid = "5692369";
    char *apikey = "7IAD225W24bOdZwxqzpO9MSR";
    char *secretkey = "vhC88sPg2VASSSYx1EPuNFd3GUuYDbA5";
    char token[64];
    char *lan = "zh";
    int ret;
    char response[1024];

// for alsa
int size;
int size_one_channel;
snd_pcm_t *handle;
snd_pcm_hw_params_t *params;
int dir;
char *buffer;
char *buffer_one_channel;
int i;

snd_pcm_stream_t  stream = SND_PCM_STREAM_CAPTURE;
snd_pcm_access_t  mode = SND_PCM_ACCESS_RW_INTERLEAVED;
snd_pcm_format_t  format = SND_PCM_FORMAT_U16_LE;
unsigned int      channels = 2;
unsigned int      rate     = 16000;
snd_pcm_uframes_t frames   = FRAME_LEN;
int bit_per_sample,bit_per_frame,chunk_bytes;

//for vda
float indata[FRAME_LEN];   
short outdata[FRAME_LEN];   
VadVars *vadstate;                     
int temp,vad;
int recording = -1;
char* audio_buffer;
char* tmp_buffer;
int content_len = 0;
tmp_buffer = audio_buffer =(char*)malloc(BUFFER_MAX_LENGTH);
// alsa init
int rc = snd_pcm_open(&handle, pcm_device_name,stream, 0);
if (rc < 0) {
    fprintf(stderr,"unable to open pcm device: %s\n",snd_strerror(rc));
    exit(1);
}
snd_pcm_hw_params_alloca(&params);
snd_pcm_hw_params_any(handle, params);
snd_pcm_hw_params_set_access(handle, params,mode);
snd_pcm_hw_params_set_format(handle, params,format);
snd_pcm_hw_params_set_channels(handle, params, channels);
snd_pcm_hw_params_set_rate_near(handle, params,&rate, 0);
snd_pcm_hw_params_set_period_size_near(handle,params, &frames, 0);
rc = snd_pcm_hw_params(handle, params);
if (rc < 0) {
    fprintf(stderr,"unable to set hw parameters: %s\n",snd_strerror(rc));
    exit(1);
}
snd_pcm_hw_params_get_period_size(params,&frames, 0);
size = frames * 4; /* 2 bytes/sample, 2 channels */
size_one_channel = frames * 2;
buffer = (char*) malloc(size);

//vad init
wb_vad_init(&(vadstate));

while (1) {
    rc = snd_pcm_readi(handle, buffer, frames);
    if (rc == -EPIPE) {
      /* EPIPE means overrun */
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr, "error from read: %s\n",snd_strerror(rc));
    } else if (rc != (int)frames) {
      fprintf(stderr, "short read, read %d frames\n", rc);
    }
    for(i = 0; i< frames; i++)
    {
        indata[i]=0;
        temp = 0;
        memcpy(&temp,buffer+4*i,2);
        indata[i]=(float)temp;
        outdata[i]=temp;
        if(indata[i]>65535/2)
            indata[i]=indata[i]-65536;
    }
    if(recording == -1)
    {
        tmp_buffer= audio_buffer;
        content_len = 0;
        memset(audio_buffer,0,BUFFER_MAX_LENGTH);
        memset(response,0,1024);
    }
 
    vad=wb_vad(vadstate,indata);
    if(vad == 1){
       	recording = 1;
    }
    else if(vad == 0 && recording == 1){
    ret = getToken(cuid,apikey,secretkey,token);
	ret = getRecognitionResult(cuid, token, rate, lan, audio_buffer, content_len, response);
    if(strstr(response,"success") != NULL)
	    printf("-------%s\n",response);
    recording = -1;
    }
    
    if(recording == 1)
    {
        memcpy(tmp_buffer,outdata,2*FRAME_LEN);
        tmp_buffer =tmp_buffer+2*FRAME_LEN;
        content_len =content_len+2*FRAME_LEN;
    }
    
}
free(buffer);
free(audio_buffer);
}
