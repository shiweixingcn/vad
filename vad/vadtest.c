#include "stdio.h"   
#include "wb_vad.h"
/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include <stdlib.h>

void main(int argc,char* argv[])   
{      
if(argc != 4){
   printf("usage: alsa_vad pcm_device output record_period_seconds\nfor example: ./alsa_vad hw:1,0 out 10\n");
   return;
}

char* pcm_device_name = argv[1];
char* output_file_name = argv[2];
int   record_seconds = atoi(argv[3]);
// for alsa
long loops;
int rc;
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
char name[128]; 
int temp,vad;
int recording = 0;
int count = 0;
FILE *fp = NULL;
FILE *fp_all = NULL;
// alsa init
rc = snd_pcm_open(&handle, pcm_device_name,stream, 0);
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
buffer_one_channel = (char*) malloc(size_one_channel);

//vad init
wb_vad_init(&(vadstate));
sprintf(name,"%s.pcm",output_file_name);
fp_all = fopen(name,"wb");

/* We want to loop for 5 seconds */
snd_pcm_hw_params_get_period_time(params,&rate, &dir);
loops = record_seconds * 1000000 / rate;

while (loops > 0) {
    loops--;
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
    vad=wb_vad(vadstate,indata);
    if(vad == 1)
       	recording = 1;
    else
        recording = 0;
    if(recording ==1 && fp == NULL)
    {
        sprintf(name,"%s.%d.pcm",output_file_name,count);
        fp=fopen(name,"wb");   
    }
    if(recording == 0 && fp != NULL)
    {   
        fclose(fp);
        fp = NULL;
        count++;
    }
    if(fp != NULL && recording == 1)
        fwrite(outdata,2,FRAME_LEN,fp);
    if(fp_all != NULL)
        fwrite(outdata,2,FRAME_LEN,fp_all);
    }
    fcloseall();
}
