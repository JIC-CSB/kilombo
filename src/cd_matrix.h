#ifndef CD_MATRIX_H
#define CD_MATRIX_H

#define NDEBUG // define to turn assertions off
#include "assert.h"
#include <time.h>

// let's keep at least this one generic
typedef struct {
  void ** data;
  size_t allocated;
  size_t size;
} p_vec;

void p_vec_init(p_vec * v, size_t pre_alloc)
	{
	v->allocated = pre_alloc;
	if (v->allocated > 0)
		{
		v->data = calloc(v->allocated, sizeof(void *));
		assert(v->data != NULL);
		}
	else
		v->data = NULL;

	v->size = 0;
	}

void p_vec_reserve(p_vec * v, size_t new_size)
	{
	// we behave like STL vector here
	if (new_size <= v->size)
		return;

	void ** new_data = realloc(v->data, new_size * sizeof(void*));
	assert(new_data != NULL);
	v->data = new_data;
	v->allocated = new_size;
	}

#define INITIAL_SIZE 8
void p_vec_push(p_vec * v, void * el) 
	{
	assert(v->size <= v->allocated);

	if (v->size == v->allocated)
	  //p_vec_reserve(v, (v->allocated == 0 ? 1 : 2*v->allocated));
	  p_vec_reserve(v, (v->allocated < INITIAL_SIZE ? INITIAL_SIZE : 2*v->allocated));

	v->data[(v->size)++] = el;
	}

void p_vec_rm_unsorted(p_vec * v, void * el)
	{
	for (size_t i=0; i<v->size; i++)
		if (v->data[i] == el)
			{
			v->data[i] = v->data[--(v->size)];
			return;
			}

	assert(v->size == 0);
	}

// another STLism
void p_vec_clear(p_vec * v)
	{
	v->size = 0;
	}


typedef struct {
  p_vec * data;
  size_t x_size, y_size;

} pv_matrix;


size_t matrix_c2i(pv_matrix * m, size_t x, size_t y)
	{
	assert(x < m->x_size && y < m->y_size);

	return y*m->x_size + x;
	}

void matrix_init(pv_matrix * m, size_t x, size_t y)
	{
	m->x_size = x;
	m->y_size = y;
	size_t n = m->x_size * m->y_size;
	m->data = calloc(n, sizeof(p_vec));

	assert(m->data != NULL);

	//for (size_t i=0; i<n; i++)
	//	p_vec_init(&(m->data[i]), 0);
	// calloc already zeroes the data - equivalent to p_vec_init with size 0
	}

p_vec * matrix_get(pv_matrix * m, size_t x, size_t y)
	{
	assert(x>=0 && x<m->x_size);
	assert(y>=0 && y<m->y_size);

	return &(m->data[matrix_c2i(m, x, y)]);
	}

void matrix_extend(pv_matrix * m, size_t xmi_p, size_t xma_p, size_t ymi_p, size_t yma_p)
	{
	  //clock_t t = clock();
	  
	  // we don't do shrinking
	assert(xmi_p >= 0);
	assert(xma_p >= 0);
	assert(ymi_p >= 0);
	assert(yma_p >= 0);

	size_t new_xsize = m->x_size + xmi_p + xma_p;
	size_t new_ysize = m->y_size + ymi_p + yma_p;

	// create a bigger matrix
	pv_matrix new_m;
	matrix_init(&new_m, new_xsize, new_ysize);
	
	// copy over the data from the old matrix
	for (size_t x=0; x<m->x_size; x++)
		for (size_t y=0; y<m->y_size; y++)
			*matrix_get(&new_m, x+xmi_p, y+ymi_p) =
				*matrix_get(m, x, y);

	// now get rid of old matrix' memory
	free(m->data);

	// all of the important stuff is pointers, so just copy everything
	*m = new_m;

	/* t = clock() - t;
	double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
	printf("matrix_extend %d %d - %f s\n", new_xsize, new_ysize, time_taken);
	*/
	}

void matrix_clear_all(pv_matrix * m)
	{
	size_t sz = m->x_size * m->y_size;

	for (size_t i=0; i<sz; i++)
		p_vec_clear(&(m->data[i]));
	}


#endif  // CD_MATRIX_H
