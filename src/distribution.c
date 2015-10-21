/* Various initial distributions for the robots.
 */

#include<math.h>
#include<jansson.h>
#include"kilolib.h"
#include"skilobot.h"
#include"params.h"
#include"stateio.h"



void distribute_bots(int n_bots);

// prototypes - functions only only used in this file
void distribute_line(int n_bots);
void distribute_rand(int n_bots, int w, int h);
void distribute_circle(int n_bots);
void distribute_pile(int n_bots);
void distribute_elipse(int n_bots);
void distribute_pile2(int n_bots);


void distribute_line(int n_bots)
{
  for (int i=0; i < n_bots; i++) {
    allbots[i]->x += 60 * i - 40 * (n_bots - 1);
  }
}

void distribute_rline(int n_bots)
{
  for (int i=0; i < n_bots; i++) {
    allbots[i]->x += 50 * (n_bots - i) - 40 * (n_bots - 1);
  }
}

void distribute_rand(int n_bots, int w, int h)
{
  for (int i=0; i < n_bots; i++) {
    allbots[i]->x = rand()%w - w/2;
    allbots[i]->y = rand()%h - h/2;
    allbots[i]->direction = 2 * M_PI * (float) rand() / (float) RAND_MAX;
  }
}

void distribute_circle(int n_bots)
{
	int bot = 0;
	float bot_radius = 1.8 * allbots[0]->radius;
	float radius = n_bots * bot_radius/M_PI;
	float theta = 0;
	float delta_theta = 2 * M_PI/n_bots;
	float alpha = theta - M_PI/4;
	float x_value = radius * sin(theta);
	float y_value = radius * cos(theta);

	while(bot < n_bots){

		allbots[bot]->x = x_value;
		allbots[bot]->y = y_value;
		allbots[bot]->direction = alpha;

		theta += delta_theta;
		alpha = theta - M_PI/4;
		x_value = radius * sin(theta);
		y_value = radius * cos(theta);

		bot++;
	}
}

void distribute_pile(int n_bots)
{
	float bot_radius = 1.3 * allbots[0]->radius;
	float radius = 0;
	int max_n_bots = 1;
	int bot = 0;
	float x_value = 0;
	float y_value = 0;
	float theta = 0;
	float delta_theta = 0;
	float alpha = theta - M_PI/4;

    while(bot < n_bots){

    	allbots[bot]->x = x_value;
    	allbots[bot]->y = y_value;
    	allbots[bot]->direction = alpha;

    	max_n_bots--;
    	if(max_n_bots > 0){

    		theta += delta_theta;
    		alpha = theta - M_PI/4;
    	}

    	else{
    		radius += 2 * bot_radius + bot_radius/3;

    		max_n_bots = (int) (M_PI * radius/bot_radius);

    		delta_theta = 2 * M_PI/max_n_bots;
    		theta = 0;
    		alpha = theta - M_PI/4;
    	}

    	x_value = radius * sin(theta);
		y_value = radius * cos(theta);

    	bot++;
    }
}

/* A circular pile of bots, like distribute_pile, but with random directions
 */
void distribute_random_pile(int n_bots)
{
	float bot_radius = 1.3 * allbots[0]->radius;
	float radius = 0;
	int max_n_bots = 1;
	int bot = 0;
	float x_value = 0;
	float y_value = 0;
	float theta = 0;
	float delta_theta = 0;
	//	float alpha = theta - M_PI/4;

    while(bot < n_bots){

    	allbots[bot]->x = x_value;
    	allbots[bot]->y = y_value;
    	allbots[bot]->direction =  ((float)rand()/(float)(RAND_MAX)) * (2*M_PI);

    	max_n_bots--;
    	if(max_n_bots > 0){
	  	theta += delta_theta;
    	}
    	else{
    		radius += 2 * bot_radius + bot_radius/3;

    		max_n_bots = (int) (M_PI * radius/bot_radius);

    		delta_theta = 2 * M_PI/max_n_bots;
    		theta = 0;
    	}

    	x_value = radius * sin(theta);
	y_value = radius * cos(theta);

    	bot++;
    }
}

void distribute_elipse(int n_bots)
{
	float bot_radius = 1.5 * allbots[0]->radius;
	float radius = 0;
	int max_n_bots = 1;
	int bot = 0;
	float x_value = 0;
	float y_value = 0;
	float theta = 0;
	float delta_theta = 0;
	float alpha = theta - M_PI/4;

    while(bot < n_bots){

    	allbots[bot]->x = x_value;
    	allbots[bot]->y = y_value;
    	allbots[bot]->direction = alpha;

    	max_n_bots--;
    	if(max_n_bots > 0){

    		theta += delta_theta;
    		alpha = theta - M_PI/4;

    	}

    	else{

    		radius += 2 * bot_radius + bot_radius/3;

    		max_n_bots = (int) (M_PI * radius/bot_radius);

    		delta_theta = 2 * M_PI/max_n_bots;
    		theta = 0;
    		alpha = theta - M_PI/4;


    	}

    	x_value = radius * sin(theta);
	y_value = 0.75 * radius * cos(theta);


    	bot++;



    }


}

void distribute_pile2(int n_bots)
{
  int pos_x=0;
  int pos_y=0;
  int radius = allbots[0]->radius;
  
  int num_rows=ceil(sqrt(n_bots));
  float num_start=ceil(num_rows/2);
  int cont=0;

  for (int i=-num_start; i < num_start; i++) {
    for (int j=-num_start; j < num_start; j++) {
      pos_x = i*2*(radius+3);
      pos_y = j*2*(radius+3);
      
      if (cont<n_bots)
	{
	  allbots[cont]->x = pos_x;
	  allbots[cont]->y = pos_y;
	  allbots[cont]->direction = ((float)rand()/(float)(RAND_MAX)) * (2*M_PI);
	  //allbots[cont]->direction = 0;
	}
      cont++;
      
    }
  }
}

void distribute_bots(int n_bots)
{
  int w = get_int_param("displayWidth", 500);
  int h = get_int_param("displayHeight", 500);
  const char *formation = get_string_param("formation","random");
  float f = get_float_param("distributePercent", 0.2);
  
  if(strcmp(formation, "line")         == 0) distribute_line(n_bots);
  if(strcmp(formation, "rline")        == 0) distribute_rline(n_bots);
  if(strcmp(formation, "random")       == 0) distribute_rand(n_bots, f * w, f * h);
  if(strcmp(formation, "pile")         == 0) distribute_pile(n_bots);
  if(strcmp(formation, "random_pile")  == 0) distribute_random_pile(n_bots);
  if(strcmp(formation, "circle")       == 0) distribute_circle(n_bots);
  if(strcmp(formation, "ellipse")      == 0) distribute_elipse(n_bots);
}



