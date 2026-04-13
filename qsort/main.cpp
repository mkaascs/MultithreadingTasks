#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>

#define THRESHOLD 1000

struct Task {
    int* numbers;
    int left;
    int right;
};

static std::queue<Task*> g_tasks;
static CRITICAL_SECTION g_critical_section;
static HANDLE g_work;
static HANDLE g_done;
static volatile int g_active;

static void submit_task(int* numbers, int left, int right) {
    Task* task = static_cast<Task*>(malloc(sizeof(Task)));
    task->numbers = numbers;
    task->left = left;
    task->right = right;

    EnterCriticalSection(&g_critical_section);
    g_tasks.push(task);
    g_active++;
    LeaveCriticalSection(&g_critical_section);

    ReleaseSemaphore(g_work, 1, nullptr);
}

static void finish_task(Task* task) {
    EnterCriticalSection(&g_critical_section);
    g_active--;
    if (g_active == 0)
        ReleaseSemaphore(g_done, 1, nullptr);

    LeaveCriticalSection(&g_critical_section);
}

static void seq_sort(int* numbers, int left, int right);

static int partition(int* numbers, int left, int right) {
    int mid = left + (right - left) / 2;
    if (numbers[left] > numbers[mid]) {
        int t = numbers[left];   
        numbers[left] = numbers[mid];     
        numbers[mid] = t;
    }

    if (numbers[left] > numbers[right-1]) { 
        int t = numbers[left]; 
        numbers[left] = numbers[right-1];
        numbers[right-1] = t; 
    }

    if (numbers[mid] > numbers[right-1]) { 
        int t = numbers[mid];   
        numbers[mid] = numbers[right-1]; 
        numbers[right-1] = t; 
    }
 
    int pivot = numbers[mid];
    int tmp = numbers[mid];
    numbers[mid] = numbers[right-1]; 
    numbers[right-1] = tmp;
 
    int i = left, j = right - 2;
    while (true) {
        while (i <= j && numbers[i] < pivot) i++;
        while (j >= i && numbers[j] > pivot) j--;

        if (i >= j) 
            break;

        int t = numbers[i];
        numbers[i] = numbers[j]; 
        numbers[j] = t;

        i++; j--;
    }

    tmp = numbers[i]; 
    numbers[i] = numbers[right-1]; 
    numbers[right-1] = tmp;
    return i;
}

static void seq_sort(int* numbers, int left, int right) {
    if (right - left <= 1) return;
    if (right - left == 2) {
        if (numbers[left] > numbers[left+1]) { 
            int t = numbers[left]; 
            numbers[left] = numbers[left+1]; 
            numbers[left+1] = t; 
        }

        return;
    }

    int p = partition(numbers, left, right);
    seq_sort(numbers, left, p);
    seq_sort(numbers, p + 1, right);
}

static DWORD WINAPI worker(void*) {
    while (true) {
        WaitForSingleObject(g_work, INFINITE);

        EnterCriticalSection(&g_critical_section);
        if (g_tasks.empty()) {
            LeaveCriticalSection(&g_critical_section);
            break;
        }

        Task* task = g_tasks.front();
        g_tasks.pop();
        LeaveCriticalSection(&g_critical_section);

        if (task->numbers == nullptr) break;

        int length = task->right - task->left;
        if (length <= THRESHOLD) {
            seq_sort(task->numbers, task->left, task->right);
            finish_task(task);
            free(task);
            continue;
        }

        int pivot = partition(task->numbers, task->left, task->right);
        if (pivot - task->left > 1) 
            submit_task(task->numbers, task->left, pivot);
        
        if (task->right - (pivot+1) > 1)
            submit_task(task->numbers, pivot+1, task->right);

        finish_task(task);
        free(task);
    }

    return 0;
}

int main() {
    FILE* input = fopen("input.txt", "r");
    if (input == nullptr) {
        printf("failed to open input.txt");
        return 1;
    }

    int T, N;
    fscanf(input, "%d\n%d", &T, &N);
    
    int* numbers = static_cast<int*>(malloc(N*sizeof(int)));
    for (int index = 0; index < N; index++)
        fscanf(input, "%d ", &numbers[index]);

    fclose(input);

    InitializeCriticalSection(&g_critical_section);
    g_work = CreateSemaphore(nullptr, 0, N + T + 1, nullptr);
    g_done = CreateSemaphore(nullptr, 0, 1, nullptr);

    HANDLE* threads = static_cast<HANDLE*>(malloc(T*sizeof(HANDLE)));
    for (int index = 0; index < T; index++)
        threads[index] = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);

    DWORD start_time = GetTickCount();

    submit_task(numbers, 0, N);

    WaitForSingleObject(g_done, INFINITE);

    DWORD end_time = GetTickCount();

    ReleaseSemaphore(g_work, T, nullptr);

    WaitForMultipleObjects(T, threads, TRUE, INFINITE);
    for (int index = 0; index < T; index++)
        CloseHandle(threads[index]);
    
    free(threads);

    CloseHandle(g_work);
    CloseHandle(g_done);
    DeleteCriticalSection(&g_critical_section);

    FILE* output = fopen("output.txt", "w");
    fprintf(output, "%d\n%d\n", T, N);
    for (int index = 0; index < N; index++)
        fprintf(output, "%d ", numbers[index]);
    
    fclose(output);

    FILE* time = fopen("time.txt", "w");
    fprintf(time, "%lu\n", (unsigned long)(end_time - start_time));
    fclose(time);

    free(numbers);
    return 0;
}