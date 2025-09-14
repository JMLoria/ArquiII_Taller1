#include <bits/stdc++.h>
#include <omp.h>

using namespace std;

struct Args {
    uint64_t n = 100'000'000;
    int bins = 256;
    int threads = 0;
    uint64_t seed = 12345;
    char variant = 'a'; // a, b, c
    bool csv = false;
};

int main(int argc, char** argv) {
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

    if (args.threads <= 0) args.threads = omp_get_max_threads();
    omp_set_num_threads(args.threads);

    vector<int> data(args.n);
    uniform_int_distribution<int> dist(0, args.bins - 1);

    // Generate data
    #pragma omp parallel 
    {
        int tid = omp_get_thread_num();
        mt19937 rng(args.seed + tid);
        #pragma omp for schedule(static)
        for (uint64_t i = 0; i < args.n; i++) {
            data[i] = dist(rng);
        }
    }

    vector<uint32_t> hist(args.bins, 0);

    auto t1 = chrono::high_resolution_clock::now();

    if (args.variant == 'a') {
        // Variant A: Private by thread + reduction
        vector<vector<uint32_t>> local(args.threads, vector<uint32_t>(args.bins, 0));
        #pragma omp parallel 
        {
            int tid = omp_get_thread_num();
            auto& h = local[tid];
            #pragma omp for schedule(static)
            for (uint64_t i = 0; i < args.n; i++) h[data[i]]++;
        }
        for (int b = 0; b < args.bins; b++) {
            uint64_t sum = 0;
            for (int t = 0; t < args.threads; t++) sum += local[t][b];
            hist[b] = sum;
        }
    } 

    else if (args.variant == 'b') {
        // Variant B: With locks
        vector<omp_lock_t> locks(args.bins);
        for (int b = 0; b < args.bins; b++) omp_init_lock(&locks[b]);
        
        #pragma omp parallel for schedule(static)
        for (uint64_t i = 0; i < args.n; i++) {
            int v = data[i];
            omp_set_lock(&locks[v]);
            hist[v]++;
            omp_unset_lock(&locks[v]);
        }

        for (int b = 0; b < args.bins; b++) omp_destroy_lock(&locks[b]);
    }

    else if (args.variant == 'c') { 
        // Variant C: Atomic 
        #pragma omp parallel for schedule(static)
        for (uint64_t i = 0; i < args.n; i++) {
            int v = data[i];
            #pragma omp atomic
            hist[v]++;
        }
    }

    auto t2 = chrono::high_resolution_clock::now();
    uint64_t sum = accumulate(hist.begin(), hist.end(), 0ull);
    
    cout << "OpenMP variant " << args.variant
        << " | Threads = " << args.threads
        << " | Time(us) = " << chrono::duration_cast<chrono::microseconds>(t2-t1).count()
        << " | Sum = " << sum << endl;

}