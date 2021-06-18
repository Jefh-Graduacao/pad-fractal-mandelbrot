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
#include "x11-helpers.c"

static fila *buffer_tarefas;
static fila *buffer_display;

static pthread_mutex_t *mutex_buffer_tarefas;
static pthread_mutex_t *mutex_buffer_display;

static pthread_cond_t *condicao_buffer_display;

void * thread_produtora()
{
    while (1) 
    {
        pthread_mutex_lock(mutex_buffer_tarefas);

        if (buffer_tarefas->esta_vazia) 
        {
            pthread_mutex_unlock(mutex_buffer_tarefas);
            break;
        }

        DadosCalculo *args = malloc(sizeof(DadosCalculo));
        fila_pop(buffer_tarefas, args);
        pthread_mutex_unlock(mutex_buffer_tarefas);

        DadosDisplay *result = malloc(sizeof(DadosDisplay));
        
        result->pontoInicial = args->pontoInicial;
        result->pontoFinal = args->pontoFinal;

        display_double(args);

        pthread_mutex_lock(mutex_buffer_display);
        fila_push(buffer_display, result);
        pthread_mutex_unlock(mutex_buffer_display);
        pthread_cond_signal(condicao_buffer_display);
    }
}

void * thread_consumidora() 
{
    int tarefasRealizas = 0;

    while (1) 
    {
        if (tarefasRealizas == buffer_display->capacidade) 
        {
            XFlush(display);
            return NULL;
        }

        pthread_mutex_lock(mutex_buffer_display);
        if (buffer_display->esta_vazia) 
        {            
            pthread_cond_wait(condicao_buffer_display, mutex_buffer_display);
            pthread_mutex_unlock(mutex_buffer_display);
            continue;
        }

        DadosDisplay *result = malloc(sizeof(DadosDisplay));        
        fila_pop(buffer_display, result);

        adicionar_imagem_x11(
            result->pontoInicial.x, result->pontoInicial.y, result->pontoInicial.x, result->pontoInicial.y, 
            (result->pontoFinal.x - result->pontoInicial.x + 1), (result->pontoFinal.y - result->pontoInicial.y + 1)
        );

        pthread_mutex_unlock(mutex_buffer_display);
        
        tarefasRealizas++;
        free(result);
    }
}

int main(void)
{
    int tamanhoImagem = 1000;
    int qtdThreads = 100;
    int quantidadeDivisoes = 10;
    int quantidadeTarefas = quantidadeDivisoes * quantidadeDivisoes;

    inicializar_x11(tamanhoImagem);

    inicializar_cores();

    buffer_tarefas = inicializar_fila(quantidadeTarefas, sizeof(DadosCalculo));
    buffer_display = inicializar_fila(quantidadeTarefas, sizeof(DadosDisplay));

    mutex_buffer_tarefas = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    mutex_buffer_display = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));

    condicao_buffer_display = (pthread_cond_t *) malloc(sizeof (pthread_cond_t));
    pthread_cond_init(condicao_buffer_display, NULL);

    pthread_mutex_init(mutex_buffer_tarefas, NULL);

    int quantidadePartes = quantidadeDivisoes;
    int tamanhoPorParte = tamanhoImagem / quantidadePartes;

    for (int y = 0; y < quantidadePartes; y++) 
    {
        for (int x = 0; x < quantidadePartes; x++) 
        {
            DadosCalculo *argsCalculo = (DadosCalculo *)malloc(sizeof(DadosCalculo));
            
            argsCalculo->pontoInicial.x = x * tamanhoPorParte;
            argsCalculo->pontoInicial.y = y * tamanhoPorParte;

            argsCalculo->pontoFinal.x = (x * tamanhoPorParte) + tamanhoPorParte;
            argsCalculo->pontoFinal.y = (y * tamanhoPorParte) + tamanhoPorParte;

            argsCalculo->imagem = imagem;
            argsCalculo->tamanhoImagem = tamanhoImagem;

            fila_push(buffer_tarefas, argsCalculo);
        }
    }

    struct timeval tempoInicioExecucao, tempoFimExecucao;
    gettimeofday(&tempoInicioExecucao, 0);

    pthread_t idsThreads[qtdThreads];
    for (int i = 0; i < qtdThreads; i++) {
        pthread_t idThread;
        pthread_create(&idThread, NULL, thread_produtora, NULL);
        idsThreads[i] = idThread;

        printf("Thread %d criada\n", (int)idThread);        
    }
    printf("\n");
    
    pthread_t consumerThreadId;
    pthread_create(&consumerThreadId, NULL, thread_consumidora, NULL);    
    pthread_join(consumerThreadId, NULL);

    gettimeofday(&tempoFimExecucao, 0);
    long segundos = tempoFimExecucao.tv_sec - tempoInicioExecucao.tv_sec;
    long milissegundos = tempoFimExecucao.tv_usec - tempoInicioExecucao.tv_usec;
    double tempoDecorrido = segundos + milissegundos * 1e-6;
    printf("\nLevou %f segundos para calcular\n", tempoDecorrido);

    loop_interface_grafica(tamanhoImagem);
    finalizar_x11();

    return 0;
}