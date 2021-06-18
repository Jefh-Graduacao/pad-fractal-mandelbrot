#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <err.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <pthread.h>

#include "mandelbrot.c"
#include "fila.c"

static Display *display;
static Window window;
static XImage *imagem;
static Atom wmDeleteMessage;
static GC gc;

static fila *fila_tarefas;
static fila *fila_display;

static pthread_mutex_t *mutex;
static pthread_mutex_t *mutex_display;

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

    total = size * ii;
    total = (total + 3) & ~3;
    total *= size;
    bytes_per_pixel = ii;

    if (bytes_per_pixel != 4)
    {
        printf("Need 32bit colour screen!");
    }

    imagem = XCreateImage(display, visual, depth, ZPixmap, 0, malloc(total), size, size, 32, 0);

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

static void * display_double2(void *args)
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

            unsigned int counts = mandel_double(cr, ci);

            ((unsigned *)imagem2->data)[x + y * size] = cols[counts];
        }
    }

    pthread_mutex_lock(mutex_display);
    XPutImage(display, window, gc, imagem2, 0, 0, 0, 0, 1000, 1000);
    XFlush(display);
    pthread_mutex_unlock(mutex_display);

    printf("Calculado eixo X (%d-%d) pela thread %d\n", argsCalculo->xInicial, argsCalculo->xFinal, (int)pthread_self());
    free(argsCalculo);
}

void * thread_callback(void * args)
{
    while (1) 
    {
        pthread_mutex_lock(mutex);

        if (fila_tarefas->is_empty) {
            pthread_mutex_unlock(mutex);
            break;
        }

        ArgsCalculo *args = malloc(sizeof(ArgsCalculo));
        fila_pop(fila_tarefas, args);
        pthread_mutex_unlock(mutex);

        display_double2(args);
    }
}

void * display_thread() 
{

}

int main(void)
{
    int larguraImagem = ASIZE;
    inicializar_x11(larguraImagem);

    inicializar_cores();

    fila_display = inicializar_fila(1000, sizeof(ArgsDisplay));

    mutex = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    mutex_display = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);

    int quantidadeTarefas = 200;
    fila_tarefas = inicializar_fila(quantidadeTarefas, sizeof(ArgsCalculo));

    int quantidadePartes = quantidadeTarefas;
    int tamanhoPorParte = larguraImagem / quantidadePartes;

    struct timeval tempoInicioExecucao, tempoFimExecucao;
    gettimeofday(&tempoInicioExecucao, 0);

    for (int i = 0; i < quantidadePartes; i++) 
    {
        struct ArgsCalculo *threadArgs = (struct ArgsCalculo *)malloc(sizeof(struct ArgsCalculo));
        
        int inicio = i * tamanhoPorParte;
        int fim = inicio + tamanhoPorParte;

        threadArgs->xInicial = inicio;
        threadArgs->xFinal = fim;
        threadArgs->tamanhoDivisao = quantidadePartes;
        threadArgs->imagem = imagem;

        fila_push(fila_tarefas, threadArgs);
    }

    int qtdThreads = 50;
    pthread_t idsThreads[qtdThreads];
    for (int i = 0; i < qtdThreads; i++) {
        pthread_t idThread;
        pthread_create(&idThread, NULL, thread_callback, NULL);
        idsThreads[i] = idThread;

        printf("Thread %d criada\n", (int)idThread);        
    }
    printf("\n");

    for (int i = 0; i < qtdThreads; i++) {
        pthread_join(idsThreads[i], NULL);
        printf("\t\n*** Fim da thread %d***\n", (int)idsThreads[i]);
    }
    printf("\n");
    
    gettimeofday(&tempoFimExecucao, 0);
    long segundos = tempoFimExecucao.tv_sec - tempoInicioExecucao.tv_sec;
    long milissegundos = tempoFimExecucao.tv_usec - tempoInicioExecucao.tv_usec;
    double tempoDecorrido = segundos + milissegundos*1e-6;
    printf("\nLevou %f segundos para calcular\n", tempoDecorrido);

    // XPutImage(display, window, gc, imagem, 0, 0, 0, 0, larguraImagem, 1000);
    // XFlush(display);

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

    exit_x11();
    return 0;
}

