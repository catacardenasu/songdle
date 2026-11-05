#include "Benchmark.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>

BenchmarkReport Benchmark::run(const std::vector<Song>& songs, const std::vector<CategorySets>& sets, const BenchmarkConfig& cfg) {
    auto start = std::chrono::steady_clock::now();

    BenchmarkReport report;
    report.games = cfg.games;
    for (const CategorySets& s : sets) {
        StructureStats st;
        st.name = s.name;
        st.buildMillis = s.buildTime;
        st.memoryBytes = s.memoryBytes();
        report.structures.push_back(st);
    }

    std::ofstream csv;
    if (!cfg.csvPath.empty()) {
        csv.open(cfg.csvPath);
        if (csv) csv << "game,guess,structure,matches,search_us\n";
    }

    std::mt19937 rng(cfg.seed);
    std::uniform_int_distribution<int> anySong(0, static_cast<int>(songs.size()) - 1);

    Game game(songs, sets, cfg.maxGuesses);
    game.setVerify(cfg.verify);

    for (int g = 0; g < cfg.games; ++g) {
        game.startRandom(rng, 0);
        while (!game.isOver()) {
            const std::vector<int>& cand = game.candidates();
            int pick;
            if (cand.empty()) {
                pick = anySong(rng);
            } else {
                std::uniform_int_distribution<int> d(0, static_cast<int>(cand.size()) - 1);
                pick = cand[d(rng)];
            }

            GuessResult r = game.guess(pick);
            if (!r.structuresAgree) ++report.mismatches;

            for (int i = 0; i < static_cast<int>(r.timings.size()); ++i) {
                const FilterTiming& t = r.timings[i];
                StructureStats& st = report.structures[i];
                st.sumTreeMicros += t.treeMicros;
                st.slowestTreeMicros = std::max(st.slowestTreeMicros, t.treeMicros);
                ++st.filters;
                if (csv) {
                    csv << g + 1 << ',' << game.guessesUsed() << ",\"" << t.structure << "\","
                        << t.matches << ',' << t.treeMicros << '\n';
                }
            }
        }
        if (game.hasWon()) ++report.solved;
        report.totalGuesses += game.guessesUsed();
    }

    report.wallSeconds = std::chrono::duration<double>(
                             std::chrono::steady_clock::now() - start).count();
    return report;
}