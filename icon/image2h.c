// gcc image2h.c -lSDL -lSDL_image -o image2h -Wall 

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

void dieSDL(char *reason)
{
  fprintf(stderr, reason, SDL_GetError());
  exit (1);
}

int main(int argc, char **argv)
{
  if (argc < 2)
    {
      printf("Give an image fiile name.\n");
      exit(1);
    }
  SDL_Surface *s = IMG_Load(argv[1]);
  if (s == NULL)
    {
      printf("Failed to load the image.\n");
      exit(1);
    }
  
   if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK) < 0)  {
    dieSDL("SDL init failed: %s\n"); 
  }

   printf ("Uint32 icon_data [] = {\n");
   int x, y;
   //int c;
   //unsigned char *bytes = s->pixels;
 
   
   for (y = 0; y < s->h; y++)
     {
       for (x = 0; x < s->w; x++)
	 {
	 /*for (c = 0; c < 4; c++)
	   {
	     int i = y*s->pitch + x*4 + c;
	     printf ("%3d, ", bytes[i]);
	     }*/

	   Uint32* pix = s->pixels + (y*s->pitch + x*4);
	   printf("0x%08x,", (unsigned int) *pix);
	 }
       printf("\n");
     }
   printf("};\n\n");
	  
   printf("int icon_w  = %d;\n", s->w);
   printf("int icon_h  = %d;\n", s->h);

   // when packing bytes,
   // the masks depend on the endian-ness of the packing and unpacking machine
   // so we do not store them.
   
   //printf("int icon_rm = %d;\n", s->format->Rmask);
   //printf("int icon_gm = %d;\n", s->format->Gmask);
   //printf("int icon_bm = %d;\n", s->format->Bmask);
   //printf("int icon_am = %d;\n", s->format->Amask);

   // alternative: store Uint32 numbers and the masks
}
