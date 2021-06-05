#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// static Display *display;
// static Window window;
// static XImage *imagem;
// static Atom wmDeleteMessage;
// static GC gc;

static void exit_x11(XImage *imagem, Display *display, Window window)
{
    XDestroyImage(imagem);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

static void inicializar_x111(int size, Display *display, Window window, XImage *imagem, Atom wmDeleteMessage, GC gc)
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
        exit_x11(imagem, display, window);
        exit(0);
    }

    XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask);

    wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);
}