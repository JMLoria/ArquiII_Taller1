#include <bits/stdc++.h>
#include <thread>
#include <atomic>
#include <mutex>

using namespace std;

struct Args {
    uint64_t n = 100'000'000;
    int bins = 256;
    int threads = thread::hardware_concurrency();
    uint64_t seed = 12345;
    char variant = 'a'; // a, b, c
    bool csv = false;
};

int main(int argc, char** argv){
    Args args;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            string s = argv[i];
            if (s == "--n") args.n = stoull(argv[++i]);
            else if (s == "--bins") args.bins = stoi(argv[++i]);
            else if (s == "--threads") args.threads = stoi(argv[++i]);
            else if (s == "--seed") args.seed = stoull(argv[++i]);
            else if (s == "--variant") args.variant = argv[++i][0];
            else if (s == "--csv") args.csv = true;
        }
    }

    vector<int> data (args.n);
    uniform_int_distribution<int> dist(0, args.bins - 1);

    // Parallel generation
    {
        vector<thread> workers;
        auto gen = [&](int tid){
            mt19937 rng(args.seed + tid);
            uint64_t chunk = (args.n + args.threads - 1) / args.threads;
            uint64_t beg = min<uint64_t>(tid * chunk, args.n);
            uint64_t end = min<uint64_t>((tid + 1) + chunk, args.n);
            for (uint64_t i = beg; i < end; i++) data[i] = dist(rng);
        };
        for (int t = 0; t < args.threads; t++) workers.emplace_back(gen, t);
        for (auto& th: workers) th.join();
    }

    auto t1 = chrono::high_resolution_clock::now();
    vector<uint32_t> hist(args.bins, 0);
    vector<atomic<uint32_t>> hist_atomic(args.bins);
    if (args.variant == 'c') {
        for (auto& h: hist_atomic) h.store(0);
    }

    if (args.variant == 'a') {
        // Variant A: Private by thread
        vector<vector<uint32_t>> local(args.threads, vector<uint32_t>(args.bins, 0));
        vector<thread> workers;
        auto work = [&](int tid){
            uint64_t chunk = (args.n + args.threads - 1) / args.threads;
            uint64_t beg = min<uint64_t>(tid * chunk, args.n);
            uint64_t end = min<uint64_t>((tid + 1) * chunk, args.n);
            auto& h = local[tid];
            for (uint64_t i = beg; i < end; i++) h[data[i]]++;
        };
        for (int t = 0; t < args.threads; t++) workers.emplace_back(work, t);
        for (auto& th: workers) th.join();
        for (int b = 0; b < args.bins; b++) {
            uint64_t sum = 0;
            for (int t = 0; t < args.threads; t++) sum += local[t][b];
            hist[b] = sum;
        }
    }

    else if (args.variant == 'b') {
        // Variant B: Global mutex
        mutex m;
        vector<thread> workers;
        auto work = [&](int tid){
            uint64_t chunk = (args.n + args.threads - 1) / args.threads;
            uint64_t beg = min<uint64_t>(tid * chunk, args.n);
            uint64_t end = min<uint64_t>((tid + 1 ) * chunk, args.n);
            for (uint64_t i = beg; i < end; i++) {
                lock_guard<mutex> lk(m);
                hist[data[i]]++;
            }
        };  

        for (int t = 0; t < args.threads; t++) workers.emplace_back(work, t);
        for (auto& th: workers) th.join();
    }

    else if (args.variant == 'c') {
        // Variant C: Atomic
        vector<thread> workers;
        auto work = [&](int tid){
            uint64_t chunk = (args.n + args.threads - 1) / args.threads;
            uint64_t beg = min<uint64_t>(tid * chunk, args.n);
            uint64_t end = min<uint64_t>((tid + 1) * chunk, args.n);
            for (uint64_t i = beg; i < end; i++) {
                hist_atomic[data[i]].fetch_add(1);
            }
        };  

        for (int t = 0; t < args.threads; t++) workers.emplace_back(work, t);
        for (auto& th: workers) th.join();
    }

    auto t2 = chrono::high_resolution_clock::now();
    uint64_t sum = 0;
    if (args.variant == 'c') {
        for (auto& h : hist_atomic) sum += h.load();
    } else {
        for (auto h: hist) sum += h;
    }

    cout << "std::thread variant " << args.variant
        << " | Threads = " << args.threads
        << " | Time(us) = " << chrono::duration_cast<chrono::microseconds>(t2-t1).count()
        << " | Sum = " << sum << endl;

}