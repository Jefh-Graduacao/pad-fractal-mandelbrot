#include <X11/Xlib.h>

struct ArgsCalculo
{
    int tamanhoDivisao;
    int parteX;
    int xInicial;
    int xFinal;    
    XImage *imagem;
} ArgsCalculo;