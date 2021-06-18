#include <X11/Xlib.h>

typedef struct Ponto
{
    int x;
    int y;
} Ponto;

typedef struct DadosCalculo
{
    XImage *imagem;
    int tamanhoImagem;
    Ponto pontoInicial;
    Ponto pontoFinal;
} DadosCalculo;

typedef struct DadosDisplay 
{
    Ponto pontoInicial;
    Ponto pontoFinal;
} DadosDisplay;
