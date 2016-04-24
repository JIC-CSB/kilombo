#include <ctype.h>

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <time.h>

#include "skilobot.h"
#include "params.h"
#include "kilolib.h"
#include <jansson.h>

json_t* (*callback_json_state) (void);

void json_store_double(json_t* root, const char *key, double value)
{
  json_t* jvalue = json_real(value);
  json_object_set_new(root, key, jvalue);
}

void json_store_int(json_t* root, const char *key, int value)
{
  json_t* jvalue = json_integer(value);
json_object_set_new(root, key, jvalue);
}

json_t* json_bot_rep(kilobot *bot)
{
  //printf("%d: %f, %f, %f\n", bot->ID, bot->x, bot->y, bot->direction);

  json_t* root = json_object();

  json_store_int(root, "ID", bot->ID);
  json_store_double(root, "direction", bot->direction);
  json_store_double(root, "x_position", bot->x);
  json_store_double(root, "y_position", bot->y);

  /*
  // store history in bot state
  // not in use currently, since full bot states can be stored periodically.
  json_t* j_x_hist = json_array();
  json_t* j_y_hist = json_array();
  json_t *j_xhval, *j_yhval;
  for (int i=0; i<bot->p_hist; i++) {
    j_xhval = json_real(bot->x_history[i]);
    json_array_append(j_x_hist, j_xhval);
    j_yhval = json_real(bot->y_history[i]);
    json_array_append(j_y_hist, j_yhval);
  }

  json_object_set(root, "x_history", j_x_hist);
  json_object_set(root, "y_history", j_y_hist);
  */
  
  // bot state
  json_t * j_state;
  if(callback_json_state)
    {
      // switch to the current bot
      prepare_bot(bot);
      j_state = callback_json_state();
    }  
  else // if the bot did not define a callback function
    j_state = json_object();   // ... output an empty object

  json_object_set_new(root, "state", j_state);
  
  return root;
}

json_t* json_rep_all_bots(kilobot **bot_array, int array_size, int ticks)
{
  json_t* root = json_object();
  json_t* j_bot_array = json_array();
  
  
  json_object_set_new(root, "bot_states", j_bot_array);
  json_store_int(root, "ticks", ticks);
   
  json_t *jbot;
  for (int i=0; i<array_size; i++) {
    jbot = json_bot_rep(bot_array[i]);
    json_array_append_new(j_bot_array, jbot);
  }

  return root;

  char *jstring = json_dumps(root, JSON_INDENT(4) | JSON_SORT_KEYS);

  printf("%s\n", jstring);

}

void save_bot_state_to_file(kilobot **bot_array, int array_size, const char *filename)
{
  json_t* root = json_rep_all_bots(bot_array, array_size, kilo_ticks);

  json_dump_file(root, filename, JSON_INDENT(2) | JSON_SORT_KEYS);

  json_decref(root);
}



double extract_double(json_t* js, const char *param_name)
{
  json_t *param = json_object_get(js, param_name);

  if (!json_is_number(param)) {
    fprintf(stderr, "error: Failed to read value: %s\n", param_name);
    return 0;
  }

  return (double) json_real_value(param);
}

int extract_int(json_t* js, const char *param_name)
{
  json_t *param = json_object_get(js, param_name);

  json_int_t i_param = json_integer_value(param);

  return (int) i_param;
}

kilobot* bot_from_json(json_t* bot_rep, int n_bots)
{
  int ID = extract_int(bot_rep, "ID");

  kilobot *bot = new_kilobot(ID, n_bots);

  bot->x = extract_double(bot_rep, "x_position");
  bot->y = extract_double(bot_rep, "y_position");
  bot->direction = extract_double(bot_rep, "direction");

  return bot;
}

kilobot** bot_loader(const char *filename, int *n_bots)
{
  json_error_t error;
  json_t* root;

  root = json_load_file(filename, 0, &error);

  if (!root) {
    fprintf(stderr, "Failed to parse %s.\nLine %d: %s\n", filename, error.line,error.text);
    return NULL;
  }

  if (!json_is_object(root)) {
    fprintf(stderr, "error: not an object\n");
    return NULL;
  }

  json_t* j_state_array = json_object_get(root, "bot_states");

  if (!json_is_array(j_state_array)) {
    fprintf(stderr, "error: bot_states is not an array\n");
    return NULL;
  }

  *n_bots = json_array_size(j_state_array);
  json_t* bot_state;
  kilobot **bots = (kilobot **) malloc(sizeof(kilobot *) * *n_bots);

  for (int i=0; i<*n_bots; i++) {
    bot_state = json_array_get(j_state_array, i);
    bots[i] = bot_from_json(bot_state, *n_bots);
  }

  return bots;
}

/* int main(int argc, char *argv[]) */
/* { */
/*   int n_bots = 1; */
/*   int h = 200; */
/*   int w = 200; */

/*   //init_all_bots(n_bots); */

/*   /\* for (int i=0; i < n_bots; i++) { *\/ */
/*   /\*   allbots[i]->x = rand()%w - w/2; *\/ */
/*   /\*   allbots[i]->y = rand()%h - h/2; *\/ */
/*   /\*   allbots[i]->direction = 2 * 3.141 * (float) rand() / (float) RAND_MAX; *\/ */
/*   /\* } *\/ */

/*   allbots = bot_loader("astates.json", &n_bots); */

/*   save_bot_state_to_file(allbots, n_bots, "teststates.json"); */

/*   return 0; */
/* } */







































