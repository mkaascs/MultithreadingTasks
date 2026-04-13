#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <cstring>
#include <pthread.h>
#include <time.h>

struct Partitions {
    int remaining;
    int max_part;
};

static int g_threads;
static long long g_result;
static int g_idle;
static int g_done;

static std::queue<Partitions> g_tasks;
static pthread_mutex_t g_mutex;
static pthread_cond_t g_new_work;
static pthread_cond_t g_finished;

static long long count_partitions(const int remaining, const int max_part) {
    if (remaining == 0)
        return 1;

    long long result = 0;
    const int lim = std::min(remaining, max_part);
    for (int partition = 1; partition <= lim; partition++) {
        result += count_partitions(remaining-partition,partition);
    }

    return result;
}

static void* worker(void*) {
    while (true) {
        pthread_mutex_lock(&g_mutex);

        while (g_tasks.size() == 0 && !g_done) {
            g_idle++;
            if (g_idle == g_threads)
                pthread_cond_signal(&g_finished);

            pthread_cond_wait(&g_new_work, &g_mutex);
            g_idle--;
        }

        if (g_tasks.size() == 0 && g_done) {
            pthread_mutex_unlock(&g_mutex);
            break;
        }

        Partitions part = g_tasks.front();
        g_tasks.pop();
        pthread_mutex_unlock(&g_mutex);

        const long long result = count_partitions(part.remaining, part.max_part);

        pthread_mutex_lock(&g_mutex);
        g_result += result;
        pthread_mutex_unlock(&g_mutex);
    }

    return nullptr;
}

int main() {
    FILE* file = fopen("input.txt", "r");
    if (file == nullptr) {
        printf("failed to open input.txt\n");
        return 1;
    }

    int T, N;
    fscanf(file, "%d\n%d", &T, &N);
    fclose(file);

    g_threads = T;
    g_idle = 0;
    g_result = 0;
    pthread_mutex_init(&g_mutex, nullptr);
    pthread_cond_init(&g_new_work, nullptr);
    pthread_cond_init(&g_finished, nullptr);

    pthread_t* threads = static_cast<pthread_t*>(malloc(T*sizeof(pthread_t)));

    for (int index = 0; index < T; index++)
        pthread_create(&threads[index], nullptr, worker, nullptr);

    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    pthread_mutex_lock(&g_mutex);
    for (int number = 1; number < N; number++)
        g_tasks.push(Partitions{N-number, number});

    pthread_cond_broadcast(&g_new_work);
    pthread_mutex_unlock(&g_mutex);

    pthread_mutex_lock(&g_mutex);
    while (g_idle != g_threads || g_tasks.size() != 0) {
        pthread_cond_wait(&g_finished, &g_mutex);
    }

    g_done = true;
    pthread_cond_broadcast(&g_new_work);
    pthread_mutex_unlock(&g_mutex);

    for (int index = 0; index < T; index++)
        pthread_join(threads[index], nullptr);

    free(threads);

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    const long long diff = static_cast<long long>(ts_end.tv_sec - ts_start.tv_sec)  * 1000
                 + static_cast<long long>(ts_end.tv_nsec - ts_start.tv_nsec) / 1000000;

    pthread_mutex_destroy(&g_mutex);
    pthread_cond_destroy(&g_finished);
    pthread_cond_destroy(&g_new_work);

    FILE* output = fopen("output.txt", "w");
    if (output == nullptr) {
        printf("failed to output result\n");
        return 1;
    }

    fprintf(output, "%d\n%d\n%lld", T, N, g_result);
    fclose(output);

    FILE* time_output = fopen("time.txt", "w");
    if (time_output == nullptr) {
        printf("failed to output time\n");
        return 1;
    }

    fprintf(time_output, "%lld", diff);
    fclose(time_output);

    return 0;
}
