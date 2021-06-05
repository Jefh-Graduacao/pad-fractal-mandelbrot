#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <err.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <pthread.h>

#include "mandelbrot.c"
// #include "x11-helpers.c"

static Display *display;
static Window window;
static XImage *imagem;
static Atom wmDeleteMessage;
static GC gc;

static void exit_x11(void)
{
    XDestroyImage(imagem);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

static void inicializar_x11(int size)
{
    int bytes_per_pixel;

    // Tentativa de abrir um display
    display = XOpenDisplay(NULL);
    if (!display)
    {
        exit(0);
    }

    unsigned long pixelBranco = WhitePixel(display, DefaultScreen(display));
    unsigned long pixelPreto = BlackPixel(display, DefaultScreen(display));

    // Criação da janela
    window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, size, size, 0, pixelPreto, pixelBranco);

    /* We want to be notified when the window appears */
    XSelectInput(display, window, StructureNotifyMask);

    /* Make it appear */
    XMapWindow(display, window);

    // Aguarda a tela ser mostrada
    while (1)
    {
        XEvent e;
        XNextEvent(display, &e);
        if (e.type == MapNotify)
            break;
    }

    XTextProperty tp;
    char name[128] = "Mandelbrot";
    char *n = name;
    Status st = XStringListToTextProperty(&n, 1, &tp);
    if (st)
        XSetWMName(display, window, &tp);

    /* Wait for the MapNotify event */
    XFlush(display);

    int ii, jj;
    int depth = DefaultDepth(display, DefaultScreen(display));
    Visual *visual = DefaultVisual(display, DefaultScreen(display));
    int total;

    /* Determine total bytes needed for image */
    ii = 1;
    jj = (depth - 1) >> 2;
    while (jj >>= 1)
        ii <<= 1;

    /* Pad the scanline to a multiple of 4 bytes */
    total = size * ii;
    total = (total + 3) & ~3;
    total *= size;
    bytes_per_pixel = ii;

    if (bytes_per_pixel != 4)
    {
        printf("Need 32bit colour screen!");
    }

    /* Make bitmap */
    imagem = XCreateImage(display, visual, depth, ZPixmap, 0, malloc(total), size, size, 32, 0);

    /* Init GC */
    gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, pixelPreto);

    if (bytes_per_pixel != 4)
    {
        printf("Need 32bit colour screen!\n");
        exit_x11();
        exit(0);
    }

    XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask);

    wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);
}

int main(void)
{
    int larguraImagem = ASIZE;
    inicializar_x11(larguraImagem);
    //inicializar_x111(larguraImagem, display, window, imagem, wmDeleteMessage, gc);

    inicializar_cores();

    int quantidadePartes = 200;
    int tamanhoPorParte = larguraImagem / quantidadePartes;
    pthread_t idsThreads[quantidadePartes];

    for (int i = 0; i < quantidadePartes; i++)
    {
        struct ArgsCalculo *threadArgs = (struct ArgsCalculo *)malloc(sizeof(struct ArgsCalculo));

        int inicio = i * tamanhoPorParte;
        int fim = inicio + tamanhoPorParte;

        threadArgs->xInicial = inicio;
        threadArgs->xFinal = fim;
        threadArgs->tamanhoDivisao = quantidadePartes;
        threadArgs->imagem = imagem;

        pthread_t idThread;
        pthread_create(&idThread, NULL, display_double, (void *)threadArgs);
        idsThreads[i] = idThread;

        printf("Thread %d criada\n", (int)idThread);
    }

    for (int i = 0; i < quantidadePartes; i++) {
        pthread_join(idsThreads[i], NULL);
    }

    XPutImage(display, window, gc, imagem, 0, 0, 0, 0, larguraImagem, 1000);
    XFlush(display);

    while (1)
    {
        XEvent event;
        KeySym key;
        char text[255];

        XNextEvent(display, &event);

        /* Just redraw everything on expose */
        if ((event.type == Expose) && !event.xexpose.count)
        {
            XPutImage(display, window, gc, imagem, 0, 0, 0, 0, ASIZE, ASIZE);
        }

        /* Press 'q' to quit */
        if ((event.type == KeyPress) && XLookupString(&event.xkey, text, 255, &key, 0) == 1)
        {
            if (text[0] == 'q')
                break;
        }

        /* Or simply close the window */
        if ((event.type == ClientMessage) && ((Atom)event.xclient.data.l[0] == wmDeleteMessage))
        {
            break;
        }
    }

    /* Done! */
    exit_x11();

    return 0;
}

