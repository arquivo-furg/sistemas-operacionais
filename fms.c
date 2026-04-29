#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

const int N = 64; // Tamanho máximo para o nome do binário

// Variáveis partilhadas entre a thread de timeout e a principal
// Variável global para comunicação entre a thread de timeout e o main
pthread_mutex_t mutex;
pthread_cond_t cond;

volatile int timeout_occurred = 0; // indica que o tempo acabou
pid_t pid_filho = 0; // PID do filho para que a thread possa matá-lo

// Estrutura para passar o timeout à thread de monitoramento
struct timeout_info
{
    unsigned int segundos;
};

// Função executada pela thread de monitoramento
void *thread_timeout(void *arg)
{
    struct timeout_info *info = (struct timeout_info *)arg;
    unsigned int seg = info->segundos;

    // Dorme pelo tempo do timeout
    sleep(seg);

    // Ao acordar, sinaliza timeout à thread principal
    pthread_mutex_lock(&mutex);
    timeout_occurred = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    // Mata o processo filho imediatamente
    if (pid_filho > 0)
    {
        kill(pid_filho, SIGKILL);
    }

    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    // Quotas globais válidas para toda a sessão do FMS
    double quota_cpu_total; // tempo máximo de CPU (usuário+sistema) em segundos
    long limite_mem_kb; // memória máxima permitida (em KB)

    printf("FMS >> Bem-vindo ao Fork Monitor & Schedule\n");

    printf("FMS >> Informe a quota global de tempo de CPU (em segundos): ");
    scanf("%lf", &quota_cpu_total);

    printf("FMS >> Informe o limite global de memoria (em KB): ");
    scanf("%ld", &limite_mem_kb);

    double cpu_acumulada = 0.0; // total de CPU já consumido
    int continuar = 1;

    // Laço principal: pede binários enquanto houver quota
    while (continuar && cpu_acumulada < quota_cpu_total)
    {

        // Solicita o nome do binário
        char nome[N];

        printf("\nFMS >> Introduza o caminho do binario a executar (ou 'sair'): ");
        scanf("%63s", nome);

        if (strcmp(nome, "sair") == 0)
            break;

        // Quota de CPU deste binário
        // Guardada para uso futuro no controle individual de CPU
        double quota_binario = quota_cpu_total;

        // Timeout (tempo de relógio)
        unsigned int timeout;

        printf("FMS >> Timeout para este binario (0 = sem timeout): ");
        scanf("%u", &timeout);

        // Criação do processo filho
        int pid = fork();

        // Processos pai e filho executam a linha porém com PIDs diferentes
        printf("PID do processo: <%d>\n", getpid());

        if (pid < 0) // Erro ao criar o processo
        {
            printf("Erro ao criar o processo!\n");
            return 1;
        }
        else if (pid == 0) // Processo filho
        {
            printf("Processo filho iniciado com PID %d\n", getpid());

            // Substitui a imagem do processo pelo binário solicitado
            // O argv[0] é o nome do FMS, o binário receberá os mesmos argumentos
            // É útil se for um programa que usa argc/argv
            execve(nome, argv, 0);

            // Se execve retornar, houve erro
            printf("Erro ao executar o binario '%s'\n", nome);
            return 1;
        }
        else // Processo pai (FMS)
        {
            printf("FMS >> processo filho criado com PID %d\n", pid);
            pid_filho = pid; // Guarda PID do filho para uso na thread

            // Estrutura para recolher resource usage do filho
            struct rusage uso;

            int status;
            int filho_terminou = 0;

            if (timeout > 0)
            {
                // Com timeout usa thread e mutex/cond para monitorar o processo filho e o tempo
                pthread_t tid;

                struct timeout_info tinfo;
                tinfo.segundos = timeout;

                // Inicializa mutex e condição
                pthread_mutex_init(&mutex, NULL);
                pthread_cond_init(&cond, NULL);

                timeout_occurred = 0; // reset para cada execução

                // Cria a thread de monitoramento
                pthread_create(&tid, NULL, thread_timeout, (void *)&tinfo);

                // Thread principal: espera até que o filho termine ou timeout ocorra
                while (1)
                {
                    // Verifica se o filho terminou
                    int w = wait4(pid, &status, WNOHANG, &uso);

                    if (w == pid)
                    {
                        filho_terminou = 1;
                        break;
                    }

                    // Bloqueia no mutex e verifica timeout
                    pthread_mutex_lock(&mutex);

                    if (timeout_occurred)
                    {
                        pthread_mutex_unlock(&mutex);
                        break;
                    }

                    // Aguarda um sinal (timeout) ou um breve período para evitar busy wait
                    // Usamos tempo limite curto para poder reavaliar wait4
                    struct timespec ts;

                    clock_gettime(CLOCK_REALTIME, &ts);

                    ts.tv_nsec += 100000000; // 100 ms

                    if (ts.tv_nsec >= 1000000000)
                    {
                        ts.tv_nsec -= 1000000000;
                        ts.tv_sec += 1;
                    }

                    pthread_cond_timedwait(&cond, &mutex, &ts);

                    pthread_mutex_unlock(&mutex);
                }

                // Se o filho ainda estiver rodando (timeout ocorreu) encerra
                if (timeout_occurred && !filho_terminou)
                {
                    kill(pid, SIGKILL);

                    // Recolhe status e estatísticas finais do filho
                    wait4(pid, &status, 0, &uso);

                    printf("FMS >> TEMPO ESGOTADO! Processo filho %d foi morto.\n", pid);
                }
                else if (filho_terminou)
                {
                    // Cancela a thread de timeout caso o filho tenha terminado antes
                    pthread_cancel(tid);

                    // Espera a thread encerrar
                    pthread_join(tid, NULL);
                }

                pthread_mutex_destroy(&mutex);
                pthread_cond_destroy(&cond);
            }
            else
            {
                // Sem timeout: espera simples com recolha de resource usage
                wait4(pid, &status, 0, &uso);
            }

            // Recolher e apresentar as estatísticas do filho
            double cpu_filho =
                uso.ru_utime.tv_sec + uso.ru_utime.tv_usec / 1e6 +
                uso.ru_stime.tv_sec + uso.ru_stime.tv_usec / 1e6;

            long mem_filho = uso.ru_maxrss; // em KB (Linux)

            printf("FMS >> CPU utilizado pelo filho: %.3f s (usuário %.3f s, sistema %.3f s)\n",
                   cpu_filho,
                   uso.ru_utime.tv_sec + uso.ru_utime.tv_usec / 1e6,
                   uso.ru_stime.tv_sec + uso.ru_stime.tv_usec / 1e6);

            printf("FMS >> Memória máxima utilizada: %ld KB\n", mem_filho);

            // Verificação dos limites globais
            // Todas as execuções contam para a quota global acumulada
            cpu_acumulada += cpu_filho;

            printf("FMS >> CPU acumulado: %.3f / %.3f s\n",
                   cpu_acumulada, quota_cpu_total);

            if (cpu_acumulada > quota_cpu_total)
            {
                printf("FMS >> QUOTA GLOBAL DE CPU EXCEDIDA! Encerrando.\n");
                continuar = 0;
            }

            if (mem_filho > limite_mem_kb)
            {
                printf("FMS >> LIMITE GLOBAL DE MEMÓRIA EXCEDIDO (filho %d usou %ld KB). Encerrando.\n", pid, mem_filho);
                continuar = 0;
            }

            // Relata o resultado do filho
            // O timeout já possui mensagem própria
            if (!timeout_occurred)
            {
                if (WIFEXITED(status))
                    printf("FMS >> processo filho %d finalizou com codigo %d\n", pid, WEXITSTATUS(status));
                else if (WIFSIGNALED(status))
                    printf("FMS >> processo filho %d terminou pelo sinal %d\n", pid, WTERMSIG(status));
            }
        }
    }

    printf("FMS >> Sessão encerrada. CPU total consumido: %.3f s\n", cpu_acumulada);

    return 0;
}
