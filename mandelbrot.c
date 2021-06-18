#include <stdio.h>
#include <X11/Xlib.h>

#include "estruturas.h"

#define MAX_ITER (1 << 15)
static unsigned int cols[MAX_ITER + 1];

static void inicializar_cores()
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

static void * display_double(void *args)
{
    struct DadosCalculo *paramsCalculo = (struct DadosCalculo *)args;

    int size = paramsCalculo->tamanhoImagem;

    double xscal = (xmax - xmin) / size;
    double yscal = (ymax - ymin) / size;

    XImage *imagemX11 = paramsCalculo->imagem;

    Ponto pontoInicial = paramsCalculo->pontoInicial;
    Ponto pontoFinal = paramsCalculo->pontoFinal;

    for (int y = pontoInicial.y; y < pontoFinal.y; y++)
    {
        for (int x = pontoInicial.x; x < pontoFinal.x; x++)
        {
            double c_real = xmin + x * xscal;
            double c_imaginario = ymin + y * yscal;

            unsigned counts = mandel_double(c_real, c_imaginario);

            int indicePixel = x + y * size;
            ((unsigned *)imagemX11->data)[indicePixel] = cols[counts];
        }
    }

    printf(
        "Calculado intervalo (%d, %d) -> (%d, %d) pela thread %d\n", 
        paramsCalculo->pontoInicial.x, paramsCalculo->pontoInicial.y,
        paramsCalculo->pontoFinal.x, paramsCalculo->pontoFinal.y,
        (int)pthread_self()
    );
    free(paramsCalculo);
}