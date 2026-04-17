package main

import (
	"fmt"
	"log"
	"math/rand"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"time"
)

const maxGoroutines = 3

type Bench struct {
	Name        string
	DockerImage string
	Path        string
	Threads     []int
	Inputs      []Input
}

type Input struct {
	Label string
	Data  string
}

type Result struct {
	BenchName string
	Label     string
	Threads   int
	Ms        int64
	Err       string
}

func exprInput(threads, n int) string {
	return fmt.Sprintf("%d\n%d\n", threads, n)
}

func sortInput(threads, n int) string {
	nums := make([]string, n)
	r := rand.New(rand.NewSource(42))
	for i := range nums {
		nums[i] = strconv.Itoa(int(r.Int31()))
	}

	return fmt.Sprintf("%d\n%d\n%s\n", threads, n, strings.Join(nums, " "))
}

func runOnceDocker(imageName string, binaryPath string, inputData string) (int64, error) {
	dir, err := os.MkdirTemp("", "bench-*")
	if err != nil {
		return 0, err
	}

	defer func(path string) {
		if err := os.RemoveAll(path); err != nil {
			log.Printf("error removing temporary directory: %v", err)
		}
	}(dir)

	absDir, _ := filepath.Abs(dir)

	if err := os.WriteFile(filepath.Join(dir, "input.txt"), []byte(inputData), 0644); err != nil {
		return 0, err
	}

	_ = os.WriteFile(filepath.Join(dir, "output.txt"), nil, 0644)
	_ = os.WriteFile(filepath.Join(dir, "time.txt"), nil, 0644)

	args := []string{
		"run", "--rm",
		"-v", fmt.Sprintf("%s:/work", absDir),
		"-w", "/work",
		imageName,
		binaryPath,
	}

	cmd := exec.Command("docker", args...)

	out, err := cmd.CombinedOutput()
	if err != nil {
		log.Println(string(out))
		return 0, fmt.Errorf("docker failed: %v\n%s", err, string(out))
	}

	timeBytes, err := os.ReadFile(filepath.Join(dir, "time.txt"))
	if err != nil {
		return 0, err
	}

	return strconv.ParseInt(strings.TrimSpace(string(timeBytes)), 10, 64)
}

func runBench(b Bench, results chan<- Result, wg *sync.WaitGroup) {
	defer wg.Done()

	sem := make(chan struct{}, maxGoroutines)

	var innerWg sync.WaitGroup
	for _, inp := range b.Inputs {
		for _, t := range b.Threads {
			inp := inp
			t := t
			innerWg.Add(1)
			sem <- struct{}{}
			go func() {
				defer innerWg.Done()
				defer func() { <-sem }()

				data := inp.Data
				lines := strings.SplitN(data, "\n", 2)
				if len(lines) >= 2 {
					data = strconv.Itoa(t) + "\n" + lines[1]
				}

				ms, err := runOnceDocker(b.DockerImage, b.Path, data)

				r := Result{BenchName: b.Name, Label: inp.Label, Threads: t, Ms: ms}
				if err != nil {
					r.Err = err.Error()
				}

				results <- r
				fmt.Printf("  [%s] %s T=%d → %dms\n", b.Name, inp.Label, t, ms)
			}()
		}
	}
	innerWg.Wait()
}

func printTable(name string, inputs []Input, threads []int, results []Result) string {
	type key struct {
		label string
		t     int
	}

	m := map[key]string{}
	for _, r := range results {
		if r.BenchName != name {
			continue
		}

		v := strconv.FormatInt(r.Ms, 10)
		if r.Err != "" {
			v = "ERR"
		}

		m[key{r.Label, r.Threads}] = v
	}

	var sb strings.Builder
	sb.WriteString(fmt.Sprintf("\n=== %s ===\n", strings.ToUpper(name)))

	sb.WriteString(fmt.Sprintf("%-20s", "Input"))
	for _, t := range threads {
		sb.WriteString(fmt.Sprintf(" %8s", fmt.Sprintf("T=%d", t)))
	}

	sb.WriteString("\n")

	sb.WriteString(strings.Repeat("-", 20+len(threads)*9) + "\n")

	// Строки
	for _, inp := range inputs {
		sb.WriteString(fmt.Sprintf("%-20s", inp.Label))
		for _, t := range threads {
			v := m[key{inp.Label, t}]
			if v == "" {
				v = "—"
			}
			sb.WriteString(fmt.Sprintf(" %8s", v))
		}

		sb.WriteString("\n")
	}

	return sb.String()
}

func main() {
	threads := []int{1, 2, 3, 4, 5, 8, 10}

	exprNs := []int{95, 100, 105, 110, 115, 120, 125, 130}
	exprInputs := make([]Input, len(exprNs))
	for i, n := range exprNs {
		exprInputs[i] = Input{
			Label: fmt.Sprintf("N=%d", n),
			Data:  exprInput(1, n),
		}
	}

	sortNs := []int{1_000_000, 2_000_000, 3_000_000, 5_000_000, 7_000_000, 8_000_000, 9_000_000, 10_000_000, 20_000_000, 30_000_000}

	sortInputs := make([]Input, len(sortNs))
	for i, n := range sortNs {
		label := fmt.Sprintf("N=%s", formatN(n))
		sortInputs[i] = Input{
			Label: label,
			Data:  sortInput(1, n),
		}
	}

	benches := []Bench{
		/*
			{
				Name:        "expr",
				DockerImage: "expr-image",
				Path:        "/app/expr",
				Threads:     threads,
				Inputs:      exprInputs,
			},
		*/
		{
			Name:        "msort",
			DockerImage: "msort-image",
			Path:        "/app/msort",
			Threads:     threads,
			Inputs:      sortInputs,
		},
	}

	fmt.Println("Starting benchmarks...")
	start := time.Now()

	results := make(chan Result, 1000)
	var wg sync.WaitGroup

	for _, b := range benches {
		wg.Add(1)
		go runBench(b, results, &wg)
	}

	go func() {
		wg.Wait()
		close(results)
	}()

	var all []Result
	for r := range results {
		all = append(all, r)
	}

	fmt.Printf("\nDone in %s\n", time.Since(start).Round(time.Second))

	var report strings.Builder
	report.WriteString(fmt.Sprintf("Benchmark results — %s\n", time.Now().Format("2006-01-02 15:04:05")))
	report.WriteString("Time in milliseconds\n")

	for _, b := range benches {
		report.WriteString(printTable(b.Name, b.Inputs, threads, all))
	}

	output := report.String()
	fmt.Println(output)

	outFile := "bench_results.txt"
	if err := os.WriteFile(outFile, []byte(output), 0644); err != nil {
		_, _ = fmt.Fprintf(os.Stderr, "Failed to write results: %v\n", err)
	} else {
		fmt.Printf("Results saved to %s\n", outFile)
	}
}

func formatN(n int) string {
	s := strconv.Itoa(n)
	var result []byte
	for i, c := range s {
		if i > 0 && (len(s)-i)%3 == 0 {
			result = append(result, '_')
		}
		result = append(result, byte(c))
	}
	return string(result)
}
