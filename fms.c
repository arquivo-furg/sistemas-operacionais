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
const int MONITOR_INTERVAL = 1; // Intervalo do monitoramento em segundos

// Variáveis globais para comunicação entre a thread e o main
pthread_mutex_t mutex;
pthread_cond_t cond;

// Motivos para encerramento do processo filho
#define STOP_NONE 0
#define STOP_TIMEOUT 1
#define STOP_CPU_LIMIT 2
#define STOP_MEM_LIMIT 3
#define STOP_CHILD_EXIT 4

volatile int monitor_stop = STOP_NONE;
pid_t pid_filho = 0; // PID do filho para que a thread possa matá-lo

// Estrutura para passar informações à thread
struct monitor_info
{
    unsigned int timeout;
    double quota_cpu;
    long limite_mem;
};

// Lê o consumo de CPU do processo a partir de /proc/[pid]/stat
double get_process_cpu(pid_t pid)
{
    char caminho[64];

    snprintf(caminho, sizeof(caminho), "/proc/%d/stat", pid);

    FILE *fp = fopen(caminho, "r");

    if (!fp)
        return -1.0;

    long utime = 0;
    long stime = 0;

    int pid_read;
    char comm[256];
    char state;

    long ppid;
    long pgrp;
    long session;
    long tty_nr;
    long tpgid;

    unsigned long flags;
    unsigned long minflt;
    unsigned long cminflt;
    unsigned long majflt;
    unsigned long cmajflt;

    // Lê os campos necessários do arquivo stat
    fscanf(fp,
           "%d %s %c %ld %ld %ld %ld %ld %lu %lu %lu %lu %lu %lu %ld %ld",
           &pid_read,
           comm,
           &state,
           &ppid,
           &pgrp,
           &session,
           &tty_nr,
           &tpgid,
           &flags,
           &minflt,
           &cminflt,
           &majflt,
           &cmajflt,
           &utime,
           &stime);

    fclose(fp);

    // Obtém quantos ticks existem por segundo
    long ticks_per_sec = sysconf(_SC_CLK_TCK);

    if (ticks_per_sec <= 0)
        ticks_per_sec = 100;

    // Retorna CPU total em segundos
    return (double)(utime + stime) / ticks_per_sec;
}

// Lê o consumo de memória do processo a partir de /proc/[pid]/status
long get_process_mem(pid_t pid)
{
    char caminho[64];

    snprintf(caminho, sizeof(caminho), "/proc/%d/status", pid);

    FILE *fp = fopen(caminho, "r");

    if (!fp)
        return -1;

    char linha[256];
    long vmrss = -1;

    // Procura pela linha VmRSS
    while (fgets(linha, sizeof(linha), fp) != NULL)
    {
        if (sscanf(linha, "VmRSS: %ld", &vmrss) == 1)
            break;
    }

    fclose(fp);

    return vmrss;
}

// Thread responsável pelo monitoramento
void *thread_monitor(void *arg)
{
    struct monitor_info *info = (struct monitor_info *)arg;

    unsigned int timeout = info->timeout;
    double quota_cpu = info->quota_cpu;
    long limite_mem = info->limite_mem;

    time_t start_time = time(NULL);

    while (1)
    {
        // Espera 1 segundo entre cada monitoramento
        sleep(MONITOR_INTERVAL);

        pthread_mutex_lock(&mutex);

        // Encerra se o processo já terminou
        if (monitor_stop != STOP_NONE)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Verifica timeout
        if (timeout > 0)
        {
            time_t agora = time(NULL);

            if (agora - start_time >= timeout)
            {
                monitor_stop = STOP_TIMEOUT;

                // Mata o processo filho
                if (pid_filho > 0)
                    kill(pid_filho, SIGKILL);

                pthread_cond_signal(&cond);

                pthread_mutex_unlock(&mutex);

                break;
            }
        }

        // Verifica CPU e memória do processo
        if (pid_filho > 0)
        {
            double cpu = get_process_cpu(pid_filho);
            long mem = get_process_mem(pid_filho);

            // Exibe relatório do monitor
            printf("FMS >> [Monitor] CPU: %.3f s / %.3f s\n",
                   cpu,
                   quota_cpu);

            printf("FMS >> [Monitor] Memória: %ld KB / %ld KB\n",
                   mem,
                   limite_mem);

            // Verifica limite de CPU
            if (cpu >= 0.0 && cpu > quota_cpu)
            {
                monitor_stop = STOP_CPU_LIMIT;

                kill(pid_filho, SIGKILL);

                pthread_cond_signal(&cond);

                pthread_mutex_unlock(&mutex);

                break;
            }

            // Verifica limite de memória
            if (mem >= 0 && mem > limite_mem)
            {
                monitor_stop = STOP_MEM_LIMIT;

                kill(pid_filho, SIGKILL);

                pthread_cond_signal(&cond);

                pthread_mutex_unlock(&mutex);

                break;
            }
        }

        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    // Quotas globais válidas para toda a sessão do FMS
    double quota_cpu_total;
    long limite_mem_kb;

    printf("FMS >> Bem-vindo ao Fork Monitor & Schedule\n");

    // Lê quota global de CPU
    printf("FMS >> Informe a quota global de tempo de CPU (em segundos): ");
    scanf("%lf", &quota_cpu_total);

    // Lê limite global de memória
    printf("FMS >> Informe o limite global de memoria (em KB): ");
    scanf("%ld", &limite_mem_kb);

    double cpu_acumulada = 0.0;
    int continuar = 1;

    // Laço principal
    while (continuar && cpu_acumulada < quota_cpu_total)
    {
        // Solicita o nome do binário
        char nome[N];

        printf("\nFMS >> Introduza o caminho do binario a executar (ou 'sair'): ");
        scanf("%63s", nome);

        // Encerra o FMS caso o usuário digite "sair"
        if (strcmp(nome, "sair") == 0)
            break;

        // Quota de CPU deste binário
        double quota_binario = quota_cpu_total;

        // Timeout
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
            execve(nome, argv, 0);

            // Se execve retornar, houve erro
            printf("Erro ao executar o binario '%s'\n", nome);

            return 1;
        }
        else // Processo pai (FMS)
        {
            printf("FMS >> processo filho criado com PID %d\n", pid);

            // Guarda PID do filho para uso na thread
            pid_filho = pid;

            // Estrutura para recolher estatísticas do filho
            struct rusage uso;

            int status;
            int filho_terminou = 0;

            // Inicializa mutex e condição
            pthread_mutex_init(&mutex, NULL);
            pthread_cond_init(&cond, NULL);

            // Configura a thread de monitoramento
            pthread_t tid;

            struct monitor_info minfo;

            minfo.timeout = timeout;
            minfo.quota_cpu = quota_binario;
            minfo.limite_mem = limite_mem_kb;

            // Reinicia o estado do monitor
            monitor_stop = STOP_NONE;

            // Cria a thread de monitoramento
            pthread_create(&tid, NULL, thread_monitor, (void *)&minfo);

            // Espera até que o filho termine ou algum limite seja atingido
            while (1)
            {
                // Verifica se o filho terminou
                int w = wait4(pid, &status, WNOHANG, &uso);

                if (w == pid)
                {
                    filho_terminou = 1;

                    pthread_mutex_lock(&mutex);

                    // Informa à thread que o processo terminou
                    monitor_stop = STOP_CHILD_EXIT;

                    pthread_cond_signal(&cond);

                    pthread_mutex_unlock(&mutex);

                    break;
                }

                pthread_mutex_lock(&mutex);

                // Sai se a thread já sinalizou algum limite
                if (monitor_stop != STOP_NONE)
                {
                    pthread_mutex_unlock(&mutex);
                    break;
                }

                // Aguarda um curto período para evitar busy wait
                struct timespec ts;

                clock_gettime(CLOCK_REALTIME, &ts);

                ts.tv_nsec += 100000000;

                if (ts.tv_nsec >= 1000000000)
                {
                    ts.tv_nsec -= 1000000000;
                    ts.tv_sec += 1;
                }

                pthread_cond_timedwait(&cond, &mutex, &ts);

                pthread_mutex_unlock(&mutex);
            }

            // Cancela a thread caso ainda esteja ativa
            pthread_cancel(tid);

            // Espera a thread encerrar
            pthread_join(tid, NULL);

            // Se o filho ainda estiver rodando recolhe o status final
            if (!filho_terminou)
            {
                kill(pid, SIGKILL);

                wait4(pid, &status, 0, &uso);
            }

            pthread_mutex_destroy(&mutex);
            pthread_cond_destroy(&cond);

            // Calcula CPU total utilizada
            double cpu_filho =
                uso.ru_utime.tv_sec + uso.ru_utime.tv_usec / 1e6 +
                uso.ru_stime.tv_sec + uso.ru_stime.tv_usec / 1e6;

            // Memória máxima utilizada
            long mem_filho = uso.ru_maxrss;

            // Exibe estatísticas finais
            printf("FMS >> CPU utilizado pelo filho: %.3f s (usuário %.3f s, sistema %.3f s)\n",
                   cpu_filho,
                   uso.ru_utime.tv_sec + uso.ru_utime.tv_usec / 1e6,
                   uso.ru_stime.tv_sec + uso.ru_stime.tv_usec / 1e6);

            printf("FMS >> Memória máxima utilizada: %ld KB\n", mem_filho);

            // Verifica falha no execve
            int falha_lancamento = 0;

            if (WIFEXITED(status) && WEXITSTATUS(status) == 1)
                falha_lancamento = 1;

            // Atualiza quota global
            if (!falha_lancamento)
            {
                cpu_acumulada += cpu_filho;

                printf("FMS >> CPU acumulado: %.3f / %.3f s\n",
                       cpu_acumulada,
                       quota_cpu_total);
            }
            else
            {
                printf("FMS >> Falha ao lançar o binário. Quota não descontada.\n");
            }

            // Verificação dos limites globais
            if (cpu_acumulada > quota_cpu_total)
            {
                printf("FMS >> QUOTA GLOBAL DE CPU EXCEDIDA! Encerrando.\n");
                continuar = 0;
            }

            if (mem_filho > limite_mem_kb)
            {
                printf("FMS >> LIMITE GLOBAL DE MEMÓRIA EXCEDIDO (filho %d usou %ld KB).\n", pid, mem_filho);

                continuar = 0;
            }

            // Relata o motivo do encerramento
            if (monitor_stop == STOP_TIMEOUT)
            {
                printf("FMS >> TEMPO ESGOTADO! Processo filho %d foi morto.\n", pid);
            }
            else if (monitor_stop == STOP_CPU_LIMIT)
            {
                printf("FMS >> QUOTA DE CPU EXCEDIDA! Processo filho %d foi morto.\n", pid);
            }
            else if (monitor_stop == STOP_MEM_LIMIT)
            {
                printf("FMS >> LIMITE DE MEMÓRIA EXCEDIDO! Processo filho %d foi morto.\n", pid);
            }
            else
            {
                if (WIFEXITED(status))
                    printf("FMS >> processo filho %d finalizou com codigo %d\n", pid, WEXITSTATUS(status));
                else if (WIFSIGNALED(status))
                    printf("FMS >> processo filho %d terminou pelo sinal %d\n", pid, WTERMSIG(status));
            }
        }
    }

    printf("FMS >> Sessão encerrada. CPU total consumido: %.3f s\n",
           cpu_acumulada);

    return 0;
}
