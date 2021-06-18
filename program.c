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
static GC gc;
static Atom wmDeleteMessage;

static fila *fila_tarefas;
static fila *fila_result;

static pthread_mutex_t *mutex;
static pthread_mutex_t *mutex_display;
static pthread_mutex_t *mutex_fila_result;

static pthread_cond_t *condicaoMtoLoca;

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

        result_data *result = malloc(sizeof(result_data));
        result->xi = args->xInicial;
        result->xf = args->xFinal;
        result->yi = 0;
        result->yf = 1000;

        display_double(args);

        pthread_mutex_lock(mutex_fila_result);
        fila_push(fila_result, result);
        pthread_mutex_unlock(mutex_fila_result);
        pthread_cond_signal(condicaoMtoLoca);
    }
}

static void * consumer(void *data) 
{
    int quantidadeTarefas = 50;
    int tarefasRealizas = 0;

    while (1) 
    {
        if (tarefasRealizas == quantidadeTarefas) 
        {
            XFlush(display);
            return NULL;
        }

        pthread_mutex_lock(mutex_fila_result);
        if (fila_result->is_empty) {            
            pthread_cond_wait(condicaoMtoLoca, mutex_fila_result);
            pthread_mutex_unlock(mutex_fila_result);
            continue;
        }

        result_data *result = malloc(sizeof(result_data));        
        fila_pop(fila_result, result);
        result->yf = 1000;

        XPutImage(
            display, window, gc, imagem,
            result->xi, result->yi, result->xi, result->yi, (result->xf - result->xi + 1), (result->yf - result->yi + 1)
        );

        pthread_mutex_unlock(mutex_fila_result);
        
        tarefasRealizas++;
        free(result);
    }
}

int main(void)
{
    int larguraImagem = ASIZE;
    inicializar_x11(larguraImagem);

    inicializar_cores();

    fila_result = inicializar_fila(100000, sizeof(ArgsDisplay));

    mutex = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    mutex_display = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    mutex_fila_result = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));

    condicaoMtoLoca = (pthread_cond_t *) malloc(sizeof (pthread_cond_t));
    pthread_cond_init(condicaoMtoLoca, NULL);

    pthread_mutex_init(mutex, NULL);

    int quantidadeTarefas = 50;
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

    int qtdThreads = 10;
    pthread_t idsThreads[qtdThreads];
    for (int i = 0; i < qtdThreads; i++) {
        pthread_t idThread;
        pthread_create(&idThread, NULL, thread_callback, NULL);
        idsThreads[i] = idThread;

        printf("Thread %d criada\n", (int)idThread);        
    }
    printf("\n");
    
    pthread_t consumerThreadId;
    pthread_create(&consumerThreadId, NULL, consumer, NULL);

    for (int i = 0; i < qtdThreads; i++) {
        pthread_join(idsThreads[i], NULL);
        printf("\t\n*** Fim da thread %d***\n", (int)idsThreads[i]);
    }
    printf("\n");
    
    pthread_join(consumerThreadId, NULL);

    gettimeofday(&tempoFimExecucao, 0);
    long segundos = tempoFimExecucao.tv_sec - tempoInicioExecucao.tv_sec;
    long milissegundos = tempoFimExecucao.tv_usec - tempoInicioExecucao.tv_usec;
    double tempoDecorrido = segundos + milissegundos*1e-6;
    printf("\nLevou %f segundos para calcular\n", tempoDecorrido);

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

        if (event.type = KeyPress)
        {
            char key_buffer[128];
            XLookupString(&event.xkey, key_buffer, sizeof key_buffer, &key, NULL);

            if (key == XK_Escape)
                break;
        }

        if ((event.type == ClientMessage) && ((Atom)event.xclient.data.l[0] == wmDeleteMessage))
        {
            break;
        }
    }

    exit_x11();
    return 0;
}

