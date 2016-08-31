#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<jansson.h>

#ifndef _PARAMS_H
#define _PARAMS_H

typedef struct {
  json_t *root;

  int timeSteps;
  int showComms;
  float maxTime; 
  float timeStep;
  const char *imageName;
  const char *finalImage;
  int storeHistory;
  int saveVideoN;
  int saveVideo;
  const char *stateFileName; 
  int stateFileSteps; 
  int stepsPerFrame; 
  const char *bot_name;
  float display_scale;  
  int display_w;
  int display_h;
  int showCommsRadius;
  float displayWidthPercent;
  float displayHeightPercent;
  int histLength;
  int showHist;
  int randSeed;
  int commsRadius;
  double turn_rate;  // degrees / s
  double speed;      // mm / s
  double pushDisplacement; // [0,1]
  int GUI;
  float msg_success_rate;
  float distance_noise;
  double distanceCoefficient; // slope of measured distance
  double displayX, displayY;
  int useGrid; // if true, use the grid cache
} simulation_params;

void parse_param_file(const char *filename);
int get_int_param(const char *param_name, int default_val);
float get_float_param(const char *param_name, float default_val);
const char* get_string_param(const char *param_name, char* default_val);
size_t get_array_param_size(const char * param_name);
int get_int_array_param(const char * param_name, int index, int default_val);
float get_float_array_param(const char * param_name, int index, float default_val);

extern simulation_params* simparams;

#endif
