#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<jansson.h>

#ifndef _PARAMS_H
#define _PARAMS_H

typedef struct {
  json_t *root;
  int timeSteps;
} simulation_params;

simulation_params* parse_param_file(const char *filename);
int get_int_param(const char *param_name, int default_val);
float get_float_param(const char *param_name, float default_val);
const char* get_string_param(const char *param_name, char* default_val);
float get_float_array_param(const char * param_name, int index, float default_val);

extern simulation_params* simparams;

#endif
