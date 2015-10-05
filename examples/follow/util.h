enum {STOP,LEFT,RIGHT,STRAIGHT};

#define MIN(a,b)((a)<(b)?a:b)
#define MAX(a,b)((a)>(b)?a:b)

void smooth_set_motors(uint8_t ccw, uint8_t cw) ;
void motion(uint8_t type);
float clipf(float f, float min, float max);

extern uint8_t colorNum[];
