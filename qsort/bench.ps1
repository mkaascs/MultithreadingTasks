$exe = ".\qsort.exe"
$output = ".\results.txt"

$threads = @(1, 2, 3, 4, 5, 8, 10)
$sizes   = @(1000000, 2000000, 3000000, 5000000, 7000000, 8000000, 9000000, 10000000, 20000000)

"Benchmark results" | Out-File $output -Encoding ascii

foreach ($n in $sizes) {
    "`nN=$n" | Out-File $output -Append -Encoding ascii

    # Генерируем массив один раз для всех T
    $rng  = [System.Random]::new(42)
    $nums = [int[]]::new($n)
    for ($i = 0; $i -lt $n; $i++) { $nums[$i] = $rng.Next() }
    $numsStr = $nums -join " "

    foreach ($t in $threads) {
        Write-Host "Running N=$n T=$t..."

        # ASCII без BOM — именно это читает fscanf
        [System.IO.File]::WriteAllText(".\input.txt",
            "$t`n$n`n$numsStr`n",
            [System.Text.Encoding]::ASCII)

        & $exe | Out-Null

        $time = (Get-Content ".\time.txt").Trim()
        "  T=$t  ${time}ms" | Out-File $output -Append -Encoding ascii
        Write-Host "  -> ${time}ms"
    }
}

Write-Host "Done! Results in $output"