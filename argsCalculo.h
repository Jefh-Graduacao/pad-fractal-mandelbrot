#include <X11/Xlib.h>

typedef struct ArgsCalculo
{
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