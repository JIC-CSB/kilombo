#ifndef __NEIGHBORS_H
#define __NEIGHBORS_H
void update_interactions_grid (int n_bots);

static inline double bot_sq_dist(kilobot *bot1, kilobot *bot2)
{
  double x1 = bot1->x;
  double x2 = bot2->x;
  double y1 = bot1->y;
  double y2 = bot2->y;

  return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}

#endif



