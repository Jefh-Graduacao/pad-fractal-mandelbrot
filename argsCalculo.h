#include <X11/Xlib.h>

typedef struct ArgsCalculo
{
    int tamanhoImagem;
    int tamanhoDivisao;
    int parteX;
    int xInicial;
    int xFinal;
    XImage *imagem;
} ArgsCalculo;

typedef struct ArgsDisplay
{
    int x;
    int y;
    int cor;
} ArgsDisplay;

typedef struct result_data {
  int xi;
  int xf;
  int yi;
  int yf;
} result_data;