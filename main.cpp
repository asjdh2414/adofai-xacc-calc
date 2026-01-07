#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <sstream>
#include <limits>
#include <queue>
#include <windows.h>

using std::cin;
using std::cout;
using std::string;

using int64 = long long;

struct Solution {
    int early20;
    int red40;
    int yellow75;
    int perfect100;
    int miss0;
    int64 totalScore;
    int denom;
    int64 disp_scaled;
};

static int64 pow10_i(int d) {
    int64 p = 1;
    for (int i = 0; i < d; ++i) p *= 10;
    return p;
}

static int64 floor_div_pos(int64 a, int64 b) {
    if (a >= 0) return a / b;
    return -(((-a) + b - 1) / b);
}
static int64 floor_div_any(int64 a, int64 b) {
    if (b < 0) return floor_div_pos(-a, -b);
    return floor_div_pos(a, b);
}
static int64 ceil_div_any(int64 a, int64 b) {
    return -floor_div_any(-a, b);
}

static int64 parse_target_scaled(const string& s, int d) {
    int sign = 1;
    size_t i = 0;
    if (!s.empty() && (s[0] == '+' || s[0] == '-')) {
        if (s[0] == '-') sign = -1;
        i = 1;
    }

    int64 ip = 0;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
        ip = ip * 10 + (s[i] - '0');
        i++;
    }

    int64 fp = 0;
    int fp_digits = 0;
    if (i < s.size() && s[i] == '.') {
        i++;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9' && fp_digits < d) {
            fp = fp * 10 + (s[i] - '0');
            fp_digits++;
            i++;
        }
    }
    while (fp_digits < d) {
        fp *= 10;
        fp_digits++;
    }

    int64 scale = pow10_i(d);
    int64 val = ip * scale + fp;
    val *= sign;

    if (val < 0) val = 0;
    int64 maxv = 100 * scale;
    if (val > maxv) val = maxv;
    return val;
}

static int64 compute_display_scaled(int64 S, int D, int d) {
    int64 scale = pow10_i(d);
    int64 num = scale * S;
    int64 val = (num + (D / 2)) / D;  // half-up
    int64 maxv = 100 * scale;
    if (val < 0) val = 0;
    if (val > maxv) val = maxv;
    return val;
}

static void print_scaled_percent(int64 scaled, int d) {
    int64 scale = pow10_i(d);
    int64 ip = scaled / scale;
    int64 fp = scaled % scale;
    cout << ip << '.';
    string s = std::to_string(fp);
    if ((int)s.size() < d) cout << string(d - (int)s.size(), '0');
    cout << s << '%';
}

static bool apply_ge(int64 a, int64 b, int64& lo, int64& hi) {
    if (a == 0) return (0 >= b);
    if (a > 0) lo = std::max(lo, ceil_div_any(b, a));
    else hi = std::min(hi, floor_div_any(b, a));
    return (lo <= hi);
}
static bool apply_le(int64 a, int64 b, int64& lo, int64& hi) {
    if (a == 0) return (0 <= b);
    if (a > 0) hi = std::min(hi, floor_div_any(b, a));
    else lo = std::max(lo, ceil_div_any(b, a));
    return (lo <= hi);
}
static bool apply_gt(int64 a, int64 b, int64& lo, int64& hi) {
    if (a == 0) return (0 > b);
    if (a > 0) lo = std::max(lo, floor_div_any(b, a) + 1);
    else hi = std::min(hi, ceil_div_any(b, a) - 1);
    return (lo <= hi);
}

static bool compute_E_range(
    int mode,
    int N_tiles,
    int d,
    int64 targetScaled,
    int64 S0,
    int64& Emin,
    int64& Emax
) {
    const int64 E_CAP = 10000000LL;
    int64 lo = 0, hi = E_CAP;

    const int64 N = (int64)N_tiles;
    const int64 T = targetScaled;
    const int64 K = 2LL * pow10_i(d); // 2*10^d

    // (2T-1)(N+E) <= K(S0+20E)
    const int64 a1 = 20LL * K - (2LL * T - 1LL);
    const int64 b1 = (2LL * T - 1LL) * N - K * S0;
    if (!apply_ge(a1, b1, lo, hi)) return false;

    // K(S0+20E) < (2T+1)(N+E)
    const int64 a2 = 20LL * K - (2LL * T + 1LL);
    const int64 b2 = (2LL * T + 1LL) * N - K * S0;
    // a2*E < b2  <=>  (-a2)*E > (-b2)
    if (!apply_gt(-a2, -b2, lo, hi)) return false;

    // mode 1: 10^d * S >= T * D
    if (mode == 1) {
        const int64 scale = pow10_i(d);
        const int64 a3 = 20LL * scale - T;
        const int64 b3 = T * N - scale * S0;
        if (!apply_ge(a3, b3, lo, hi)) return false;
    }

    lo = std::max<int64>(lo, 0);
    hi = std::min<int64>(hi, E_CAP);
    if (lo > hi) return false;

    Emin = lo;
    Emax = hi;
    return true;
}


struct Node {
    int Y, R, M, P;
    int64 S0;
    int64 Emin, Ecur;
};

struct NodeBetter {
    // Miss 최대 → Perfect 최대 → Early!! 최대 → Yellow 최대 → Red 최소
    bool operator()(const Node& a, const Node& b) const {
        if (a.M != b.M) return a.M < b.M;
        if (a.P != b.P) return a.P < b.P;
        if (a.Ecur != b.Ecur) return a.Ecur < b.Ecur;
        if (a.Y != b.Y) return a.Y < b.Y;
        return a.R > b.R;
    }
};

static bool is_valid_solution(int mode, int N_tiles, int d, int64 targetScaled, const Node& node, int64 E) {
    // 새 제한: 2*Early!! <= Perfect
    if (2LL * E > (int64)node.P) return false;

    int64 S = node.S0 + 20LL * E;
    int D = (int)(N_tiles + (int)E);

    int64 disp = compute_display_scaled(S, D, d);
    if (disp != targetScaled) return false;

    if (mode == 1) {
        int64 scale = pow10_i(d);
        if (scale * S < targetScaled * (int64)D) return false;
    }
    return true;
}

static std::vector<Solution> generate_solutions(int mode, int N_tiles, int d, int64 targetScaled, int K, bool noMiss) {
    std::vector<Solution> out;
    if (N_tiles <= 0 || K <= 0) return out;

    // (Red+Yellow) 최소 우선
    for (int k = 0; k <= N_tiles; ++k) {
        std::priority_queue<Node, std::vector<Node>, NodeBetter> pq;

        for (int Y = k; Y >= 0; --Y) {
            int R = k - Y;

            int Mmax = noMiss ? 0 : (N_tiles - k);
            for (int M = Mmax; M >= 0; --M) {
                int P = N_tiles - k - M;
                if (P < 0) continue;

                int64 S0 = 100LL * P + 75LL * Y + 40LL * R;

                int64 Emin = 0, Emax = 0;
                if (!compute_E_range(mode, N_tiles, d, targetScaled, S0, Emin, Emax)) continue;

                // 새 제한으로 인해 E의 상한이 Perfect/2로 더 줄어든다.
                int64 Ecap2 = (int64)P / 2LL;
                if (Emax > Ecap2) Emax = Ecap2;
                if (Emax < Emin) continue;

                Node node;
                node.Y = Y;
                node.R = R;
                node.M = M;
                node.P = P;
                node.S0 = S0;
                node.Emin = Emin;
                node.Ecur = Emax;

                while (node.Ecur >= node.Emin && !is_valid_solution(mode, N_tiles, d, targetScaled, node, node.Ecur)) {
                    node.Ecur--;
                }
                if (node.Ecur >= node.Emin) pq.push(node);
            }
        }

        while (!pq.empty() && (int)out.size() < K) {
            Node node = pq.top();
            pq.pop();

            int64 E = node.Ecur;
            int D = (int)(N_tiles + (int)E);
            int64 S = node.S0 + 20LL * E;

            Solution sol;
            sol.early20 = (int)E;
            sol.red40 = node.R;
            sol.yellow75 = node.Y;
            sol.perfect100 = node.P;
            sol.miss0 = node.M;
            sol.totalScore = S;
            sol.denom = D;
            sol.disp_scaled = compute_display_scaled(S, D, d);

            out.push_back(sol);

            node.Ecur = E - 1;
            while (node.Ecur >= node.Emin && !is_valid_solution(mode, N_tiles, d, targetScaled, node, node.Ecur)) {
                node.Ecur--;
            }
            if (node.Ecur >= node.Emin) pq.push(node);
        }

        if ((int)out.size() >= K) break;
    }

    return out;
}

int main() {
    std::ios::sync_with_stdio(false);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    cout << "얼불춤 절대정확도 판정 조합 생성기\n";
    cout << "가중치: Perfect=100, Yellow=75, Red=40, Early!!=20, Miss=0\n";
    cout << "모드 0: 표기만 맞으면 됨, 모드 1: 표기도 맞고 실제값도 목표 이상\n";
    cout << "옵션: 입력 끝에 nomiss를 붙이면 Miss=0인 조합만 출력함\n\n";

    while (true) {
        int K;
        cout << "K개의 결과 표시 (0 입력시 프로그램 종료): " << std::flush;
        if (!(cin >> K)) return 0;
        if (K == 0) break;

        int d;
        cout << "표시 자릿수 d 입력 (예: 2, 3, 4): " << std::flush;
        if (!(cin >> d)) return 0;
        if (d < 0 || d > 6) {
            cout << "d는 0~6 범위로 입력해야 함\n\n";
            continue;
        }

        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        cout << "입력: 모드  타일수N  목표정확도  [nomiss] : " << std::flush;

        string line;
        std::getline(cin, line);
        if (line.empty()) {
            cout << "입력이 비었음\n\n";
            continue;
        }

        std::istringstream iss(line);
        int mode, N_tiles;
        string targetStr;
        if (!(iss >> mode >> N_tiles >> targetStr)) {
            cout << "입력 형식 오류: 예) 0 1900 94.74  또는  1 1900 89.47 nomiss\n\n";
            continue;
        }

        bool noMiss = false;
        string opt;
        while (iss >> opt) {
            if (opt == "nomiss") noMiss = true;
        }

        if (!(mode == 0 || mode == 1) || N_tiles <= 0 || K < 0) {
            cout << "Invalid input.\n\n";
            continue;
        }

        int64 targetScaled = parse_target_scaled(targetStr, d);
        auto sols = generate_solutions(mode, N_tiles, d, targetScaled, K, noMiss);

        cout << "\n목표 정확도 = ";
        print_scaled_percent(targetScaled, d);
        cout << ", 타일수 N = " << N_tiles << ", mode = " << mode;
        if (noMiss) cout << ", 옵션 = nomiss";
        cout << "\n";
        cout << "우선순위: (Red+Yellow 최소) → (Miss 최대) → (Perfect 최대) → (Early!! 최대) → (Yellow 최대) → (Red 최소)\n";

        if (sols.empty()) {
            cout << "No solutions found.\n\n";
            continue;
        }

        for (int i = 0; i < (int)sols.size(); ++i) {
            const auto& s = sols[i];
            cout << "---- #" << (i + 1) << " ----\n";
            cout << "Early!! : " << s.early20
                 << ", Red : " << s.red40
                 << ", Yellow : " << s.yellow75
                 << ", Perfect! : " << s.perfect100
                 << ", Miss : " << s.miss0 << "\n";
            cout << "Display : ";
            print_scaled_percent(s.disp_scaled, d);
            cout << "  (분모=" << s.denom << ", S=" << s.totalScore << ")\n";
        }

        cout << "\n";
    }

    cout << "Exit.\n";
    return 0;
}
