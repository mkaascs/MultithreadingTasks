#include <windows.h>
#include <cstdio>
#include <cstdlib>

#define N 5
#define LEFT(i) (((i) + N - 1) % N)
#define RIGHT(i) (((i) + 1) % N)

enum State {
    THINKING = 1,
    HUNGRY,
    EATING
};

static State g_state[N];
static HANDLE g_event[N];
static HANDLE g_mutex;

static LONGLONG g_start;
static int g_total;
static int g_phil;

static int g_eat_count[N] = {0};
static int g_think_count[N] = {0};

static LONGLONG timenow_ms() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return counter.QuadPart * 1000LL / freq.QuadPart;
}

static void print_state(int index, char old_state, char new_state) {
    LONGLONG time = timenow_ms() - g_start;
    printf("%lld:%d:%c->%c\n", time, index + 1, old_state, new_state);
    fflush(stdout);
}

static void try_eat(int index) {
    if (g_state[index] == HUNGRY &&
        g_state[LEFT(index)] != EATING &&
        g_state[RIGHT(index)] != EATING) {

        g_state[index] = EATING;
        SetEvent(g_event[index]);
    }
}

static DWORD WINAPI philosopher(void* arg) {
    int index = *(int*)arg;

    while (true) {
        Sleep(g_phil);

        if (timenow_ms() - g_start >= g_total)
            break;

        WaitForSingleObject(g_mutex, INFINITE);
        g_state[index] = HUNGRY;
        try_eat(index);
        ReleaseMutex(g_mutex);

        LONGLONG remaining = (LONGLONG)g_total - (timenow_ms() - g_start);
        DWORD wait_ms = remaining > 0 ? (DWORD)remaining : 0;
        DWORD res = WaitForSingleObject(g_event[index], wait_ms);

        if (res == WAIT_TIMEOUT) {
            WaitForSingleObject(g_mutex, INFINITE);
            if (g_state[index] == HUNGRY) {
                g_state[index] = THINKING;
                try_eat(LEFT(index));
                try_eat(RIGHT(index));
            }
            ReleaseMutex(g_mutex);
            break;
        }

        if (timenow_ms() - g_start >= g_total) {
            WaitForSingleObject(g_mutex, INFINITE);
            g_state[index] = THINKING;
            try_eat(LEFT(index));
            try_eat(RIGHT(index));
            ReleaseMutex(g_mutex);
            break;
        }

        print_state(index, 'T', 'E');
        g_eat_count[index]++;

        Sleep(g_phil);

        WaitForSingleObject(g_mutex, INFINITE);
        g_state[index] = THINKING;
        print_state(index, 'E', 'T');
        g_think_count[index]++;

        try_eat(LEFT(index));
        try_eat(RIGHT(index));

        ReleaseMutex(g_mutex);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("usage: phil TOTAL PHIL\n");
        return 1;
    }

    g_total = atoi(argv[1]);
    g_phil = atoi(argv[2]);

    g_mutex = CreateMutex(nullptr, FALSE, nullptr);

    for (int index = 0; index < N; index++) {
        g_state[index] = THINKING;
        g_event[index] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }

    g_start = timenow_ms();

    HANDLE threads[N];
    int args[N];

    for (int index = 0; index < N; index++) {
        args[index] = index;
        threads[index] = CreateThread(nullptr, 0, philosopher, &args[index], 0, nullptr);
    }

    WaitForMultipleObjects(N, threads, TRUE, INFINITE);

    for (int index = 0; index < N; index++) {
        CloseHandle(threads[index]);
        CloseHandle(g_event[index]);
    }

    CloseHandle(g_mutex);

    printf("\nTOTAL\tPHIL");
    for (int index = 0; index < N; index++) {
        printf("\t%d(E)\t%d(T)", index+1, index+1);
    }

    printf("\n");

    printf("%d\t%d", g_total, g_phil);
    for (int index = 0; index < N; index++) {
        printf("\t%d\t%d", g_eat_count[index], g_think_count[index]);
    }

    printf("\n");

    return 0;
}