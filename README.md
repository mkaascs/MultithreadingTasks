# Multithreading Lab

Laboratory work on multithreading mechanisms. Covers thread pool patterns, synchronization primitives, and parallel algorithms across Linux (pthreads) and Windows (WinAPI).

---

## Tasks

### expr — Partition Counter (Linux)

Counts the number of unique ways to represent a natural number N as a sum of smaller natural numbers. Partitions that differ only in order are considered identical (e.g. `3 = 2+1` and `3 = 1+2` count as one).

Synchronization: mutex + condition variable.

Input `input.txt`:
```
<number of threads>
<N>
```

Output `output.txt`:
```
<number of threads>
<N>
<result>
```

---

### msort — Parallel Merge Sort (Linux)

Sorts an array of 32-bit signed integers in ascending order using a thread pool and parallel merge sort.

Synchronization: semaphores.

Input `input.txt`:
```
<number of threads>
<N>
<N space-separated integers>
```

Output `output.txt`:
```
<number of threads>
<N>
<sorted integers>
```

---

### qsort — Parallel Quick Sort (Windows)

Sorts an array of 32-bit signed integers in ascending order using a thread pool and parallel quicksort.

Synchronization: semaphore + critical section.

Input/output format is the same as msort.

---

### phil — Dining Philosophers (Windows)

Simulates five philosophers alternating between thinking and eating around a round table with five forks. The solution is deadlock-free and starvation-free, based on tracking philosopher states rather than fork states.

Synchronization: mutex + auto-reset events.

```
phil.exe TOTAL PHIL
```

- `TOTAL` — total runtime in milliseconds
- `PHIL` — minimum time a philosopher spends in any single state (thinking or eating), in milliseconds

Output format:
```
TIME:NUM:OLD->NEW
```

Example:
```
550:1:E->T
670:2:T->E
```

---

## Running

### Linux tasks (expr, msort)

Requires Docker and Docker Compose.

Place your `input.txt` in the task folder, then run:

```bash
cd expr
docker compose up --build
```

```bash
cd msort
docker compose up --build
```

Results will appear in `output.txt` and `time.txt` in the same folder.

### Windows tasks (qsort, phil)

Pre-built executables are in the `build/` folder of each task.

**qsort** — place `input.txt` next to the executable, then run:

```
qsort\build\qsort.exe
```

Results appear in `output.txt` and `time.txt`.

**phil** — run with two arguments:

```
phil\build\phil.exe 2000 100
```