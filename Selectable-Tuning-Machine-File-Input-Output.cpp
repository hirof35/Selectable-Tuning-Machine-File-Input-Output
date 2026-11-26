#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>
#include <iomanip>
#include <limits>
#include <numeric>
#include <string>
#include <sstream>
#define M_PI 3.14159265
// ===================== 設定 =====================
constexpr int DIMENSION = 2;
constexpr double MIN_X = -5.12;
constexpr double MAX_X = 5.12;

// ===================== 乱数生成器 =====================
std::mt19937 rng(std::random_device{}());
double rand_double(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}
int rand_int(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}
double rand_normal(double mean, double stddev) {
    std::normal_distribution<double> dist(mean, stddev);
    return dist(rng);
}

// ===================== 目的関数 =====================
double rastrigin(const std::vector<double>& x) {
    double sum = DIMENSION * 10.0;
    for (double xi : x) {
        sum += xi * xi - 10.0 * cos(2.0 * M_PI * xi);
    }
    return sum;
}

// ===================== ファイル入出力ユーティリティ =====================
// 結果をテキストに追記
void save_text_result(const std::string& filename,
    const std::string& algo_name,
    const std::vector<double>& x,
    double fitness) {
    std::ofstream ofs(filename, std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "⚠️  書き込み失敗: " << filename << "\n";
        return;
    }
    ofs << "===== " << algo_name << " =====\n";
    ofs << "解: (";
    for (size_t i = 0; i < x.size(); ++i) {
        if (i > 0) ofs << ", ";
        ofs << std::fixed << std::setprecision(6) << x[i];
    }
    ofs << ")\n評価値: " << std::fixed << std::setprecision(8) << fitness << "\n";
    ofs << "----------------------------------------\n";
    ofs.close();
    std::cout << "✅ テキスト保存: " << filename << "\n";
}

// 結果をCSV形式で保存（ヘッダーは1回のみ）
void save_csv_result(const std::string& filename,
    const std::string& algo_name,
    const std::vector<double>& x,
    double fitness) {
    bool need_header = false;
    {
        std::ifstream ifs(filename);
        if (!ifs.is_open()) need_header = true;
    }
    std::ofstream ofs(filename, std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "⚠️  CSV保存失敗\n";
        return;
    }
    if (need_header) {
        ofs << "アルゴリズム,x1,x2,評価値\n";
    }
    ofs << algo_name;
    for (double xi : x) {
        ofs << "," << std::fixed << std::setprecision(8) << xi;
    }
    ofs << "," << std::fixed << std::setprecision(8) << fitness << "\n";
    ofs.close();
    std::cout << "✅ CSV保存: " << filename << "\n";
}

// 設定ファイル読み込み
bool load_config(const std::string& filename,
    int& hc_iter, double& hc_step,
    int& sa_iter, double& sa_temp, double& sa_cool,
    int& ga_pop, int& ga_gen, double& ga_mut,
    int& pso_swarm, int& pso_iter) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::cerr << "ℹ️  設定ファイルが見つかりません。既定値を使用します。\n";
        return false;
    }
    std::string key, eq;
    int ival; double dval;
    while (ifs >> key >> eq >> ival >> dval) {
        if (key == "HC_ITER") hc_iter = ival;
        else if (key == "HC_STEP") hc_step = dval;
        else if (key == "SA_ITER") sa_iter = ival;
        else if (key == "SA_TEMP") sa_temp = dval;
        else if (key == "SA_COOL") sa_cool = dval;
        else if (key == "GA_POP") ga_pop = ival;
        else if (key == "GA_GEN") ga_gen = ival;
        else if (key == "GA_MUT") ga_mut = dval;
        else if (key == "PSO_SWARM") pso_swarm = ival;
        else if (key == "PSO_ITER") pso_iter = ival;
    }
    ifs.close();
    std::cout << "✅ 設定読み込み完了: " << filename << "\n";
    return true;
}

// ファイル内容表示
void show_file(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::cout << "ファイルが存在しません。\n";
        return;
    }
    std::cout << "\n--- " << filename << " の内容 ---\n";
    std::string line;
    while (std::getline(ifs, line)) std::cout << line << "\n";
    std::cout << "------------------------\n\n";
}

// ===================== 1. 山登り法 =====================
void hill_climbing(int iterations, double step_size,
    const std::string& txt_file, const std::string& csv_file) {
    std::cout << "\n===== 山登り法 =====\n";
    std::vector<double> current(DIMENSION);
    for (int i = 0; i < DIMENSION; ++i)
        current[i] = rand_double(MIN_X, MAX_X);
    double current_val = rastrigin(current);

    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<double> neighbor = current;
        for (int i = 0; i < DIMENSION; ++i) {
            neighbor[i] += rand_normal(0, step_size);
            neighbor[i] = std::max(MIN_X, std::min(MAX_X, neighbor[i]));
        }
        double neighbor_val = rastrigin(neighbor);
        if (neighbor_val < current_val) {
            current = neighbor;
            current_val = neighbor_val;
        }
    }
    std::cout << "最終結果:\n  解: ("
        << std::fixed << std::setprecision(6) << current[0] << ", " << current[1] << ")\n"
        << "  評価値: " << std::fixed << std::setprecision(8) << current_val << "\n";
    save_text_result(txt_file, "山登り法", current, current_val);
    save_csv_result(csv_file, "山登り法", current, current_val);
}

// ===================== 2. 焼きなまし法 =====================
void simulated_annealing(int iterations, double T0, double cooling,
    const std::string& txt_file, const std::string& csv_file) {
    std::cout << "\n===== 焼きなまし法 =====\n";
    std::vector<double> current(DIMENSION);
    for (int i = 0; i < DIMENSION; ++i)
        current[i] = rand_double(MIN_X, MAX_X);
    double current_val = rastrigin(current);
    std::vector<double> best = current;
    double best_val = current_val;
    double T = T0;

    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<double> neighbor = current;
        for (int i = 0; i < DIMENSION; ++i) {
            neighbor[i] += rand_normal(0, 0.15);
            neighbor[i] = std::max(MIN_X, std::min(MAX_X, neighbor[i]));
        }
        double neighbor_val = rastrigin(neighbor);
        double delta = neighbor_val - current_val;
        if (delta < 0.0 || rand_double(0.0, 1.0) < exp(-delta / T)) {
            current = neighbor;
            current_val = neighbor_val;
            if (current_val < best_val) {
                best = current;
                best_val = current_val;
            }
        }
        T *= cooling;
    }
    std::cout << "最終結果:\n  解: ("
        << std::fixed << std::setprecision(6) << best[0] << ", " << best[1] << ")\n"
        << "  評価値: " << std::fixed << std::setprecision(8) << best_val << "\n";
    save_text_result(txt_file, "焼きなまし法", best, best_val);
    save_csv_result(csv_file, "焼きなまし法", best, best_val);
}

// ===================== 3. 遺伝的アルゴリズム =====================
void genetic_algorithm(int population_size, int generations, double mutation_rate,
    const std::string& txt_file, const std::string& csv_file) {
    std::cout << "\n===== 遺伝的アルゴリズム =====\n";
    using Individual = std::vector<double>;
    std::vector<Individual> pop(population_size, Individual(DIMENSION));
    std::vector<double> fitness(population_size);
    for (int i = 0; i < population_size; ++i) {
        for (int j = 0; j < DIMENSION; ++j)
            pop[i][j] = rand_double(MIN_X, MAX_X);
        fitness[i] = rastrigin(pop[i]);
    }
    for (int gen = 0; gen < generations; ++gen) {
        int best_idx = std::min_element(fitness.begin(), fitness.end()) - fitness.begin();
        Individual best = pop[best_idx];
        double best_fit = fitness[best_idx];
        std::vector<Individual> new_pop;
        new_pop.push_back(best);
        while (new_pop.size() < population_size) {
            int a = rand_int(0, population_size - 1), b = rand_int(0, population_size - 1);
            int p1 = (fitness[a] < fitness[b]) ? a : b;
            a = rand_int(0, population_size - 1); b = rand_int(0, population_size - 1);
            int p2 = (fitness[a] < fitness[b]) ? a : b;
            Individual child(DIMENSION);
            for (int d = 0; d < DIMENSION; ++d)
                child[d] = (rand_double(0, 1) < 0.5) ? pop[p1][d] : pop[p2][d];
            if (rand_double(0, 1) < mutation_rate) {
                int mut_dim = rand_int(0, DIMENSION - 1);
                child[mut_dim] += rand_normal(0, 0.3);
                child[mut_dim] = std::max(MIN_X, std::min(MAX_X, child[mut_dim]));
            }
            new_pop.push_back(child);
        }
        pop = std::move(new_pop);
        for (int i = 0; i < population_size; ++i)
            fitness[i] = rastrigin(pop[i]);
    }
    int best_idx = std::min_element(fitness.begin(), fitness.end()) - fitness.begin();
    std::cout << "最終結果:\n  解: ("
        << std::fixed << std::setprecision(6) << pop[best_idx][0] << ", " << pop[best_idx][1] << ")\n"
        << "  評価値: " << std::fixed << std::setprecision(8) << fitness[best_idx] << "\n";
    save_text_result(txt_file, "遺伝的アルゴリズム", pop[best_idx], fitness[best_idx]);
    save_csv_result(csv_file, "遺伝的アルゴリズム", pop[best_idx], fitness[best_idx]);
}

// ===================== 4. PSO =====================
void particle_swarm_optimization(int swarm_size, int iterations,
    const std::string& txt_file, const std::string& csv_file) {
    std::cout << "\n===== 粒子群最適化 (PSO) =====\n";
    struct Particle {
        std::vector<double> x, v, best_x;
        double best_val;
    };
    const double w = 0.729, c1 = 1.49445, c2 = 1.49445;
    std::vector<Particle> swarm(swarm_size);
    std::vector<double> global_best_x(DIMENSION);
    double global_best_val = std::numeric_limits<double>::max();
    for (int i = 0; i < swarm_size; ++i) {
        swarm[i].x.resize(DIMENSION); swarm[i].v.resize(DIMENSION);
        for (int d = 0; d < DIMENSION; ++d) {
            swarm[i].x[d] = rand_double(MIN_X, MAX_X);
            swarm[i].v[d] = rand_double(-1.0, 1.0);
        }
        double val = rastrigin(swarm[i].x);
        swarm[i].best_x = swarm[i].x;
        swarm[i].best_val = val;
        if (val < global_best_val) {
            global_best_val = val;
            global_best_x = swarm[i].x;
        }
    }
    for (int iter = 0; iter < iterations; ++iter) {
        for (int i = 0; i < swarm_size; ++i) {
            auto& p = swarm[i];
            for (int d = 0; d < DIMENSION; ++d) {
                double r1 = rand_double(0, 1), r2 = rand_double(0, 1);
                p.v[d] = w * p.v[d] + c1 * r1 * (p.best_x[d] - p.x[d])
                    + c2 * r2 * (global_best_x[d] - p.x[d]);
                p.v[d] = std::max(-1.0, std::min(1.0, p.v[d]));
            }
            for (int d = 0; d < DIMENSION; ++d) {
                p.x[d] += p.v[d];
                p.x[d] = std::max(MIN_X, std::min(MAX_X, p.x[d]));
            }
            double val = rastrigin(p.x);
            if (val < p.best_val) {
                p.best_val = val;
                p.best_x = p.x;
                if (val < global_best_val) {
                    global_best_val = val;
                    global_best_x = p.x;
                }
            }
        }
    }
    std::cout << "最終結果:\n  解: ("
        << std::fixed << std::setprecision(6) << global_best_x[0] << ", " << global_best_x[1] << ")\n"
        << "  評価値: " << std::fixed << std::setprecision(8) << global_best_val << "\n";
    save_text_result(txt_file, "PSO", global_best_x, global_best_val);
    save_csv_result(csv_file, "PSO", global_best_x, global_best_val);
}

// ===================== メイン =====================
int main() {
    // 既定パラメータ
    int hc_iter = 10000;     double hc_step = 0.1;
    int sa_iter = 50000;      double sa_temp = 100.0, sa_cool = 0.9995;
    int ga_pop = 100;         int ga_gen = 500;       double ga_mut = 0.15;
    int pso_swarm = 50;       int pso_iter = 1000;

    // 設定ファイル読み込み
    load_config("config.txt", hc_iter, hc_step, sa_iter, sa_temp, sa_cool,
        ga_pop, ga_gen, ga_mut, pso_swarm, pso_iter);

    const std::string txt_file = "results.txt";
    const std::string csv_file = "results.csv";

    std::cout << "=== 統合：選択式チューニングマシン ===\n";
    std::cout << "目的関数: Rastrigin  (最小値 0 を探索)\n";
    std::cout << "結果保存: " << txt_file << " / " << csv_file << "\n\n";

    int choice;
    do {
        std::cout << "\n========== メニュー ==========\n";
        std::cout << " 1: 山登り法\n";
        std::cout << " 2: 焼きなまし法\n";
        std::cout << " 3: 遺伝的アルゴリズム\n";
        std::cout << " 4: PSO(粒子群最適化)\n";
        std::cout << " 5: 結果ファイル表示\n";
        std::cout << " 0: 終了\n";
        std::cout << " 選択: ";
        if (!(std::cin >> choice)) break;

        switch (choice) {
        case 1: hill_climbing(hc_iter, hc_step, txt_file, csv_file); break;
        case 2: simulated_annealing(sa_iter, sa_temp, sa_cool, txt_file, csv_file); break;
        case 3: genetic_algorithm(ga_pop, ga_gen, ga_mut, txt_file, csv_file); break;
        case 4: particle_swarm_optimization(pso_swarm, pso_iter, txt_file, csv_file); break;
        case 5:
            std::cout << "\n===== テキスト結果 =====\n"; show_file(txt_file);
            std::cout << "===== CSV結果 =====\n"; show_file(csv_file);
            break;
        case 0: std::cout << "終了します。\n"; break;
        default: std::cout << "無効な選択です。\n";
        }
    } while (choice != 0);
    return 0;
}
