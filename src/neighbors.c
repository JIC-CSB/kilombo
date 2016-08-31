/* Functions for finding the neighbors of all robots efficiently.
 *
 */

#include<stdio.h>
#include<math.h>

#define NDEBUG // define to turn assertions off
#include<assert.h>
#include"skilobot.h"
#include"cd_matrix.h"
#include "neighbors.h"

pv_matrix grid_cache;
coord2D gc_offset = {0, 0};
coord2D gc_cell_sz = {100, 100};

// initialized in update_all_bots before movement
coord2D max_coord, min_coord;

size_t bot2gc_x(double x)
{
  // using int here to be able to detect if it is ever negative
  int xi = (x-gc_offset.x) / gc_cell_sz.x;
  // printf ("x:%f, max_coord.x:%f, xi:%d grid_cache.x_size:%zd\n", x, max_coord.x, xi, grid_cache.x_size);
  assert (xi >= 0);                  // make sure we always return a valid index
  assert (xi < grid_cache.x_size);
  return xi;
}

size_t bot2gc_y(double y)
{
  int yi = (y-gc_offset.y) / gc_cell_sz.y;
  assert (yi >= 0);                  // make sure we always return a valid index
  assert (yi < grid_cache.y_size);
  return yi;
}

void prepare_grid_cache(double cr)
{
  //printf("area: %g, %g - %g, %g\n", min_coord.x, min_coord.y, max_coord.x, max_coord.y);
  // for now we fill the matrix from scratch
  // it might be good to only do that if it had to be extended
  matrix_clear_all(&grid_cache);
  
  assert(max_coord.x >= min_coord.x);
  assert(max_coord.y >= min_coord.y);

    
  double eps = .1; // small margin to avoid rounding trouble
                   // risk: rightmost point's x+cr gets rounded to one past the last index in matrix.
                   // TODO: ceil is dangerous in some (very rare) cases -- DONE?

  // increase grid cell size to cr
  if (cr+eps > gc_cell_sz.x)
    gc_cell_sz.x = cr+eps;  // eps is not crucial here, just to ensure we need to loop over only 3 cells
  if (cr+eps > gc_cell_sz.y)
    gc_cell_sz.y = cr+eps;

  size_t x_range = ceil((max_coord.x - min_coord.x + 2*cr + 2*eps)/gc_cell_sz.x);
  size_t y_range = ceil((max_coord.y - min_coord.y + 2*cr + 2*eps)/gc_cell_sz.y);
  // here eps is important, to ensure the grid extends a bit beyond the outermost robots
  
  // this looks like a lot of effort compared to just creating a new matrix
  // but it saves us expensive reallocation of the per-cell arrays
  if (x_range < grid_cache.x_size)
    x_range = grid_cache.x_size;
  
  if (y_range < grid_cache.y_size)
    y_range = grid_cache.y_size;
  
  if (x_range > grid_cache.x_size || y_range > grid_cache.y_size)
    matrix_extend(&grid_cache, 0, x_range-grid_cache.x_size, 0, y_range-grid_cache.y_size);
  
  gc_offset.x = min_coord.x - cr - eps;
  gc_offset.y = min_coord.y - cr - eps;
}


void store_cache(kilobot * bot)
{
  assert(bot != NULL);
  
  p_vec * cell = matrix_get(&grid_cache, bot2gc_x(bot->x), bot2gc_y(bot->y));
  p_vec_push(cell, bot);
}


int check_bots_in_bounds(int n_bots)
{
  int i;
  kilobot *bot;
  for (i = 0; i < n_bots; i++)
    {
      bot = allbots[i];
      assert(bot->x >= min_coord.x);
      assert(bot->x <= max_coord.x);
      assert(bot->y >= min_coord.y);
      assert(bot->y <= max_coord.y);
    }

  return 1;
}

/* Update the bots' interactions with each other.
 *
 * - Move clashing bots apart.
 * - Update which bots can communicate with each other.
 *  -- pointers to bots in range are stored in bot->in_range[]
 */
void update_interactions_grid (int n_bots)
{
  if (user_obstacles != NULL) {
    double push_x, push_y;

    for (int i=0; i<n_bots; i++) {
      if (user_obstacles(allbots[i]->x, allbots[i]->y, &push_x, &push_y)){
        allbots[i]->x += push_x;
	allbots[i]->y += push_y;
      }
    }
  }

  // initialize bounding box
  max_coord.x = min_coord.x = allbots[0]->x;
  max_coord.y = min_coord.y = allbots[0]->y;

  double cr = allbots[0]->cr;
  double sq_r = allbots[0]->radius * allbots[0]->radius;
  double sq_cr = cr * cr;

  int i;
  kilobot *bot;
  // bounding box
  for (i = 0; i < n_bots; i++)
    {
      bot = allbots[i];

      bot->x > max_coord.x ? (max_coord.x = bot->x) :
	(bot->x < min_coord.x ? (min_coord.x = bot->x) : 0);
      
      bot->y > max_coord.y ? (max_coord.y = bot->y) :
	(bot->y < min_coord.y ? (min_coord.y = bot->y) : 0);
    }
  // use assert here so that the call gets compiled out in release
  assert(check_bots_in_bounds(n_bots));
  prepare_grid_cache(cr);
  assert(check_bots_in_bounds(n_bots));
   
   // insert bots into the grid
   for (i=0; i<n_bots; i++)
     {
       store_cache(allbots[i]);
       allbots[i]->n_in_range = 0;
     }
   assert(check_bots_in_bounds(n_bots));
   
   // loop over the bots, find neighbors using the grid
   for (int i=0; i<n_bots; i++) {
     kilobot * cur = allbots[i];

     // range of cells we have to check
     //     printf ("bot:%d x:%f y:%f cr:%f\n", i, cur->x, cur->y, cr);
     size_t low_x = bot2gc_x(cur->x - cr);
     size_t high_x = bot2gc_x(cur->x + cr);
     size_t low_y = bot2gc_y(cur->y - cr);
     size_t high_y = bot2gc_y(cur->y + cr);

     //printf("(%d, %d, %d, %d)", low_x, high_x, low_y, high_y);
     
     // FIXME: what about movement? update cache?
     for (size_t y=low_y; y<=high_y; y++)
       for (size_t x=low_x; x<=high_x; x++)
	 {
	   p_vec * cell = matrix_get(&grid_cache, x, y);
	   //printf("cs:%d ", cell->size);
	   for (size_t b=0; b<cell->size; b++)
	     {
	       kilobot * other = cell->data[b];
	       assert(other != NULL);
	       
	       // only process each pair once and don't pair with self
	       if (other <= cur)
		 continue;
	       
	       double sq_bd = bot_sq_dist(cur, other);
	       if (sq_bd < sq_cr) {
		 //if (i == 0) printf("%d and %d in range\n", i, j);
		 cur->in_range[cur->n_in_range++] = other->ID;  // ugly conversion back to index
		 other->in_range[other->n_in_range++] = cur->ID;
	       }
	     }
	 }
      }

   
   // Move colliding robots appart, using the list of neighbors in range.
   // Note: Once the bots are moved, the grid cache is no longer valid
   
   int j;
   for (i = 0; i < n_bots; i++)
     {
      kilobot * cur = allbots[i];
      for (j = 0; j < cur->n_in_range; j++)
	{
	  kilobot * other = allbots[cur->in_range[j]];
	  double sq_bd = bot_sq_dist(cur, other);
	  if (sq_bd < (4 * sq_r))
	    {
	    //	  printf("Whack %d %d\n", i, j);
        separate_clashing_bots(cur, other);
	    // we move the bots, this changes the distance.
	    // so bd should be recalculated.
	    // but we only need it below to tell if the bots are
	    // in communications range, and after resolving the collision, 
	    // they will still be. 
	  }
	}
     }
	   
}
