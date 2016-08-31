#include"params.h"

simulation_params *simparams = NULL;

void parse_param_file(const char *filename)
{
  json_error_t error;
  json_t *root, *data;

  simparams = (simulation_params*) malloc(sizeof(simulation_params));
  printf ("Reading simulator parameters from %s\n", filename);
  root = json_load_file(filename, 0, &error);

  if (!root) {
    fprintf(stderr, "Failed to parse %s.\nLine %d: %s\n", filename, error.line,error.text);
    exit(1);
    //    return;
  }

  if (json_is_object(root)) {
    data = root;
  } else {
    data = json_array_get(root, 0);
  }

  if (!json_is_object(data)) {
    fprintf(stderr, "error: not an object\n");
    return;
  }

  simparams->root = root;


  // extract parameter values, place in simparams struct.

  simparams->showComms            = get_int_param   ("showComms",      1);
  simparams->maxTime              = get_float_param ("simulationTime", 0);
  simparams->timeStep             = get_float_param ("timeStep",       0.02);
  simparams->imageName            = get_string_param("imageName",      NULL);
  simparams->finalImage           = get_string_param("finalImage",     NULL);
  simparams->storeHistory         = get_int_param   ("storeHistory",   0);
  simparams->saveVideoN           = get_int_param   ("saveVideoN",     1); // save video screenshot every Nth frame
  simparams->saveVideo            = get_int_param   ("saveVideo",      0); // whether to save video.
                                                                      // Toggle with 'v' at runtime
  simparams->stateFileName        = get_string_param("stateFileName",  NULL);
  simparams->stateFileSteps       = get_int_param   ("stateFileSteps", 100);
  simparams->stepsPerFrame        = get_int_param   ("stepsPerFrame",  1);
  simparams->bot_name             = get_string_param("botName",        "default");
  simparams->display_w            = get_int_param   ("displayWidth",  -1);
  simparams->display_h            = get_int_param   ("displayHeight", -1);
  simparams->display_scale        = get_float_param ("displayScale",   1.0);
  simparams->showCommsRadius      = get_int_param   ("showCommsRadius", 1);
  simparams->commsRadius          = get_int_param   ("commsRadius", 70);
  simparams->displayWidthPercent  = get_float_param("displayWidthPercent",  0.9);
  simparams->displayHeightPercent = get_float_param("displayHeightPercent", 0.9);
  simparams->histLength           = get_int_param("histLength", 500); // number of history points to draw
  simparams->showHist             = get_int_param("showHist", 0);
  simparams->randSeed             = get_int_param("randSeed", 0);
  simparams->GUI                  = get_int_param("GUI", 1);
  simparams->distance_noise       = get_float_param("distanceNoise", 0);
  simparams->msg_success_rate     = get_float_param("msgSuccessRate", 1);
  simparams->speed                = get_float_param("speed", 7);
  simparams->turn_rate            = get_float_param("turnRate", 13);
  simparams->pushDisplacement     = get_float_param("pushDisplacement", 1.0); 
  simparams->distanceCoefficient  = get_float_param("distanceCoefficient", 1.0);
  simparams->displayX             = get_float_param("displayX", 0);
  simparams->displayY             = get_float_param("displayY", 0);
  simparams->useGrid              = get_int_param("useGrid", 1);
}

int get_int_param(const char *param_name, int default_val)
{
  if (!simparams) {
    fprintf(stderr, "Error: attempted to read parameter without loading parameter file\n");
    exit(2);
  }

  json_t *param = json_object_get(simparams->root, param_name);

  if (!json_is_integer(param)) {
    fprintf(stderr, "Requested parameter: %s is not an integer.\n Using default value %d.\n", param_name, default_val);
    return default_val;
  }

  json_int_t i_param = json_integer_value(param);

  return (int) i_param;
}

float get_float_param(const char *param_name, float default_val)
{
  if (!simparams) {
    fprintf(stderr, "Error: attempted to read parameter without loading parameter file\n");
    exit(2);
  }

  json_t *param = json_object_get(simparams->root, param_name);

  if (!json_is_number(param)) {
    fprintf(stderr, "Requested parameter: %s is not a number.\n Using default value %f.\n", param_name, default_val);
    return default_val;
  }

  float f_param = json_number_value(param);

  return f_param;
}

size_t get_array_param_size(const char * param_name){
  if (!simparams) {
    fprintf(stderr, "Error: attempted to read parameter without loading parameter file\n");
    exit(2);
  }

  json_t *param = json_object_get(simparams->root, param_name);

  if (!json_is_array(param)) {
    fprintf(stderr, "Requested parameter: %s is not an array.\n", param_name);
    return ~0u;
  }

  return json_array_size(param);
}

int get_int_array_param(const char * param_name, int index, int default_val)
{
  if (!simparams) {
    fprintf(stderr, "Error: attempted to read parameter without loading parameter file\n");
    exit(2);
  }

  json_t *param = json_object_get(simparams->root, param_name);

  if (!json_is_array(param)) {
    fprintf(stderr, "Requested parameter: %s is not an array.\n Using default value %d.\n", param_name, default_val);
    return default_val;
  }

  if (index >= json_array_size(param)) {
	  fprintf(stderr, "Index %d out of bound in array parameter %s.\n Using default value %d.\n",
		  index, param_name, default_val);
	  return default_val;
  }

  json_t *elem = json_array_get(param, index);

  if (!json_is_number(elem)) {
    fprintf(stderr, "Requested element %d of parameter: %s is not a number.\n Using default value %d.\n", index, param_name, default_val);
    return default_val;
  }

  int f_param = json_integer_value(elem);

  return f_param;
}

float get_float_array_param(const char * param_name, int index, float default_val)
{
  if (!simparams) {
    fprintf(stderr, "Error: attempted to read parameter without loading parameter file\n");
    exit(2);
  }

  json_t *param = json_object_get(simparams->root, param_name);

  if (!json_is_array(param)) {
    fprintf(stderr, "Requested parameter: %s is not an array.\n Using default value %f.\n", param_name, default_val);
    return default_val;
  }

  if (index >= json_array_size(param)) {
	  fprintf(stderr, "Index %d out of bound in array parameter %s.\n Using default value %f.\n",
		  index, param_name, default_val);
	  return default_val;
  }

  json_t *elem = json_array_get(param, index);

  if (!json_is_number(elem)) {
    fprintf(stderr, "Requested element %d of parameter: %s is not a number.\n Using default value %f.\n", index, param_name, default_val);
    return default_val;
  }

  float f_param = json_number_value(elem);

  return f_param;
}

const char* get_string_param(const char *param_name, char *default_val)
{
  if (!simparams) {
    fprintf(stderr, "Error: attempted to read parameter without loading parameter file\n");
    exit(2);
  }

  json_t *param = json_object_get(simparams->root, param_name);

  if (!json_is_string(param) && !json_is_null(param)) {
    fprintf(stderr, "Requested parameter: %s is not a string\n", param_name);
    return default_val;
  }

  const char* c_param = json_string_value(param);

  return c_param;
}
