all:vadtest.c  wb_vad.c http_request.c https_request.c base64.c cJSON.c
	gcc -o vad_baidu vadtest.c wb_vad.c http_request.c https_request.c base64.c cJSON.c -lcurl -lasound -lm
clean:
	rm -f vad_baidu
