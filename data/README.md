# SAT 2002 benchmark files used

Source: SAT 2002 Competition archive `sat-2002-beta.tgz` (147 MB), linked from
<https://satcompetition.github.io/> and hosted at
<https://www.cs.ubc.ca/~hoos/SATLIB/Benchmarks/SAT/New/Competition-02/sat-2002-beta.tgz>.
The archive and the `.cnf` files are **not** committed (`.gitignore`, SPEC §11). To
reproduce, extract these 8 files from the archive into this folder.

| # | File (path inside the archive) | Vars | Clauses |
|---|---|---:|---:|
| 1 | `submitted/biere/cmpadd/ca002.shuffled.cnf` | 26 | 70 |
| 2 | `generated/gen-1/gen-1.1/unif-c900-v450-s1604180397.cnf` | 450 | 900 |
| 3 | `submitted/pehoushek/graphcolors3/3col260_5_7.shuffled.cnf` | 520 | 2,216 |
| 4 | `submitted/goldberg/rand_net/rand_net70-30-1.shuffled.cnf` | 4,200 | 12,461 |
| 5 | `submitted/goldberg/fpga_routing/too_large_gr_rcs_w7.shuffled.cnf` | 3,633 | 50,373 |
| 6 | `submitted/dellacherie/ip/ip36.shuffled.cnf` | 47,273 | 153,368 |
| 7 | `submitted/dellacherie/fifo8/fifo8_200.shuffled.cnf` | 129,762 | 353,513 |
| 8 | `submitted/dellacherie/cache/cache_10.shuffled.cnf` | 227,210 | 879,754 |

## CLI run (TODO 3.2): `./logic_a1 --cnf data/<file> --shape left|balanced`

Heights are in edges (leaf = 0). "rec skipped" means the CLI refused to run the
recursive variants because height > `kRecursionLimit` (10 000, SPEC §5); it did not
crash. The height at which the recursive functions really crash is measured by
`make bench` in a `fork()`ed child (TODO 4.1 / 4.2). Peak RSS is for the whole CLI run.

| File | Shape | Nodes | Height | Recursive variants | Peak RSS |
|---|---|---:|---:|---|---:|
| ca002 | left | 441 | 72 | run | 7 MB |
| ca002 | balanced | 441 | 10 | run | 7 MB |
| unif-c900 | left | 6,746 | 902 | run | 7 MB |
| unif-c900 | balanced | 6,746 | 13 | run | 7 MB |
| 3col260 | left | 20,781 | 2,219 | run | 7 MB |
| 3col260 | balanced | 20,781 | 16 | run | 7 MB |
| rand_net70 | left | 73,359 | 12,462 | skipped | 19 MB |
| rand_net70 | balanced | 73,359 | 84 | run | 18 MB |
| too_large_gr_rcs_w7 | left | 259,022 | 50,374 | skipped | 54 MB |
| too_large_gr_rcs_w7 | balanced | 259,022 | 23 | run | 49 MB |
| ip36 | left | 915,528 | 153,372 | skipped | 187 MB |
| ip36 | balanced | 915,528 | 38 | run | 169 MB |
| fifo8_200 | left | 1,999,208 | 353,514 | skipped | 399 MB |
| fifo8_200 | balanced | 1,999,208 | 30 | run | 360 MB |
| cache_10 | left | 5,479,586 | 879,756 | skipped | 1,027 MB |
| cache_10 | balanced | 5,479,586 | 53 | run | 973 MB |

All 16 runs exit 0. Memory note: `cache_10` needs about 1 GB because the token
vector, the prefix vector and the tree each hold millions of std::string-bearing
entries; run the benchmark on one file at a time.
