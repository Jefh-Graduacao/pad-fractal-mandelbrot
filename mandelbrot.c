#include <stdio.h>
#include <X11/Xlib.h>

#include "argsCalculo.h"


#define MAX_ITER (1 << 15)
static unsigned int cols[MAX_ITER + 1];

static void inicializar_cores(void)
{
    int i;

    for (i = 0; i < MAX_ITER; i++)
    {
        char r = (rand() & 0xff) * MAX_ITER / (i + MAX_ITER + 1);
        char g = (rand() & 0xff) * MAX_ITER / (i + MAX_ITER + 1);
        char b = (rand() & 0xff) * MAX_ITER / (i + MAX_ITER + 1);

        cols[i] = b + 256 * g + 256 * 256 * r;
    }

    cols[MAX_ITER] = 0;
}

static unsigned mandel_double(double cr, double ci)
{
    double zr = cr, zi = ci;
    double tmp;

    unsigned i;

    for (i = 0; i < MAX_ITER; i++)
    {
        tmp = zr * zr - zi * zi + cr;
        zi *= 2 * zr;
        zi += ci;
        zr = tmp;

        if (zr * zr + zi * zi > 4.0)
            break;
    }

    return i;
}

static double xmin = -2;
static double xmax = 1.0;
static double ymin = -1.5;
static double ymax = 1.5;

#define ASIZE 1000

static void * display_double(void *args)
{
    struct ArgsCalculo *argsCalculo = (struct ArgsCalculo *)args;

    int size = ASIZE;

    double xscal = (xmax - xmin) / size;
    double yscal = (ymax - ymin) / size;

    XImage *imagem2 = argsCalculo->imagem;
    for (int y = 0; y < size; y++)
    {
        for (int x = argsCalculo->xInicial; x < argsCalculo->xFinal; x++)
        {
            double cr = xmin + x * xscal;
            double ci = ymin + y * yscal;

            unsigned counts = mandel_double(cr, ci);

            ((unsigned *)imagem2->data)[x + y * size] = cols[counts];
        }
    }

    printf("Calculado eixo X (%d-%d) pela thread %d\n", argsCalculo->xInicial, argsCalculo->xFinal, (int)pthread_self());
    free(argsCalculo);
}