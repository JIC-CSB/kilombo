#include"params.h"

simulation_params *simparams = NULL;

simulation_params* parse_param_file(const char *filename)
{
  simulation_params* parsed_params;

  json_error_t error;
  json_t *root, *data;

  parsed_params = (simulation_params*) malloc(sizeof(simulation_params));
  printf ("Reading simulator parameters from %s\n", filename);
  root = json_load_file(filename, 0, &error);

  if (!root) {
    fprintf(stderr, "Failed to parse %s\n", filename);
    return NULL;
  }

  if (json_is_object(root)) {
    data = root;
  } else {
    data = json_array_get(root, 0);
  }

  if (!json_is_object(data)) {
    fprintf(stderr, "error: not an object\n");
    return NULL;
  }

  parsed_params->root = root;

  return parsed_params;
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

  if (!json_is_string(param)) {
    fprintf(stderr, "Requested parameter: %s is not a string\n", param_name);
    return default_val;
  }

  const char* c_param = json_string_value(param);

  return c_param;
}
