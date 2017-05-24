/*
 * main.c
 *
 *  Created on: Dec 20, 2016
 *      Author: zjm09
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "src/json.h"

static int flag_pint = 0;
static int flag_set = 0;
static int flag_String = 0;
static int flag_del = 0;
static char config_file[2048] = {0};

static struct json *J;

void usage(char *p)
{
	fprintf(stderr,"%s -[psSf] [file] [config path] [value]\n",basename(p));
	fprintf(stderr,"\t -p print all params\n");
	fprintf(stderr,"\t -s set params\n");
	fprintf(stderr,"\t -S set params as string\n");
	fprintf(stderr,"\t -f set config file by [file]\n");
}

void pop_path(char *path)
{
	int len = strlen(path)-1;

	while(len--){
		if(path[len] == '[' || path[len-1] == '.'){
			path[len] = 0;
			return ;
		}
	}
}

void pop_path_array(char *path)
{
	int len = strlen(path);

	while(len--){
		if(path[len] == '['){
			path[len] = 0;
			return ;
		}
	}
}

int isNumber(const char *v)
{
	char *n = v;
	int point = 0;

	while (*n != 0) {
		if (!isdigit(*n)) {
			if (*n != '.') {
				return 0;
			}else if(++ point > 1)
				return 0;
		}
		n++;
	}

	return 1;
}

int main(int argc,char *argv[])
{
	int i = 0;
	int error;
	if(argc < 2){
		usage(argv[0]);
		exit(1);
	}

	if(argv[1][0] == '-'){
		for (i = 1; i < argc; ++i) {
			if(!strcmp(argv[i],"-p")){
				flag_pint = 1;
				break;
			}else if(!strcmp(argv[i],"-S")){
				flag_String = 1;
				break;
			}else if(!strcmp(argv[i],"-s")){
				flag_set = 1;
				break;
			}else if(!strcmp(argv[i],"-d")){
				flag_del = 1;
				break;
			}else if(!strcmp(argv[i],"-f")){
				i ++;
				if(i > argc){
					usage(argv[0]);
					exit(1);
				}
				strcpy(config_file,argv[i]);
	//			break;
			}
		}
	}

	i ++;

	if(strlen(config_file) == 0){
		strcpy(config_file,"/etc/system.json");
	}

	J = json_open(JSON_F_NONE, &error);
	json_loadpath(J,config_file);

	if(flag_pint){
		struct json_iterator I;
		struct json_value *V, *K;
		int index;
		char path[16384];
		struct jsonxs trap;
		int deep = 0;

		json_enter(J, &trap);

		json_push(J,".");

		memset(&I, 0, sizeof I);
		memset(path, 0, sizeof path);

		json_v_start(J, &I, json_top(J));

		while ((V = json_v_next(J, &I))) {
			if (json_i_order(J, &I) == JSON_I_PREORDER) {
				if (json_i_depth(J, &I) > 0) {
					if ((K = json_v_keyof(J, V))) {
//								printf("%s", json_v_string(J, K));
						strcat(path,json_v_string(J, K));
					} else if (-1 != (index = json_v_indexof(J, V))) {
						char _path[16];
						sprintf(_path,"[%d]", index);
						strcat(path,_path);
					}
				}

				switch (json_v_type(J, V)) {
					case JSON_T_ARRAY:
//								printf("[]");
//						strcat(path,".");
						deep ++;
						break;
					case JSON_T_OBJECT:
						strcat(path,".");
						break;
					case JSON_T_NULL:
						printf("%s=(null)",path);
						pop_path(path);
						break;
					case JSON_T_BOOLEAN:
						printf("%s=%s\n", path,json_v_boolean(J, V)?"true":"false");
						pop_path(path);
						break;
					case JSON_T_NUMBER:{
						double v = json_v_number(J, V);

						if(fmod(v,1) > 0.0)
							printf("%s=%f\n",path,v);
						else
							printf("%s=%d\n",path,(int)v);
						pop_path(path);
					}break;
					case JSON_T_STRING:
						printf("%s='%s'\n", path,json_v_string(J, V));
						pop_path(path);
						break;
				}
			}else {
				if (json_i_depth(J, &I) > 0 ){
					if(json_v_type(J, V) == JSON_T_ARRAY ||json_v_type(J, V) == JSON_T_OBJECT){
						pop_path(path);
					}
				}
			}
		}

		json_pop(J);
		exit(0);
	}

	if(flag_set || flag_String){
		FILE *fp = NULL;
		fp = fopen(config_file, "w");
		char *v = argv[i+1];

		if(flag_String){
			json_setstring(J, argv[i+1],argv[i]);
		}

		else if(isNumber(v))
			json_setnumber(J, atof(argv[i+1]),argv[i]);
		else
			json_setstring(J, argv[i+1],argv[i]);
		json_printfile(J, fp, JSON_F_PRETTY);
		fclose(fp);
		exit(0);
	}

	if(flag_del){
		FILE *fp = NULL;
		fp = fopen(config_file, "w");
		json_delete(J,argv[i]);
		json_printfile(J, fp, JSON_F_PRETTY);
		fclose(fp);
		exit(0);
	}

	enum json_type type = json_type(J,argv[i]);
	switch(type){
		case JSON_T_BOOLEAN:
//			json_boolean(J,argv[i]);
			printf("%s\n",json_boolean(J,argv[i])?"true":"false");
			break;
		case JSON_T_NUMBER:
//			json_number(J,argv[i]);
			printf("%d\n",(int)json_number(J,argv[i]));
			break;
		case JSON_T_STRING:
			printf("%s\n",json_string(J,argv[i]));
			break;
		case JSON_T_ARRAY:
		case JSON_T_OBJECT:{
			struct json_iterator I;
			struct json_value *V, *K;
			int index;
			char path[16384];
			struct jsonxs trap;
			int deep = 0;

			json_enter(J, &trap);

			json_push(J,argv[i]);

			memset(&I, 0, sizeof I);
			memset(path, 0, sizeof path);

			json_v_start(J, &I, json_top(J));

			while ((V = json_v_next(J, &I))) {
				if (json_i_order(J, &I) == JSON_I_PREORDER) {
					if (json_i_depth(J, &I) > 0) {
						if ((K = json_v_keyof(J, V))) {
	//								printf("%s", json_v_string(J, K));
							strcat(path,json_v_string(J, K));
						} else if (-1 != (index = json_v_indexof(J, V))) {
							char _path[16];
							sprintf(_path,"[%d]", index);
							strcat(path,_path);
						}
					}

					switch (json_v_type(J, V)) {
						case JSON_T_ARRAY:
	//								printf("[]");
	//						strcat(path,".");
							deep ++;
							break;
						case JSON_T_OBJECT:
							strcat(path,".");
							break;
						case JSON_T_NULL:
							printf("%s=(null)",path);
							pop_path(path);
							break;
						case JSON_T_BOOLEAN:
							printf("%s=%s\n", path,json_v_boolean(J, V)?"true":"false");
							pop_path(path);
							break;
						case JSON_T_NUMBER:{
								double v = json_v_number(J, V);

								if(fmod(v,1) > 0.0)
									printf("%s=%f\n",path,v);
								else
									printf("%s=%d\n",path,(int)v);
								pop_path(path);
							}break;
						case JSON_T_STRING:
							printf("%s='%s'\n", path,json_v_string(J, V));
							pop_path(path);
							break;
					}
				}else {
					if (json_i_depth(J, &I) > 0 ){
						if(json_v_type(J, V) == JSON_T_ARRAY ||json_v_type(J, V) == JSON_T_OBJECT){
							pop_path(path);
						}
					}
				}
			}

			json_pop(J);
			}break;
		default:
//			printf("error\n");
			break;
	}

	return 0;
}

