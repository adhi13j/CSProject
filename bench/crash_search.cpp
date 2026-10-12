/**
 * @file crash_search.cpp
 * @brief Finds the tree height at which each recursive function overflows the call stack (SPEC 8.2).
 *
 * For every recursive function it builds a left-deep chain of growing height in a fork()ed child
 * and calls the function once. The height is doubled until a child dies of a signal (SIGSEGV from
 * stack overflow), then bisected. This program links the UNinstrumented library, so the frame
 * sizes are those of the real code. Rows are appended to the CSV written by `bench`.
 *
 * Usage: crash_search [results.csv]
 *
 * @authors Replace with group member names.
 */
#include "formula.h"

#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <functional>
#include <iostream>

namespace {

/**
 * @brief Build a left-deep chain `((x+x)+x)...` with `height + 1` leaves directly, without strings.
 * @param height Height of the chain in edges (leaf = 0).
 * @return The root; the caller does not free it (the child process exits right away).
 * @throws std::bad_alloc if memory runs out.
 * @note Complexity: O(height) time / O(height) space.
 */
Node* buildChain(long height) {
    Node* root = new Node{NodeType::ATOM, "x", nullptr, nullptr};
    for (long i = 0; i < height; ++i) {
        Node* leaf = new Node{NodeType::ATOM, "x", nullptr, nullptr};
        root = new Node{NodeType::OR, "+", root, leaf};
    }
    return root;
}

/** @brief Outcome of one probe. */
struct Probe {
    bool crashed;
    long rssKb;
};

/**
 * @brief Run `fn(chain of the given height)` once in a child process.
 * @param height Chain height.
 * @param fn     The recursive function to call.
 * @return Whether the child died abnormally, and the child's peak RSS.
 * @throws Nothing.
 * @note Complexity: O(height) time / O(height) space (in the child).
 */
Probe probe(long height, const std::function<void(const Node*)>& fn) {
    std::fflush(nullptr);
    const pid_t pid = fork();
    if (pid == 0) {
        const Node* root = buildChain(height);
        fn(root);
        _exit(0);   // not freeing: the child ends here
    }
    int status = 0;
    rusage ru{};
    wait4(pid, &status, 0, &ru);
    return {!(WIFEXITED(status) && WEXITSTATUS(status) == 0), ru.ru_maxrss};
}

} // namespace

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "bench/results.csv";
    std::FILE* out = std::fopen(path, "a");
    if (out == nullptr) {
        std::cerr << "crash_search: cannot append to " << path << "\n";
        return 1;
    }
    rlimit lim{};
    getrlimit(RLIMIT_STACK, &lim);
    std::cerr << "stack limit: " << (lim.rlim_cur == RLIM_INFINITY ? -1L : static_cast<long>(lim.rlim_cur / 1024))
              << " kB\n";
    std::fprintf(out, "env,-,NA,NA,NA,stack_limit_kb,NA,%ld,0,OK\n",
                 lim.rlim_cur == RLIM_INFINITY ? -1L : static_cast<long>(lim.rlim_cur / 1024));

    struct Target {
        const char* name;
        std::function<void(const Node*)> fn;
    };
    const Assignment assign{{"x", true}};
    const std::vector<Target> targets = {
        {"inorderRec", [](const Node* r) { (void)inorderRec(r); }},
        {"preorderRec", [](const Node* r) { (void)preorderRec(r); }},
        {"postorderRec", [](const Node* r) { (void)postorderRec(r); }},
        {"heightRec", [](const Node* r) { (void)heightRec(r); }},
        {"evaluateRec", [&assign](const Node* r) { (void)evaluateRec(r, assign); }},
    };
    const long cap = 4000000;   // a 4M-high chain is about 500 MB; beyond that we report "no crash"

    for (const Target& t : targets) {
        long ok = 1000;     // assumed fine; verified below
        long bad = -1;
        long rssOk = 0;
        long rssBad = 0;
        if (probe(ok, t.fn).crashed) {
            bad = ok;
            ok = 0;
        }
        for (long h = ok * 2; bad < 0 && h <= cap; h *= 2) {
            const Probe p = probe(h, t.fn);
            if (p.crashed) {
                bad = h;
                rssBad = p.rssKb;
            } else {
                ok = h;
                rssOk = p.rssKb;
            }
        }
        while (bad > 0 && bad - ok > 500) {   // bisect to a 500-level resolution
            const long mid = ok + (bad - ok) / 2;
            const Probe p = probe(mid, t.fn);
            if (p.crashed) {
                bad = mid;
                rssBad = p.rssKb;
            } else {
                ok = mid;
                rssOk = p.rssKb;
            }
        }
        std::fprintf(out, "crash-search,LEFT_DEEP,%ld,%ld,%ld,%s,NA,%ld,%ld,OK\n", 2 * ok + 1, ok, ok + 1, t.name,
                     ok + 1, rssOk);
        if (bad > 0) {
            std::fprintf(out, "crash-search,LEFT_DEEP,%ld,%ld,%ld,%s,NA,%ld,%ld,CRASH\n", 2 * bad + 1, bad, bad + 1,
                         t.name, bad + 1, rssBad);
            std::cerr << t.name << ": OK at height " << ok << ", CRASH at height " << bad << "\n";
        } else {
            std::cerr << t.name << ": no crash up to height " << ok << "\n";
        }
        std::fflush(out);
    }
    std::fclose(out);
    return 0;
}
