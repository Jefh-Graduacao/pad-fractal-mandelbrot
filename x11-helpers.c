#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static Display *display;
static Window window;
static XImage *imagem;
static GC gc;
static Atom wmDeleteMessage;

static void finalizar_x11(void)
{
    XDestroyImage(imagem);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

static void inicializar_x11(int tamanhoImagem)
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
    window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, tamanhoImagem, tamanhoImagem, 0, pixelPreto, pixelBranco);

    // Solicita notificação quando a tela aparecer
    XSelectInput(display, window, StructureNotifyMask);

    // Mostra a tela
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

    XFlush(display);

    int ii, jj;
    int depth = DefaultDepth(display, DefaultScreen(display));
    Visual *visual = DefaultVisual(display, DefaultScreen(display));
    int total;

    ii = 1;
    jj = (depth - 1) >> 2;
    while (jj >>= 1)
        ii <<= 1;

    total = tamanhoImagem * ii;
    total = (total + 3) & ~3;
    total *= tamanhoImagem;
    bytes_per_pixel = ii;

    if (bytes_per_pixel != 4)
    {
        printf("Need 32bit colour screen!");
    }

    imagem = XCreateImage(display, visual, depth, ZPixmap, 0, malloc(total), tamanhoImagem, tamanhoImagem, 32, 0);

    gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, pixelPreto);

    if (bytes_per_pixel != 4)
    {
        printf("Need 32bit colour screen!\n");
        finalizar_x11();
        exit(0);
    }

    XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask);
    
    wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);
}

void loop_interface_grafica(int tamanhoImagem)
{
    while (1)
    {
        XEvent event;
        KeySym key;

        XNextEvent(display, &event);

        if ((event.type == Expose) && !event.xexpose.count)
        {
            XPutImage(display, window, gc, imagem, 0, 0, 0, 0, tamanhoImagem, tamanhoImagem);
        }
        
        if (event.type == KeyPress)
        {
            char key_buffer[128];
            XLookupString(&event.xkey, key_buffer, sizeof key_buffer, &key, NULL);
            
            if (key == XK_Escape)
            {
                break;
            }
        }

        if ((event.type == ClientMessage) && ((Atom)event.xclient.data.l[0] == wmDeleteMessage))
        {
            break;
        }
    }
}