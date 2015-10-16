/* Saving bot state as json. Not for use in the real bot, only in the simulator. */
#include <kilombo.h>

#ifdef SIMULATOR

#include <jansson.h>
#include <stdio.h>
#include <string.h>
#include "gradient.h"

json_t *json_state()
{
  //create the state object we return
  json_t* state = json_object();

  // store the gradient value
  json_t* g = json_integer(mydata->gradient_value);
  json_object_set (state, "gradient", g);

  return state;
}

#endif
