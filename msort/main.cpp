#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <cstring>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

#define THRESHOLD 1000

struct Cont {
    sem_t mutex;
    int count;
    int* numbers;
    int left, middle, right;
    Cont* parent;
};

enum TaskType { TASK_SORT, TASK_MERGE, TASK_EXIT };

struct Task {
    TaskType type;
    int* numbers;
    int left, middle, right;
    Cont* cont;
};

static std::queue<Task*> g_queue;
static sem_t g_mutex;
static sem_t g_work;
static sem_t g_done;

static void submit(const TaskType type, int* numbers,
                   const int left, const int middle, const int right, Cont* cont) {
    Task* task = static_cast<Task*>(malloc(sizeof(Task)));
    task->type = type;
    task->numbers  = numbers;
    task->left = left;
    task->middle  = middle;
    task->right= right;
    task->cont = cont;

    sem_wait(&g_mutex);
    g_queue.push(task);
    sem_post(&g_mutex);

    sem_post(&g_work);
}

static void do_merge(int* numbers, const int left, const int middle, const int right) {
    const int length = right - left;
    int* temp = static_cast<int *>(malloc(length * sizeof(int)));

    int i = left, j = middle, k = 0;
    while (i < middle && j < right)
        temp[k++] = numbers[i] <= numbers[j]
            ? numbers[i++]
            : numbers[j++];

    while (i < middle)
        temp[k++] = numbers[i++];

    while (j < right)
        temp[k++] = numbers[j++];

    memcpy(numbers + left, temp, length * sizeof(int));
    free(temp);
}

static void seq_sort(int* numbers, const int left, const int right) {
    if (right - left <= 1) return;
    const int middle = left + (right - left) / 2;

    seq_sort(numbers, left,middle);
    seq_sort(numbers, middle, right);
    do_merge(numbers, left, middle, right);
}

static void notify_done(Cont* cont) {
    if (cont == nullptr) {
        sem_post(&g_done);
        return;
    }

    sem_wait(&cont->mutex);
    cont->count++;
    const int done = cont->count;
    sem_post(&cont->mutex);

    if (done < 2)
        return;

    Cont* parent = cont->parent;
    int* numbers = cont->numbers;
    const int left = cont->left;
    const int middle = cont->middle;
    const int right = cont->right;
    sem_destroy(&cont->mutex);
    free(cont);
    submit(TASK_MERGE, numbers, left, middle, right, parent);
}

static void* worker(void*){
    while (true) {
        sem_wait(&g_work);

        sem_wait(&g_mutex);
        Task* task = g_queue.front();
        g_queue.pop();
        sem_post(&g_mutex);

        if (task->type == TASK_EXIT) {
            free(task);
            break;
        }

        if (task->type == TASK_MERGE) {
            do_merge(task->numbers, task->left, task->middle, task->right);
            Cont* c = task->cont;
            free(task);
            notify_done(c);
            continue;
        }

        const int len = task->right - task->left;

        if (len <= THRESHOLD) {
            seq_sort(task->numbers, task->left, task->right);
            Cont* c = task->cont;
            free(task);
            notify_done(c);
            continue;
        }

        int middle = task->left + len / 2;

        Cont* cont = static_cast<Cont *>(malloc(sizeof(Cont)));
        sem_init(&cont->mutex, 0, 1);
        cont->count  = 0;
        cont->numbers = task->numbers;
        cont->left = task->left;
        cont->middle = middle;
        cont->right = task->right;
        cont->parent = task->cont;
        free(task);

        submit(TASK_SORT, cont->numbers, cont->left, 0, middle, cont);
        submit(TASK_SORT, cont->numbers, middle,0, cont->right, cont);
    }

    return nullptr;
}

int main() {
    FILE* fin = fopen("input.txt", "r");
    if (!fin) { printf("cannot open input.txt\n"); return 1; }

    int T, N;
    fscanf(fin, "%d %d", &T, &N);
    int* arr = static_cast<int *>(malloc(N * sizeof(int)));
    for (int i = 0; i < N; i++)
        fscanf(fin, "%d", &arr[i]);

    fclose(fin);

    sem_init(&g_mutex, 0, 1);
    sem_init(&g_work,  0, 0);
    sem_init(&g_done,  0, 0);

    pthread_t* tids = static_cast<pthread_t *>(malloc(T * sizeof(pthread_t)));
    for (int i = 0; i < T; i++)
        pthread_create(&tids[i], nullptr, worker, nullptr);

    timespec ts0, ts1;
    clock_gettime(CLOCK_MONOTONIC, &ts0);

    submit(TASK_SORT, arr, 0, 0, N, nullptr);
    sem_wait(&g_done);

    clock_gettime(CLOCK_MONOTONIC, &ts1);

    for (int i = 0; i < T; i++)
        submit(TASK_EXIT, nullptr, 0, 0, 0, nullptr);
    for (int i = 0; i < T; i++)
        pthread_join(tids[i], nullptr);

    free(tids);

    sem_destroy(&g_mutex);
    sem_destroy(&g_work);
    sem_destroy(&g_done);

    long long ms = (ts1.tv_sec  - ts0.tv_sec)  * 1000LL
                 + (ts1.tv_nsec - ts0.tv_nsec) / 1000000LL;

    FILE* fout = fopen("output.txt", "w");
    fprintf(fout, "%d\n%d\n", T, N);
    for (int i = 0; i < N; i++)
        fprintf(fout, "%d%c", arr[i], i+1<N ? ' ' : '\n');

    fclose(fout);

    FILE* ftm = fopen("time.txt", "w");
    fprintf(ftm, "%lld\n", ms);
    fclose(ftm);

    free(arr);
    return 0;
}