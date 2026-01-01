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
    int early20;    // Early!! (20점)  [타일 수에는 포함되지 않음]
    int red40;      // Red = Early!+Late! (40점)
    int yellow75;   // Yellow = EPerfect!+LPerfect! (75점)
    int perfect100; // Perfect! (100점)
    int miss0;      // Miss/Overload (0점)
    int64 totalScore; // S = 100P + 75Y + 40R + 20E
    int disp_bp;      // 표시 정확도(%) 소수 둘째 자리 정수(예: 89.47 -> 8947)
    int denom;        // 정확도 분모 = (타일 수 N + Early!!)
};

static int64 floor_div_pos(int64 a, int64 b) { // b > 0, floor(a/b)
    if (b <= 0) return 0;
    if (a >= 0) return a / b;
    return -(((-a) + b - 1) / b);
}
static int64 floor_div_any(int64 a, int64 b) { // b != 0, floor(a/b)
    if (b < 0) return floor_div_pos(-a, -b);
    return floor_div_pos(a, b);
}
static int64 ceil_div_any(int64 a, int64 b) { // b != 0, ceil(a/b)
    return -floor_div_any(-a, b);
}

static int parse_target_bp(const string& s) {
    // "89.47" -> 8947, "89.4" -> 8940, "89" -> 8900
    int sign = 1;
    size_t i = 0;
    if (!s.empty() && (s[0] == '+' || s[0] == '-')) {
        if (s[0] == '-') sign = -1;
        i = 1;
    }

    long long ip = 0;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
        ip = ip * 10 + (s[i] - '0');
        i++;
    }

    long long fp = 0;
    int fp_digits = 0;
    if (i < s.size() && s[i] == '.') {
        i++;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9' && fp_digits < 2) {
            fp = fp * 10 + (s[i] - '0');
            fp_digits++;
            i++;
        }
    }
    while (fp_digits < 2) {
        fp *= 10;
        fp_digits++;
    }

    long long bp = ip * 100 + fp;
    bp *= sign;
    if (bp < 0) bp = 0;
    if (bp > 10000) bp = 10000;
    return (int)bp;
}

static int compute_display_bp(int64 S, int D) {
    // 정확도(%) = S/D, 표시 basis point = round(정확도 * 100) = round(100*S/D)
    int64 num = 100LL * S;
    int64 bp = (num + (D / 2)) / D; // half-up
    if (bp < 0) bp = 0;
    if (bp > 10000) bp = 10000;
    return (int)bp;
}

static void print_bp_as_percent(int bp) {
    int a = bp / 100;
    int b = bp % 100;
    cout << a << '.';
    if (b < 10) cout << '0';
    cout << b << '%';
}

static bool apply_ge(int64 a, int64 b, int64& lo, int64& hi) {
    // a*E >= b
    if (a == 0) return (0 >= b);
    if (a > 0) lo = std::max(lo, ceil_div_any(b, a));
    else hi = std::min(hi, floor_div_any(b, a));
    return (lo <= hi);
}
static bool apply_le(int64 a, int64 b, int64& lo, int64& hi) {
    // a*E <= b
    if (a == 0) return (0 <= b);
    if (a > 0) hi = std::min(hi, floor_div_any(b, a));
    else lo = std::max(lo, ceil_div_any(b, a));
    return (lo <= hi);
}
static bool apply_gt(int64 a, int64 b, int64& lo, int64& hi) {
    // a*E > b
    if (a == 0) return (0 > b);
    if (a > 0) lo = std::max(lo, floor_div_any(b, a) + 1);
    else hi = std::min(hi, ceil_div_any(b, a) - 1);
    return (lo <= hi);
}

static bool compute_E_range(
    int mode,
    int N_tiles,      // 입력 타일 수(= P+Y+R+M)
    int target_bp,    // 목표 표시값
    int64 S0,         // E 제외 점수: 100P + 75Y + 40R
    int64& Emin,
    int64& Emax
) {
    // 표시 조건: (2T-1)D <= 200S < (2T+1)D,  S=S0+20E, D=N+E
    const int64 E_CAP = 10000000LL;

    int64 lo = 0;
    int64 hi = E_CAP;

    const int64 T = (int64)target_bp;
    const int64 N = (int64)N_tiles;

    // (2T-1)(N+E) <= 200(S0+20E)
    const int64 a1 = 4001LL - 2LL * T;
    const int64 b1 = (2LL * T - 1LL) * N - 200LL * S0;
    if (!apply_ge(a1, b1, lo, hi)) return false;

    // 200(S0+20E) < (2T+1)(N+E)
    const int64 a2 = 2LL * T - 3999LL;
    const int64 b2 = 200LL * S0 - (2LL * T + 1LL) * N;
    if (!apply_gt(a2, b2, lo, hi)) return false;

    // 모드 1: 실제값 >= 목표
    // 100(S0+20E) >= T(N+E)  <=> (T-2000)E <= 100S0 - TN
    if (mode == 1) {
        const int64 a3 = T - 2000LL;
        const int64 b3 = 100LL * S0 - T * N;
        if (!apply_le(a3, b3, lo, hi)) return false;
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
    int64 Emin, Ecur; // Ecur는 현재 출력 후보, Emin까지 내려가며 생성
};

struct NodeBetter {
    // (R+Y 고정된 그룹 내부 우선순위):
    // Miss 최대 → Early!! 최대 → Perfect 최대 → Yellow 최대
    bool operator()(const Node& a, const Node& b) const {
        if (a.M != b.M) return a.M < b.M;       // Miss 큰 것이 우선
        if (a.Ecur != b.Ecur) return a.Ecur < b.Ecur; // Early!! 큰 것이 우선
        if (a.P != b.P) return a.P < b.P;       // Perfect 큰 것이 우선
        if (a.Y != b.Y) return a.Y < b.Y;       // Yellow 큰 것이 우선
        return a.R > b.R;
    }
};

static bool is_valid_solution(int mode, int N_tiles, int target_bp, const Node& node, int64 E) {
    int64 S = node.S0 + 20LL * E;
    int D = (int)(N_tiles + (int)E);
    int disp = compute_display_bp(S, D);
    if (disp != target_bp) return false;
    if (mode == 1) {
        if (100LL * S < (int64)target_bp * (int64)D) return false;
    }
    return true;
}

static std::vector<Solution> generate_solutions(int mode, int N_tiles, int target_bp, int K, bool noMiss) {
    std::vector<Solution> out;
    if (N_tiles <= 0 || K <= 0) return out;

    // 최우선: (Red+Yellow) 최소
    for (int k = 0; k <= N_tiles; ++k) { // k = R + Y
        std::priority_queue<Node, std::vector<Node>, NodeBetter> pq;

        for (int Y = k; Y >= 0; --Y) {
            int R = k - Y;

            int Mmin = 0;
            int Mmax = noMiss ? 0 : (N_tiles - k);
            for (int M = Mmin; M <= Mmax; ++M) {
                int P = N_tiles - k - M;
                if (P < 0) continue;

                int64 S0 = 100LL * P + 75LL * Y + 40LL * R;

                int64 Emin = 0, Emax = 0;
                if (!compute_E_range(mode, N_tiles, target_bp, S0, Emin, Emax)) continue;

                Node node;
                node.Y = Y;
                node.R = R;
                node.M = M;
                node.P = P;
                node.S0 = S0;
                node.Emin = Emin;
                node.Ecur = Emax;

                while (node.Ecur >= node.Emin && !is_valid_solution(mode, N_tiles, target_bp, node, node.Ecur)) {
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
            sol.disp_bp = compute_display_bp(S, D);

            out.push_back(sol);

            node.Ecur = E - 1;
            while (node.Ecur >= node.Emin && !is_valid_solution(mode, N_tiles, target_bp, node, node.Ecur)) {
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

    cout << "얼불춤 절대정확도 판정 조합 생성기\n\n";
    cout << "표시 반올림: 소수 둘째 자리(셋째 자리에서 반올림)\n";
    cout << "모드 0: 표기만 맞으면 됨, 모드 1: 표기도 맞고 실제값도 목표 이상\n";
    cout << "옵션: 입력 끝에 nomiss를 붙이면 Miss=0인 조합만 출력함\n\n";

    while (true) {
        int K;
        cout << "K개의 결과 표시 (0 입력시 프로그램 종료): " << std::flush;
        if (!(cin >> K)) return 0;
        if (K == 0) break;

        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        cout << "\n입력: 모드  타일수N  목표정확도  [nomiss] : " << std::flush;

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

        int target_bp = parse_target_bp(targetStr);
        auto sols = generate_solutions(mode, N_tiles, target_bp, K, noMiss);

        cout << "\n목표 정확도 = ";
        print_bp_as_percent(target_bp);
        cout << ", 타일수 N = " << N_tiles << ", mode = " << mode;
        if (noMiss) cout << ", 옵션 = nomiss";
        cout << "\n";

        if (sols.empty()) {
            cout << "No solutions found.\n\n";
            continue;
        }

        for (int i = 0; i < (int)sols.size(); ++i) {
            const auto& s = sols[i];
            cout << "\n";
            cout << "---- #" << (i + 1) << " ----\n";
            cout << "Early!! : " << s.early20
                 << ", Red : " << s.red40
                 << ", Yellow : " << s.yellow75
                 << ", Perfect : " << s.perfect100
                 << ", Miss : " << s.miss0 << "\n";
            cout << "Display : ";
            print_bp_as_percent(s.disp_bp);
            cout << "  (총 판정 개수=" << s.denom << ", 가중치 합=" << s.totalScore << ")\n";
        }

        cout << "\n";
    }

    cout << "Exit.\n";
    return 0;
}
