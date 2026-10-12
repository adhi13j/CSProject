/**
 * @file bench.cpp
 * @brief Benchmark driver for SPEC section 8: time, auxiliary peak, RSS and survival per function.
 *
 * Writes `bench/results.csv` with the columns
 * `dataset,shape,nodes,height,atoms,function,median_ms,aux_peak,rss_kb,status`.
 *
 * - median_ms: median of 5 runs, std::chrono::steady_clock. `NA` when not measured.
 * - aux_peak: largest stack / queue length (iterative code) or recursion depth (recursive code),
 *             counted by src/instrument.h in the -DBENCH_INSTRUMENT build this program links.
 * - rss_kb:   ru_maxrss. In-process rows: peak of the whole benchmark process so far (it only grows).
 *             Forked rows: peak of that child.
 * - status:   OK, CRASH (killed by a signal, e.g. SIGSEGV from stack overflow) or SKIPPED.
 *
 * Recursive functions run in a fork()ed child so that a stack overflow cannot kill the benchmark.
 *
 * Usage: bench [results.csv] [--quick]   (--quick skips the 1M-node, large-SAT and k=20 runs)
 *
 * @authors Replace with group member names.
 */
#include "formula.h"
#include "../src/instrument.h"
#include "gen.h"

#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <random>

namespace {

constexpr int kRuns = 5;                    ///< repetitions per measurement (median is reported)
volatile std::size_t g_sink = 0;            ///< consumes results so the optimiser cannot delete the work

/** @brief One CSV row; dataset-level fields are filled in once the tree has been built. */
struct Row {
    std::string function;
    double medianMs = -1;      ///< negative means NA
    long long aux = -1;        ///< negative means NA
    long rssKb = 0;
    std::string status = "OK";
};

/** @brief Result of timing one function. */
struct Stats {
    double medianMs;
    std::size_t aux;
};

/** @brief Dataset-level fields shared by all rows of a dataset. */
struct Meta {
    std::string dataset;
    std::string shape;
    long long nodes = -1;
    long long height = -1;
    long long atoms = -1;
};

/**
 * @brief Current process peak RSS in kilobytes (Linux: ru_maxrss is in kB).
 * @return Peak resident set size.
 * @throws Nothing.
 * @note Complexity: O(1).
 */
long selfRssKb() {
    rusage ru{};
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_maxrss;
}

/**
 * @brief Time `work` kRuns times, running `prepare` before and `cleanup` after each run (both untimed).
 * @param prepare Untimed setup for one run.
 * @param work    The function under test.
 * @param cleanup Untimed teardown for one run.
 * @return Median wall time in milliseconds and the largest aux counter seen in any run.
 * @throws Whatever `work` throws.
 * @note Complexity: kRuns * cost(work).
 */
Stats measure(const std::function<void()>& prepare, const std::function<void()>& work,
              const std::function<void()>& cleanup) {
    std::vector<double> ms;
    std::size_t aux = 0;
    for (int r = 0; r < kRuns; ++r) {
        prepare();
#ifdef BENCH_INSTRUMENT
        bench::auxReset();
#endif
        const auto t0 = std::chrono::steady_clock::now();
        work();
        const auto t1 = std::chrono::steady_clock::now();
#ifdef BENCH_INSTRUMENT
        aux = std::max(aux, bench::auxPeak);
#endif
        cleanup();
        ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    std::sort(ms.begin(), ms.end());
    return {ms[ms.size() / 2], aux};
}

/** @brief measure() with no prepare and no cleanup. */
Stats measure(const std::function<void()>& work) {
    return measure([] {}, work, [] {});
}

/**
 * @brief Run measure() in a fork()ed child and report how it ended.
 * @param work The function under test.
 * @return A row: OK with the child's numbers, or CRASH if the child died from a signal.
 * @throws Nothing.
 * @note Complexity: that of measure() plus one fork.
 */
Row measureForked(const std::string& name, const std::function<void()>& work) {
    Row row;
    row.function = name;
    int fd[2];
    if (pipe(fd) != 0) {
        row.status = "SKIPPED";
        return row;
    }
    std::fflush(nullptr);
    const pid_t pid = fork();
    if (pid == 0) {
        close(fd[0]);
        Stats s{};
        try {
            s = measure(work);
        } catch (...) {
            _exit(2);
        }
        const ssize_t w = write(fd[1], &s, sizeof s);
        _exit(w == static_cast<ssize_t>(sizeof s) ? 0 : 3);
    }
    close(fd[1]);
    Stats s{};
    std::size_t got = 0;
    while (got < sizeof s) {
        const ssize_t r = read(fd[0], reinterpret_cast<char*>(&s) + got, sizeof s - got);
        if (r <= 0) {
            break;
        }
        got += static_cast<std::size_t>(r);
    }
    close(fd[0]);
    int status = 0;
    rusage ru{};
    wait4(pid, &status, 0, &ru);
    row.rssKb = ru.ru_maxrss;
    if (WIFSIGNALED(status) || !(WIFEXITED(status) && WEXITSTATUS(status) == 0) || got != sizeof s) {
        row.status = "CRASH";
        if (WIFSIGNALED(status)) {
            std::cerr << "  " << name << ": child killed by signal " << WTERMSIG(status) << "\n";
        }
    } else {
        row.medianMs = s.medianMs;
        row.aux = static_cast<long long>(s.aux);
    }
    return row;
}

/** @brief Measure in-process and make a row. */
Row measureRow(const std::string& name, const std::function<void()>& work) {
    Row row;
    row.function = name;
    const Stats s = measure(work);
    row.medianMs = s.medianMs;
    row.aux = static_cast<long long>(s.aux);
    row.rssKb = selfRssKb();
    return row;
}

/** @brief A row for something that was deliberately not run. */
Row skippedRow(const std::string& name) {
    Row row;
    row.function = name;
    row.status = "SKIPPED";
    row.rssKb = selfRssKb();
    return row;
}

/** @brief CSV output; each row is flushed so a long run can be watched with `tail -f`. */
class Csv {
public:
    /** @brief Open `path` and write the header. @throws std::runtime_error if it cannot be opened. */
    explicit Csv(const std::string& path) : out_(std::fopen(path.c_str(), "w")) {
        if (out_ == nullptr) {
            throw std::runtime_error("cannot write " + path);
        }
        std::fprintf(out_, "dataset,shape,nodes,height,atoms,function,median_ms,aux_peak,rss_kb,status\n");
    }
    Csv(const Csv&) = delete;
    Csv& operator=(const Csv&) = delete;
    ~Csv() {
        if (out_ != nullptr) {
            std::fclose(out_);
        }
    }

    /** @brief Write all rows of one dataset. @param m Dataset fields. @param rows The rows. */
    void write(const Meta& m, const std::vector<Row>& rows) {
        for (const Row& r : rows) {
            std::fprintf(out_, "%s,%s,", m.dataset.c_str(), m.shape.c_str());
            printNum(m.nodes);
            printNum(m.height);
            printNum(m.atoms);
            std::fprintf(out_, "%s,", r.function.c_str());
            if (r.medianMs < 0) {
                std::fprintf(out_, "NA,");
            } else {
                std::fprintf(out_, "%.6f,", r.medianMs);
            }
            printNum(r.aux);
            std::fprintf(out_, "%ld,%s\n", r.rssKb, r.status.c_str());
        }
        std::fflush(out_);
    }

private:
    /** @brief Print an integer field followed by a comma; negative prints `NA`. */
    void printNum(long long v) {
        if (v < 0) {
            std::fprintf(out_, "NA,");
        } else {
            std::fprintf(out_, "%lld,", v);
        }
    }
    std::FILE* out_;
};

/**
 * @brief Count the nodes of a tree iteratively.
 * @param root Tree root.
 * @return Node count.
 * @throws Nothing.
 * @note Complexity: O(n) time / O(h) space.
 */
long long countNodes(const Node* root) {
    long long c = 0;
    std::vector<const Node*> st;
    if (root != nullptr) {
        st.push_back(root);
    }
    while (!st.empty()) {
        const Node* n = st.back();
        st.pop_back();
        ++c;
        if (n->left != nullptr) {
            st.push_back(n->left);
        }
        if (n->right != nullptr) {
            st.push_back(n->right);
        }
    }
    return c;
}

/**
 * @brief Benchmark everything from tokenizing `infix` to evaluating the tree.
 * @param csv         Output.
 * @param meta        Dataset name and shape (nodes/height/atoms are filled in here).
 * @param infix       The formula text; consumed (freed early to limit peak memory).
 * @param extraRows   Rows measured before the pipeline (e.g. readDimacs), already complete.
 * @param truthOnly   If true, only the truth-table and evaluation rows are produced.
 * @param rng         Random engine for the evaluation assignment.
 * @throws std::exception on a pipeline error (which would be a bug).
 * @note Complexity: kRuns times the cost of every function on this dataset.
 */
void benchPipeline(Csv& csv, Meta meta, std::string infix, std::vector<Row> rows, bool truthOnly,
                   std::mt19937_64& rng) {
    std::cerr << "dataset " << meta.dataset << " (" << meta.shape << ") ...\n";
    std::vector<std::string> tokens;
    std::vector<std::string> prefix;
    if (!truthOnly) {
        rows.push_back(measureRow("tokenize", [&] { g_sink += tokenize(infix).size(); }));
    }
    tokens = tokenize(infix);
    std::string().swap(infix);

    if (!truthOnly) {
        rows.push_back(measureRow("infixToPrefix", [&] { g_sink += infixToPrefix(tokens).size(); }));
    }
    prefix = infixToPrefix(tokens);
    std::vector<std::string>().swap(tokens);

    Node* last = nullptr;
    if (!truthOnly) {
        Row build;
        build.function = "prefixToTree";
        const Stats bs = measure([] {}, [&] { last = prefixToTree(prefix); }, [&] { freeTree(last); last = nullptr; });
        build.medianMs = bs.medianMs;
        build.aux = static_cast<long long>(bs.aux);
        build.rssKb = selfRssKb();
        rows.push_back(build);
    }
    Node* root = prefixToTree(prefix);

    meta.nodes = countNodes(root);
    meta.height = heightIter(root);
    const std::vector<std::string> atoms = collectAtoms(root);
    meta.atoms = static_cast<long long>(atoms.size());
    Assignment assign;
    for (const std::string& a : atoms) {
        assign[a] = (rng() & 1u) != 0;
    }

    if (!truthOnly) {
        rows.push_back(measureForked("inorderRec", [&] { g_sink += inorderRec(root).size(); }));
        rows.push_back(measureRow("inorderIter", [&] { g_sink += inorderIter(root).size(); }));
        rows.push_back(measureForked("preorderRec", [&] { g_sink += preorderRec(root).size(); }));
        rows.push_back(measureRow("preorderIter", [&] { g_sink += preorderIter(root).size(); }));
        rows.push_back(measureForked("postorderRec", [&] { g_sink += postorderRec(root).size(); }));
        rows.push_back(measureRow("postorderIter", [&] { g_sink += postorderIter(root).size(); }));
        rows.push_back(measureForked("heightRec", [&] { g_sink += static_cast<std::size_t>(heightRec(root) + 1); }));
        rows.push_back(measureRow("heightIter", [&] { g_sink += static_cast<std::size_t>(heightIter(root) + 1); }));
        rows.push_back(measureRow("collectAtoms", [&] { g_sink += collectAtoms(root).size(); }));
    }
    rows.push_back(measureForked("evaluateRec", [&] { g_sink += evaluateRec(root, assign) ? 1 : 0; }));
    rows.push_back(measureRow("evaluateIter", [&] { g_sink += evaluateIter(root, assign) ? 1 : 0; }));
    if (meta.atoms <= 20) {
        rows.push_back(measureRow("truthTableColumn", [&] { g_sink += truthTableColumn(root).size(); }));
    } else {
        rows.push_back(skippedRow("truthTableColumn"));   // 2^k rows: infeasible (SPEC 8.4)
    }
    freeTree(root);
    if (!truthOnly) {
        Node* t = nullptr;
        // freeTree timing: build outside the timer, free inside it.
        Row fr;
        fr.function = "freeTree";
        const Stats s = measure([&] { t = prefixToTree(prefix); }, [&] { freeTree(t); }, [] {});
        fr.medianMs = s.medianMs;
        fr.aux = static_cast<long long>(s.aux);
        fr.rssKb = selfRssKb();
        rows.push_back(fr);
    }
    csv.write(meta, rows);
}

/**
 * @brief Read the `input:` line of a golden file.
 * @param path Golden file.
 * @return The formula text.
 * @throws std::runtime_error if there is no `input:` line.
 * @note Complexity: O(file size).
 */
std::string goldenInput(const std::string& path) {
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("input: ", 0) == 0) {
            return line.substr(7);
        }
    }
    throw std::runtime_error("no input line in " + path);
}

/** @brief Benchmark one SAT file in both tree shapes. */
void benchSat(Csv& csv, const std::string& path, std::mt19937_64& rng) {
    const std::string name = std::filesystem::path(path).stem().string();
    std::cerr << "reading " << path << " ...\n";
    CNF cnf;
    Row read = measureRow("readDimacs", [&] { g_sink += readDimacs(path).clauses.size(); });
    cnf = readDimacs(path);
    {
        Meta m;
        m.dataset = "sat:" + name;
        m.shape = "FILE";
        m.nodes = cnf.numClauses;     // for the file row: clauses
        m.atoms = cnf.numVars;        // and variables
        csv.write(m, {read});
    }
    for (TreeShape shape : {TreeShape::LEFT_DEEP, TreeShape::BALANCED}) {
        const char* shapeName = shape == TreeShape::LEFT_DEEP ? "LEFT_DEEP" : "BALANCED";
        std::vector<Row> rows;
        rows.push_back(measureRow("cnfToInfix", [&] { g_sink += cnfToInfix(cnf, shape).size(); }));
        Meta m;
        m.dataset = "sat:" + name;
        m.shape = shapeName;
        benchPipeline(csv, m, cnfToInfix(cnf, shape), rows, false, rng);
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string csvPath = "bench/results.csv";
    bool quick = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--quick") == 0) {
            quick = true;
        } else {
            csvPath = argv[i];
        }
    }
    try {
        Csv csv(csvPath);
        std::mt19937_64 rng(20261012);

        // 1. Golden formulas (Appendix A).
        std::vector<std::string> goldens;
        for (const auto& e : std::filesystem::directory_iterator("tests/golden")) {
            if (e.path().extension() == ".txt") {
                goldens.push_back(e.path().string());
            }
        }
        std::sort(goldens.begin(), goldens.end());
        for (const std::string& g : goldens) {
            Meta m;
            m.dataset = "golden:" + std::filesystem::path(g).stem().string();
            m.shape = "GOLDEN";
            benchPipeline(csv, m, goldenInput(g), {}, false, rng);
        }

        // 2. Synthetic scaling: random formulas and left-deep chains.
        std::vector<long> sizes = {10, 100, 1000, 10000, 100000, 1000000};
        if (quick) {
            sizes.pop_back();
        }
        for (long n : sizes) {
            Meta r;
            r.dataset = "random-" + std::to_string(n);
            r.shape = "RANDOM";
            benchPipeline(csv, r, randomFormula(rng, n, static_cast<int>(std::max(3L, n / 20))), {}, false, rng);
            Meta c;
            c.dataset = "chain-" + std::to_string(n);
            c.shape = "LEFT_DEEP";
            benchPipeline(csv, c, leftDeepChain(std::max(1L, n / 2), '+'), {}, false, rng);
        }

        // 3. Truth-table growth: same size, increasing atom count.
        for (int k : {4, 8, 12, 16, 20}) {
            if (quick && k == 20) {
                continue;
            }
            Meta m;
            m.dataset = "tt-k" + std::to_string(k);
            m.shape = "RANDOM";
            benchPipeline(csv, m, randomFormula(rng, 100, k), {}, true, rng);
        }

        // 4. SAT 2002 files, if present (they are not shipped in the submission).
        std::vector<std::string> sat;
        if (std::filesystem::is_directory("data")) {
            for (const auto& e : std::filesystem::directory_iterator("data")) {
                if (e.path().extension() == ".cnf") {
                    sat.push_back(e.path().string());
                }
            }
        }
        std::sort(sat.begin(), sat.end(), [](const std::string& a, const std::string& b) {
            return std::filesystem::file_size(a) < std::filesystem::file_size(b);
        });
        if (sat.empty()) {
            std::cerr << "no data/*.cnf found: SAT rows skipped\n";
        }
        for (const std::string& f : sat) {
            if (quick && std::filesystem::file_size(f) > 1000000) {
                continue;
            }
            benchSat(csv, f, rng);
        }
    } catch (const std::exception& e) {
        std::cerr << "bench: " << e.what() << "\n";
        return 1;
    }
    std::cerr << "done: " << csvPath << "\n";
    return 0;
}
