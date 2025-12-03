// main.cpp
// Persistent stock simulator (final fixed version)
// Build: g++ -std=gnu++17 main.cpp -o sim.exe
// Run:   ./sim.exe --dataset dataset_indian_15days --run-name myrun

#include <bits/stdc++.h>
using namespace std;
using ll = long long;

vector<string> segments = {"M","D","C"};
int segIndex(const string &s) { for (size_t i=0;i<segments.size();++i) if (segments[i]==s) return (int)i; return -1; }
void advanceSeg(int &day, string &seg) { int idx = segIndex(seg); if (idx<0) { seg="M"; return; } if (idx+1 < (int)segments.size()) seg = segments[idx+1]; else { seg = segments[0]; day += 1; } }
pair<int,string> getPrevSegment(int curDay, const string &curSeg) {
    int idx = segIndex(curSeg);
    if (idx < 0) return {-1, ""};
    if (idx > 0) return {curDay, segments[idx-1]};
    return {curDay-1, segments.back()};
}

static string now_iso() {
    using namespace std::chrono;
    auto t = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(t);
    auto ms = chrono::duration_cast<chrono::milliseconds>(t.time_since_epoch()) % 1000;
    std::tm tm{};
#if defined(_MSC_VER)
    gmtime_s(&tm, &tt);
#elif (defined(__unix__) || defined(__APPLE__) || defined(_POSIX_VERSION))
    gmtime_r(&tt, &tm);
#else
    if (std::tm* p = std::gmtime(&tt)) tm = *p;
#endif
    char buf[64]; strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    char out[80]; snprintf(out, sizeof(out), "%s.%03lldZ", buf, (long long)ms.count());
    return string(out);
}

void printWrapped(const string &text, int width=80, const string &indent="") {
    stringstream ss(text);
    string line;
    while (getline(ss, line, '\n')) {
        size_t start = 0;
        while (start < line.length()) {
            if (line.length() - start <= (size_t)width) {
                cout << indent << line.substr(start) << "\n";
                break;
            }
            size_t end = start + width;
            size_t space = line.rfind(' ', end);
            if (space == string::npos || space < start) space = end;
            cout << indent << line.substr(start, space - start) << "\n";
            start = space + 1;
        }
    }
}

// ----- CSV helpers -----
vector<string> splitCSV(const string &line) {
    vector<string> out; string cur; bool inq=false;
    for (size_t i=0;i<line.size();++i) {
        char c=line[i];
        if (c=='\"') { inq = !inq; cur.push_back(c); continue; }
        if (c==',' && !inq) { out.push_back(cur); cur.clear(); } else cur.push_back(c);
    }
    out.push_back(cur); return out;
}
string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a==string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a,b-a+1);
}
string unquote(const string &s) {
    if (s.size()>=2 && s.front()=='\"' && s.back()=='\"') return s.substr(1,s.size()-2);
    return s;
}

// ----- Data structures -----
struct StockRow {
    int day;
    string seg;
    int id;
    string symbol;
    string company;
    string sector;
    double open;
    double high;
    double low;
    double close;     // segment_close -> current price
    double change_pct;
    int volume;
    string short_note;
    StockRow(): day(0), seg(""), id(0), symbol(""), company(""), sector(""),
                open(0), high(0), low(0), close(0), change_pct(0), volume(0), short_note("") {}
    // Expected indices (matching dataset):
    // 0:day,1:segment,2:stock_id,3:symbol,4:company,5:sector,
    // 6:segment_open,7:segment_high,8:segment_low,9:segment_close,
    // 10:change_pct,11:volume,...
    void fromCols(const vector<string>& c) {
        if (c.size() < 10) throw runtime_error("row has insufficient cols");
        day = stoi(trim(unquote(c[0])));
        seg = trim(unquote(c[1]));
        id = stoi(trim(unquote(c[2])));
        symbol = trim(unquote(c[3]));
        company = unquote(trim(c[4]));
        sector = trim(unquote(c[5]));
        try { open = stod(trim(unquote(c[6]))); } catch(...) { open = 0.0; }
        try { high = stod(trim(unquote(c[7]))); } catch(...) { high = 0.0; }
        try { low  = stod(trim(unquote(c[8]))); } catch(...) { low = 0.0; }
        try { close = stod(trim(unquote(c[9]))); } catch(...) { close = 0.0; }
        try { change_pct = stod(trim(unquote(c[10]))); } catch(...) { change_pct = 0.0; }
        try { volume = stoi(trim(unquote(c[11]))); } catch(...) { volume = 0; }
        if (c.size()>15) short_note = unquote(trim(c[15])); else short_note="";
    }
};

struct PriceInfo {
    double close=0;
    double change=0;
    int volume=0;
    int day=0;
    string seg="";
    double high=0;
    double low=0;
    double open=0;
};

// ----- Market -----
class Market {
    string datasetPath;
    vector<StockRow> rows; // current segment rows
    map<tuple<int,string,int>, PriceInfo> priceMap; // (day,seg,id) -> priceinfo
    set<int> allStockIds;
public:
    Market(const string &p): datasetPath(p) { loadPriceMap(); }
    void loadPriceMap() {
        priceMap.clear();
        allStockIds.clear();
        string fname = datasetPath + "/stock_history.csv";
        ifstream f(fname);
        if (!f.is_open()) return;
        string hdr; getline(f,hdr);
        string line;
        while (getline(f,line)) {
            if (line.empty()) continue;
            auto c = splitCSV(line);
            if (c.size() < 10) continue;
            try {
                int day = stoi(trim(unquote(c[0])));
                string seg = trim(unquote(c[1]));
                int sid = stoi(trim(unquote(c[2])));
                double open=0, high=0, low=0, close=0, change=0; int vol=0;
                if (c.size()>6) try{ open = stod(trim(unquote(c[6])));}catch(...){} 
                if (c.size()>7) try{ high = stod(trim(unquote(c[7])));}catch(...){} 
                if (c.size()>8) try{ low  = stod(trim(unquote(c[8])));}catch(...){} 
                if (c.size()>9) try{ close = stod(trim(unquote(c[9])));}catch(...){} 
                if (c.size()>10) try{ change = stod(trim(unquote(c[10])));}catch(...){} 
                if (c.size()>11) try{ vol = stoi(trim(unquote(c[11])));}catch(...){} 
                priceMap[{day,seg,sid}] = PriceInfo{close,change,vol,day,seg,high,low,open};
                allStockIds.insert(sid);
            } catch(...) { continue; }
        }
    }
    void loadSegment(int day, const string &seg) {
        rows.clear();
        string fname = datasetPath + "/stock_history.csv";
        ifstream f(fname);
        if (!f.is_open()) return;
        string hdr; getline(f,hdr);
        string line;
        while (getline(f,line)) {
            if (line.empty()) continue;
            auto c = splitCSV(line);
            if (c.size() < 4) continue;
            try {
                int d = stoi(trim(unquote(c[0])));
                string s = trim(unquote(c[1]));
                if (d==day && s==seg) {
                    StockRow sr; sr.fromCols(c); rows.push_back(sr);
                }
            } catch(...) { continue; }
        }
        sort(rows.begin(), rows.end(), [](const StockRow &a, const StockRow &b){ return a.id < b.id; });
    }
    const vector<StockRow>& getRows() const { return rows; }
    vector<int> getAllStockIds() const {
        vector<int> out(allStockIds.begin(), allStockIds.end());
        sort(out.begin(), out.end());
        return out;
    }
    // get price for exact day+seg; returns {found, PriceInfo}
    pair<bool, PriceInfo> getPriceExact(int day, const string &seg, int sid) const {
        auto it = priceMap.find({day,seg,sid});
        if (it==priceMap.end()) return {false, PriceInfo()};
        return {true, it->second};
    }
    // get latest available price up to day/seg (inclusive): search backward
    pair<bool, PriceInfo> getLatestPriceUpTo(int day, const string &seg, int sid) const {
        int segIdx = segIndex(seg);
        if (segIdx < 0) segIdx = (int)segments.size()-1;
        for (int d = day; d >= 1; --d) {
            for (int si = segIdx; si >= 0; --si) {
                string s = segments[si];
                auto it = priceMap.find({d,s,sid});
                if (it != priceMap.end()) return {true, it->second};
            }
            segIdx = (int)segments.size()-1;
        }
        return {false, PriceInfo()};
    }
    // last N movements for a stock (descending by day/seg) - original
    vector<PriceInfo> getLastN(int sid, int N) const {
        vector<PriceInfo> out;
        for (auto &kv : priceMap)
            if (get<2>(kv.first) == sid)
                out.push_back(kv.second);
        sort(out.begin(), out.end(), [](const PriceInfo &a, const PriceInfo &b){
            if (a.day != b.day) return a.day > b.day;
            int ai = (a.seg=="M")?0:(a.seg=="D")?1:2;
            int bi = (b.seg=="M")?0:(b.seg=="D")?1:2;
            return ai > bi;
        });
        if ((int)out.size() > N) out.resize(N);
        return out;
    }
    // NEW: last N movements up to a given (day,seg)
    vector<PriceInfo> getLastNUpTo(int sid, int N, int curDay, const string &curSeg) const {
        vector<PriceInfo> all;
        for (auto &kv : priceMap)
            if (get<2>(kv.first) == sid)
                all.push_back(kv.second);
        sort(all.begin(), all.end(), [](const PriceInfo &a, const PriceInfo &b){
            if (a.day != b.day) return a.day > b.day;
            int ai = (a.seg=="M")?0:(a.seg=="D")?1:2;
            int bi = (b.seg=="M")?0:(b.seg=="D")?1:2;
            return ai > bi;
        });
        vector<PriceInfo> out;
        int curIdx = segIndex(curSeg);
        for (const auto &p : all) {
            if (p.day > curDay) continue;
            if (p.day == curDay) {
                int pi = segIndex(p.seg);
                if (pi > curIdx) continue;
            }
            out.push_back(p);
            if ((int)out.size() >= N) break;
        }
        return out;
    }
    const StockRow* findById(int id) const {
        for (auto &r : rows) if (r.id == id) return &r;
        return nullptr;
    }
};

// ----- Portfolio & Persistence -----
struct Position {
    long long qty = 0;
    double avg_price = 0.0;
    double realized_pl = 0.0;
    int last_mod_day = -1;
    string last_mod_seg = "";
};

class Portfolio {
    double cash;
    unordered_map<int, Position> pos;
public:
    Portfolio(double start=100000.0): cash(start) {}
    double getCash() const { return cash; }
    void setCash(double c) { cash = c; }
    const unordered_map<int, Position>& getPositions() const { return pos; }
    long long getQty(int sid) const { auto it = pos.find(sid); if (it==pos.end()) return 0; return it->second.qty; }
    double getAvgPrice(int sid) const { auto it = pos.find(sid); if (it==pos.end()) return 0.0; return it->second.avg_price; }

    // buy at current price
    bool buy(int stockId, long long qty, double price, int curDay, const string &curSeg, string &err) {
        if (qty <= 0) { err = "qty>0"; return false; }
        double cost = (double)qty * price;
        if (cost > cash + 1e-9) { err = "insufficient cash"; return false; }
        Position &p = pos[stockId];
        double prevCost = p.avg_price * (double)max(0LL,p.qty);
        double newCost = prevCost + cost;
        p.qty += qty;
        p.avg_price = (p.qty>0) ? newCost / (double)p.qty : 0.0;
        p.last_mod_day = curDay; p.last_mod_seg = curSeg;
        cash -= cost;
        return true;
    }

    // (Old sell kept but unused)
    bool sell(int stockId, long long qty, double price, int curDay, const string &curSeg, string &err) {
        if (qty <= 0) { err = "qty>0"; return false; }
        auto it = pos.find(stockId);
        if (it == pos.end() || it->second.qty <= 0) { err = "no long position to sell"; return false; }
        Position &p = it->second;
        long long closeQty = min(p.qty, qty);
        double proceeds = (double)closeQty * price;
        double rpl = (price - p.avg_price) * (double)closeQty;
        p.realized_pl += rpl;
        p.qty -= closeQty;
        cash += proceeds;
        p.last_mod_day = curDay; p.last_mod_seg = curSeg;
        return true;
    }

    // NEW: partial/full exit by qty
    bool exitPositionPartial(int stockId, long long qty,
                             double marketPrice, int curDay,
                             const string &curSeg, string &err) {
        if (qty <= 0) { err = "qty must be > 0"; return false; }
        auto it = pos.find(stockId);
        if (it == pos.end() || it->second.qty <= 0) {
            err = "no position";
            return false;
        }
        Position &p = it->second;
        if (qty > p.qty) {
            err = "exit qty exceeds holding";
            return false;
        }
        double proceeds = marketPrice * (double)qty;
        double rpl = (marketPrice - p.avg_price) * (double)qty;
        p.qty -= qty;
        p.realized_pl += rpl;
        cash += proceeds;
        p.last_mod_day = curDay;
        p.last_mod_seg = curSeg;
        if (p.qty == 0) {
            p.avg_price = 0.0;
            p.last_mod_day = -1;
            p.last_mod_seg = "";
        }
        return true;
    }

    // old full exit kept in case of future use
    bool exitPosition(int stockId, double marketPrice, int curDay, const string &curSeg, string &err) {
        auto it = pos.find(stockId);
        if (it==pos.end() || it->second.qty==0) { err="no position"; return false; }
        Position &p = it->second;
        double proceeds = (double)p.qty * marketPrice;
        double rpl = (marketPrice - p.avg_price) * (double)p.qty;
        p.realized_pl += rpl;
        cash += proceeds;
        p.qty = 0; p.avg_price = 0.0; p.last_mod_day=-1; p.last_mod_seg="";
        return true;
    }

    void computeMetrics(const Market &market, int curDay, const string &curSeg,
                        double &equity, double &netPosValue, double &maxConcPct,
                        double &unrealPct, double &realizedPL) const {
        double longV=0.0, longCost=0.0, realPL=0.0;
        for (auto &kv: pos) {
            int sid = kv.first; const Position &p = kv.second;
            double curPrice = 0.0;
            auto pp = market.getPriceExact(curDay, curSeg, sid);
            if (pp.first) curPrice = pp.second.close;
            else {
                auto lp = market.getLatestPriceUpTo(curDay, curSeg, sid);
                if (lp.first) curPrice = lp.second.close;
            }
            if (p.qty > 0) { longV += curPrice * (double)p.qty; longCost += p.avg_price * (double)p.qty; }
            realPL += p.realized_pl;
        }
        netPosValue = longV;
        equity = cash + netPosValue;
        double maxPos = 0.0;
        for (auto &kv: pos) {
            int sid = kv.first; const Position &p = kv.second;
            double curPrice = 0.0;
            auto lp = market.getLatestPriceUpTo(curDay, curSeg, sid);
            if (lp.first) curPrice = lp.second.close;
            double absVal = curPrice * (double)llabs(p.qty);
            if (absVal > maxPos) maxPos = absVal;
        }
        maxConcPct = (equity>0.0) ? (maxPos / equity * 100.0) : 0.0;
        double longPL = (longCost>0.0) ? (longV - longCost) : 0.0;
        double totalCost = longCost;
        unrealPct = (totalCost>0.0) ? (longPL / totalCost * 100.0) : 0.0;
        realizedPL = realPL;
    }

    bool loadState(const string &path) {
        string f = path + "/portfolio_state.csv";
        ifstream in(f);
        if (!in.is_open()) return false;
        string line;
        if (!getline(in,line)) return false;
        try { cash = stod(trim(line)); } catch(...) { cash = 100000.0; }
        while (getline(in,line)) {
            if (line.empty()) continue;
            auto c = splitCSV(line);
            if (c.size() < 6) continue;
            try {
                int sid = stoi(trim(unquote(c[0])));
                Position p;
                p.qty = stoll(trim(unquote(c[1])));
                p.avg_price = stod(trim(unquote(c[2])));
                p.realized_pl = stod(trim(unquote(c[3])));
                p.last_mod_day = stoi(trim(unquote(c[4])));
                p.last_mod_seg = unquote(trim(c[5]));
                pos[sid] = p;
            } catch(...) { continue; }
        }
        return true;
    }
    bool saveState(const string &path) const {
        string f = path + "/portfolio_state.csv";
        ofstream out(f, ios::trunc);
        if (!out.is_open()) return false;
        out << fixed << setprecision(2) << cash << "\n";
        for (auto &kv: pos) {
            out << kv.first << "," << kv.second.qty << ","
                << fixed << setprecision(4) << kv.second.avg_price << ","
                << fixed << setprecision(4) << kv.second.realized_pl << ","
                << kv.second.last_mod_day << ",\""
                << kv.second.last_mod_seg << "\"\n";
        }
        return true;
    }
};

// ----- Logger -----
class SimpleLogger {
    string runPath;
public:
    SimpleLogger(const string &p): runPath(p) {}
    void ensureFiles() {
#if defined(_WIN32) || defined(_WIN64)
        string cmd = string("mkdir \"") + runPath + "\" >nul 2>nul";
        system(cmd.c_str());
#else
        string cmd = string("mkdir -p \"") + runPath + "\"";
        system(cmd.c_str());
#endif
        string t = runPath + "/trades.csv";
        if (!ifstream(t)) {
            ofstream f(t);
            f << "run_id,trade_id,day,segment,timestamp_iso,stock_id,symbol,action,qty,price,trade_value,cash_before,cash_after,position_qty_after,position_avg_price_after,realized_pl\n";
        }
        string p = runPath + "/portfolio_snapshots.csv";
        if (!ifstream(p)) {
            ofstream f(p);
            f << "run_id,day,segment,timestamp_iso,total_value,cash,value_positions,max_concentration_pct,unrealized_pl_pct,realized_pl\n";
        }
        string v = runPath + "/views.csv";
        if (!ifstream(v)) {
            ofstream f(v);
            f << "run_id,day,segment,timestamp_iso,stock_id,symbol,action\n";
        }
    }
    ll nextTradeId() {
        ll last = 0;
        string path = runPath + "/trades.csv";
        ifstream f(path);
        if (!f.is_open()) return 1;
        string hdr; getline(f,hdr);
        string line;
        while (getline(f,line)) {
            if (line.empty()) continue;
            auto c = splitCSV(line);
            if (c.size()>=2) {
                try { last = max(last, stoll(trim(unquote(c[1])))); } catch(...) {}
            }
        }
        return last+1;
    }
    void appendTrade(ll run_id, ll trade_id, int day, const string &seg, const string &ts,
                     int stock_id, const string &symbol, const string &action, ll qty,
                     double price, double trade_value, double cash_before, double cash_after,
                     long long pos_qty_after, double pos_avg_price_after, double realized_pl) {
        ofstream f(runPath + "/trades.csv", ios::app);
        f << run_id << "," << trade_id << "," << day << "," << seg << ",\"" << ts << "\"," 
          << stock_id << ",\"" << symbol << "\"," << "\"" << action << "\"," << qty << ","
          << fixed << setprecision(2) << price << "," << trade_value << ","
          << cash_before << "," << fixed << setprecision(2) << cash_after << ","
          << pos_qty_after << "," << pos_avg_price_after << ","
          << fixed << setprecision(2) << realized_pl << "\n";
    }
    void appendSnapshot(ll run_id, int day, const string &seg, const string &ts,
                        double total, double cash, double value_positions,
                        double maxc, double unreal, double realized) {
        ofstream f(runPath + "/portfolio_snapshots.csv", ios::app);
        f << run_id << "," << day << "," << seg << ",\"" << ts << "\"," 
          << fixed << setprecision(2) << total << "," << fixed << setprecision(2) << cash << ","
          << fixed << setprecision(2) << value_positions << "," << fixed << setprecision(2) << maxc << ","
          << fixed << setprecision(2) << unreal << "," << fixed << setprecision(2) << realized << "\n";
    }
    void appendView(ll run_id, int day, const string &seg, const string &ts,
                    int stock_id, const string &symbol) {
        ofstream f(runPath + "/views.csv", ios::app);
        f << run_id << "," << day << "," << seg << ",\"" << ts << "\"," 
          << stock_id << ",\"" << symbol << "\",\"view\"\n";
    }
};

// ----- News loaders (UPDATED GLOBAL NEWS) -----
map<int, vector<string>> loadGlobalNews(const string &dataset) {
    map<int, vector<string>> out;
    string f = dataset + "/global_news.csv";
    ifstream in(f);
    if (!in.is_open()) return out;
    string hdr; getline(in,hdr);
    string line;
    while (getline(in,line)) {
        if (line.empty()) continue;
        auto c = splitCSV(line);
        if (c.size()>=3) {
            try {
                int day = stoi(trim(unquote(c[0])));
                string det = unquote(trim(c[2]));
                out[day].push_back(det);
            } catch(...) {}
        }
    }
    return out;
}

map<pair<int,string>,string> loadSegmentNews(const string &dataset) {
    map<pair<int,string>,string> out;
    string f = dataset + "/segment_news.csv";
    ifstream in(f);
    if (!in.is_open()) return out;
    string hdr; getline(in,hdr);
    string line;
    while (getline(in,line)) {
        if (line.empty()) continue;
        auto c = splitCSV(line);
        if (c.size()>=4) {
            try {
                int day = stoi(trim(unquote(c[0])));
                string seg = trim(unquote(c[1]));
                string det = unquote(trim(c[3]));
                out[{day,seg}] = det;
            } catch(...) {}
        }
    }
    return out;
}
map<tuple<int,string,int>,string> loadStockNews(const string &dataset) {
    map<tuple<int,string,int>,string> out;
    string f = dataset + "/stock_daily_news.csv";
    ifstream in(f);
    if (!in.is_open()) return out;
    string hdr; getline(in,hdr);
    string line;
    while (getline(in,line)) {
        if (line.empty()) continue;
        auto c = splitCSV(line);
        if (c.size()>=5) {
            try {
                int day = stoi(trim(unquote(c[0])));
                string seg = trim(unquote(c[1]));
                int sid = stoi(trim(unquote(c[2])));
                string brief = unquote(trim(c[4]));
                out[{day,seg,sid}] = brief;
            } catch(...) {}
        }
    }
    return out;
}

// ----- Main -----
int main(int argc, char** argv) {
    cout << "StockSim (persistent terminal) - final\n";
    cout << "StockSim (persistent terminal) - final\n";

    string dataset = "dataset_indian_15days";
    string runNameArg = "";
    for (int i=1;i<argc;++i) {
        string a = argv[i];
        if (a=="--dataset" && i+1<argc) dataset = argv[++i];
        else if (a=="--run-name" && i+1<argc) runNameArg = argv[++i];
    }

    ifstream test(dataset + "/stock_history.csv");
    if (!test.is_open()) {
        cerr << "ERROR: dataset does not contain stock_history.csv at path: " << dataset << "\n";
        return 1;
    }
    test.close();

    string runsRoot = dataset + "/runs";
#if defined(_WIN32) || defined(_WIN64)
    system((string("mkdir \"") + runsRoot + "\" >nul 2>nul").c_str());
#else
    system((string("mkdir -p \"") + runsRoot + "\"").c_str());
#endif

    string runName;
    if (!runNameArg.empty()) runName = runNameArg;
    else {
        cout << "Resume existing run? (y/N): ";
        string ans;
        if (!getline(cin, ans)) ans="";
        if (!ans.empty() && (ans[0]=='y' || ans[0]=='Y')) {
            cout << "Enter run name to resume: ";
            if (!getline(cin, runName)) runName="";
        }
        if (runName.empty()) {
            cout << "Enter new run name (no spaces): ";
            if (!getline(cin, runName)) return 0;
            if (runName.empty()) runName = "run_default";
        }
    }

    string runPath = runsRoot + "/" + runName;
#if defined(_WIN32) || defined(_WIN64)
    system((string("mkdir \"") + runPath + "\" >nul 2>nul").c_str());
#else
    system((string("mkdir -p \"") + runPath + "\"").c_str());
#endif

    Market market(dataset);
    int curDay = 1; string curSeg = "M";
    market.loadSegment(curDay, curSeg);

    auto globalNews = loadGlobalNews(dataset);
    auto segNews = loadSegmentNews(dataset);
    auto stockNews = loadStockNews(dataset);

    SimpleLogger logger(runPath);
    logger.ensureFiles();
    Portfolio portfolio(100000.0);
    bool loaded = portfolio.loadState(runPath);
    if (loaded) cout << "Resumed portfolio state from " << runPath << "/portfolio_state.csv\n";
    else cout << "Starting new portfolio with cash 100000.00\n";

    ll run_id = (ll)time(nullptr);
    ll nextTid = logger.nextTradeId();

    bool running = true;
    while (running) {
        market.loadSegment(curDay, curSeg);
        auto rows = market.getRows();

        cout << "\n=== Day " << curDay << " Segment " << curSeg << " ===\n";
        if (curSeg == "M" && globalNews.count(curDay)) {
            cout << "=== Morning News ===\n";
            int idx = 1;
            for (const auto &item : globalNews[curDay]) {
                cout << idx++ << ") ";
                printWrapped(item, 76, "");
            }
            cout << "\n";
        }
        if (segNews.count({curDay,curSeg})) {
            cout << "=== Segment note ===\n";
            printWrapped(segNews[{curDay,curSeg}], 80, "");
            cout << "\n";
        }

        double total, net, maxc, unreal, realpl;
        portfolio.computeMetrics(market, curDay, curSeg, total, net, maxc, unreal, realpl);

        // Portfolio table (unchanged: still shows company here)
        cout << left << setw(4) << "ID" << setw(8) << "Symbol" << setw(22) << "Company" << setw(12) << "Sector"
             << right << setw(8) << "Qty" << setw(12) << "AvgPrice" << setw(12) << "CurPrice"
             << setw(12) << "Invested" << setw(12) << "CurValue" << setw(12) << "UnrealPL" << "\n";
        cout << string(110,'-') << "\n";
        if (portfolio.getPositions().empty()) {
            cout << "(no holdings)\n";
        } else {
            for (auto &kv: portfolio.getPositions()) {
                int sid = kv.first; const Position &p = kv.second;
                const StockRow* sr = market.findById(sid);
                string sym = sr? sr->symbol : to_string(sid);
                string comp = sr? sr->company : "";
                string sec = sr? sr->sector : "";
                double curPrice = 0.0;
                auto pp = market.getPriceExact(curDay, curSeg, sid);
                if (pp.first) curPrice = pp.second.close;
                else {
                    auto lp = market.getLatestPriceUpTo(curDay, curSeg, sid);
                    if (lp.first) curPrice = lp.second.close;
                }
                double invested = (double)p.qty * p.avg_price;
                double curVal = (double)p.qty * curPrice;
                double unrealized = curVal - invested;
                cout << left << setw(4) << sid << setw(8) << sym << setw(22) << comp << setw(12) << sec
                     << right << setw(8) << p.qty << setw(12) << fixed << setprecision(2) << p.avg_price
                     << setw(12) << curPrice << setw(12) << invested << setw(12) << curVal << setw(12) << unrealized << "\n";
            }
        }
        cout << "\nEquity: " << fixed << setprecision(2) << total
             << "  Cash: " << portfolio.getCash()
             << "  Unreal%: " << unreal
             << "  RealizedPL: " << realpl << "\n\n";

        // Menu
        cout << "Menu:\n"
             << "1) List stocks\n"
             << "2) View stock (history + brief)\n"
             << "3) Buy\n"
             << "4) Exit position (partial/full)\n"
             << "5) Save snapshot\n"
             << "6) Advance to next segment\n"
             << "7) Quit (save state)\n"
             << "Choose: ";
        int ch; if (!(cin >> ch)) break;

        if (ch == 1) {
            cout << left << setw(4) << "ID" << setw(8) << "Symbol" << setw(22) << "Company" << setw(12) << "Sector"
                 << right << setw(10) << "CurPrice" << setw(10) << "PrevClose"
                 << setw(10) << "DayHigh" << setw(10) << "DayLow"
                 << setw(10) << "Delta" << setw(8) << "D%" << setw(8) << "Vol" << "\n";
            cout << string(110,'-') << "\n";
            if (rows.empty()) {
                cout << "(No data in this segment)\n";
            } else {
                auto prevSegInfo = getPrevSegment(curDay, curSeg);
                int prevDayRef = prevSegInfo.first;
                string prevSegRef = prevSegInfo.second;
                for (auto &r : rows) {
                    double curP = r.close;
                    double prevClose = 0.0;
                    bool havePrev = false;
                    if (prevDayRef >= 1) {
                        auto pp = market.getPriceExact(prevDayRef, prevSegRef, r.id);
                        if (pp.first) { prevClose = pp.second.close; havePrev = true; }
                    }
                    double deltaNum = havePrev ? (curP - prevClose) : 0.0;
                    double deltaPct = (havePrev && prevClose != 0.0) ? (deltaNum / prevClose * 100.0) : 0.0;
                    ostringstream ossPrev, ossDelta, ossPct;
                    if (havePrev) {
                        ossPrev << fixed << setprecision(2) << prevClose;
                        ossDelta << fixed << setprecision(2) << deltaNum;
                        ossPct << fixed << setprecision(2) << deltaPct;
                    }
                    string prevStr = havePrev ? ossPrev.str() : string("-");
                    string deltaStr = havePrev ? ossDelta.str() : string("-");
                    string dPctStr = havePrev ? ossPct.str() : string("-");
                    cout << left << setw(4) << r.id << setw(8) << r.symbol << setw(22) << r.company << setw(12) << r.sector
                         << right << setw(10) << fixed << setprecision(2) << curP
                         << setw(10) << prevStr
                         << setw(10) << fixed << setprecision(2) << r.high
                         << setw(10) << fixed << setprecision(2) << r.low
                         << setw(10) << deltaStr
                         << setw(8) << dPctStr
                         << setw(8) << r.volume << "\n";
                }
            }
        } else if (ch == 2) {
            cout << "Enter stock id: "; 
            int id; cin >> id;

            logger.appendView(run_id, curDay, curSeg, now_iso(), id, to_string(id));
            const StockRow* maybe = market.findById(id);
            if (maybe) {
                // Removed company name here as requested
                cout << "Stock: " << maybe->symbol << " (" << maybe->sector << ")\n";
                cout << "Short: ";
                printWrapped(maybe->short_note, 80, "");
                cout << "\n";
            } else {
                auto lp = market.getLatestPriceUpTo(curDay, curSeg, id);
                if (!lp.first) {
                    cout << "Stock id not found in dataset\n";
                    goto AFTER_VIEW;
                } else {
                    cout << "Stock id " << id << " (no current segment row) - using latest available info\n";
                }
            }

            // New: show same-style list table but only for this stock, last 5 segments up to current day/seg
            {
                auto hist = market.getLastNUpTo(id, 5, curDay, curSeg);
                if (hist.empty()) {
                    cout << "  (no history found up to current segment)\n";
                } else {
                    cout << left << setw(4) << "ID" << setw(8) << "Symbol" << setw(12) << "Sector"
                         << right << setw(10) << "CurPrice" << setw(10) << "PrevClose"
                         << setw(10) << "DayHigh" << setw(10) << "DayLow"
                         << setw(10) << "Delta" << setw(8) << "D%" << setw(8) << "Vol" << "\n";
                    cout << string(110,'-') << "\n";

                    string sym = maybe ? maybe->symbol : to_string(id);
                    string sec = maybe ? maybe->sector : "";

                    for (const auto &h : hist) {
                        double curP = h.close;
                        double prevClose = 0.0;
                        bool havePrev = false;
                        auto prevSegInfo = getPrevSegment(h.day, h.seg);
                        if (prevSegInfo.first >= 1) {
                            auto pp = market.getPriceExact(prevSegInfo.first, prevSegInfo.second, id);
                            if (pp.first) { prevClose = pp.second.close; havePrev = true; }
                        }
                        double deltaNum = havePrev ? (curP - prevClose) : 0.0;
                        double deltaPct = (havePrev && prevClose != 0.0) ? (deltaNum / prevClose * 100.0) : 0.0;
                        ostringstream ossPrev, ossDelta, ossPct;
                        if (havePrev) {
                            ossPrev << fixed << setprecision(2) << prevClose;
                            ossDelta << fixed << setprecision(2) << deltaNum;
                            ossPct << fixed << setprecision(2) << deltaPct;
                        }
                        string prevStr = havePrev ? ossPrev.str() : string("-");
                        string deltaStr = havePrev ? ossDelta.str() : string("-");
                        string dPctStr = havePrev ? ossPct.str() : string("-");

                        cout << left << setw(4) << id
                             << setw(8) << sym
                             << setw(12) << sec
                             << right << setw(10) << fixed << setprecision(2) << curP
                             << setw(10) << prevStr
                             << setw(10) << fixed << setprecision(2) << h.high
                             << setw(10) << fixed << setprecision(2) << h.low
                             << setw(10) << deltaStr
                             << setw(8) << dPctStr
                             << setw(8) << h.volume << "\n";
                    }
                }

                // holdings summary (unchanged logic)
                double curP=0, high=0, low=0;
                auto pp = market.getPriceExact(curDay, curSeg, id);
                if (pp.first) { curP = pp.second.close; high = pp.second.high; low = pp.second.low; }
                else {
                    auto lp = market.getLatestPriceUpTo(curDay, curSeg, id);
                    if (lp.first) { curP = lp.second.close; high = lp.second.high; low = lp.second.low; }
                }

                long long qty = portfolio.getQty(id);
                double avgp = portfolio.getAvgPrice(id);
                if (qty != 0) {
                    double invested = (double)qty * avgp;
                    double curVal = (double)qty * curP;
                    double unreal = curVal - invested;
                    cout << "Holdings: qty=" << qty
                         << " invested=" << fixed << setprecision(2) << invested
                         << " current=" << fixed << setprecision(2) << curVal
                         << " unrealized=" << fixed << setprecision(2) << unreal << "\n";
                } else {
                    cout << "Holdings: none\n";
                }
            }
AFTER_VIEW: ;
        } else if (ch == 3) {
            cout << "Buy - id: ";
            int id; cin >> id;
            double curP=0.0;
            auto pp = market.getPriceExact(curDay, curSeg, id);
            if (pp.first) curP = pp.second.close;
            else {
                auto lp = market.getLatestPriceUpTo(curDay, curSeg, id);
                if (lp.first) curP = lp.second.close;
            }
            if (curP <= 0.0) {
                cout << "No valid price available for stock id " << id << "\n";
                continue;
            }
            cout << "Price: " << fixed << setprecision(2) << curP << " Qty: ";
            long long qty; cin >> qty;
            if (qty <= 0) { cout << "Invalid qty\n"; continue; }
            string err;
            double cashBefore = portfolio.getCash();
            bool ok = portfolio.buy(id, qty, curP, curDay, curSeg, err);
            if (!ok) {
                cout << "Trade rejected: " << err << "\n";
                continue;
            }
            double cashAfter = portfolio.getCash();
            ll tid = nextTid++;
            string action = "BUY";
            long long posAfter = portfolio.getQty(id);
            double avgP = portfolio.getAvgPrice(id);
            double tradeValue = (double)qty * curP;
            logger.appendTrade(run_id, tid, curDay, curSeg, now_iso(),
                               id, to_string(id), action, qty, curP,
                               tradeValue, cashBefore, cashAfter,
                               posAfter, avgP, 0.0);
            double total2, net2, maxc2, unreal2, realpl2;
            portfolio.computeMetrics(market, curDay, curSeg,
                                     total2, net2, maxc2, unreal2, realpl2);
            logger.appendSnapshot(run_id, curDay, curSeg, now_iso(),
                                  total2, portfolio.getCash(),
                                  net2, maxc2, unreal2, realpl2);
            cout << "Bought " << qty << " of id " << id
                 << " @ " << fixed << setprecision(2) << curP << "\n";
            portfolio.saveState(runPath);
        } else if (ch == 4) {
            cout << "Enter stock id to exit: ";
            int id; cin >> id;

            long long holdingQty = portfolio.getQty(id);
            bool doExit = true;
            if (holdingQty <= 0) {
                cout << "No holdings for this stock\n";
                doExit = false;
            }

            if (doExit) {
                cout << "Holding qty: " << holdingQty << "\n";
                cout << "Enter qty to exit (or exact holding qty to exit all): ";
                long long exitQty;
                cin >> exitQty;

                double curP = 0.0;
                auto pp = market.getPriceExact(curDay, curSeg, id);
                if (pp.first) curP = pp.second.close;
                else {
                    auto lp = market.getLatestPriceUpTo(curDay, curSeg, id);
                    if (lp.first) curP = lp.second.close;
                }
                if (curP <= 0.0) {
                    cout << "No valid price to exit for stock id " << id << "\n";
                    doExit = false;
                }

                if (doExit) {
                    string err;
                    double cashBefore = portfolio.getCash();
                    bool ok = portfolio.exitPositionPartial(id, exitQty, curP, curDay, curSeg, err);
                    if (!ok) {
                        cout << "Exit failed: " << err << "\n";
                    } else {
                        double cashAfter = portfolio.getCash();
                        ll tid = nextTid++;
                        double tradeValue = (double)exitQty * curP;
                        logger.appendTrade(run_id, tid, curDay, curSeg, now_iso(),
                                           id, to_string(id), "EXIT",
                                           exitQty, curP,
                                           tradeValue, cashBefore, cashAfter,
                                           portfolio.getQty(id),
                                           portfolio.getAvgPrice(id),
                                           0.0);
                        double total2, net2, maxc2, unreal2, realpl2;
                        portfolio.computeMetrics(market, curDay, curSeg,
                                                 total2, net2, maxc2, unreal2, realpl2);
                        logger.appendSnapshot(run_id, curDay, curSeg, now_iso(),
                                              total2, portfolio.getCash(),
                                              net2, maxc2, unreal2, realpl2);
                        cout << "Exited " << exitQty << " of id " << id
                             << " @ " << fixed << setprecision(2) << curP << "\n";
                        portfolio.saveState(runPath);
                    }
                }
            }
        } else if (ch == 5) {
            double total2, net2, maxc2, unreal2, realpl2;
            portfolio.computeMetrics(market, curDay, curSeg,
                                     total2, net2, maxc2, unreal2, realpl2);
            logger.appendSnapshot(run_id, curDay, curSeg, now_iso(),
                                  total2, portfolio.getCash(),
                                  net2, maxc2, unreal2, realpl2);
            cout << "Snapshot saved\n";
            portfolio.saveState(runPath);
        } else if (ch == 6) {
            int oldDay = curDay; string oldSeg = curSeg;
            advanceSeg(curDay, curSeg);
            market.loadSegment(curDay, curSeg);
            if (market.getRows().empty()) {
                curDay = oldDay; curSeg = oldSeg;
                market.loadSegment(curDay, curSeg);
                cout << "No data for target segment; advance cancelled\n";
            } else {
                cout << "Advanced to Day " << curDay << " Segment " << curSeg << "\n";
                if (curSeg == "M" && globalNews.count(curDay)) {
                    cout << "=== Morning News ===\n";
                    int idx = 1;
                    for (const auto &item : globalNews[curDay]) {
                        cout << idx++ << ") ";
                        printWrapped(item, 76, "");
                    }
                    cout << "\n";
                }
                if (segNews.count({curDay,curSeg})) {
                    cout << "=== Segment note ===\n";
                    printWrapped(segNews[{curDay,curSeg}], 80, "");
                    cout << "\n";
                }
            }
        } else if (ch == 7) {
            portfolio.saveState(runPath);
            cout << "State saved to " << runPath << "/portfolio_state.csv\n";
            running = false;
        } else {
            cout << "Invalid\n";
        }

        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    cout << "Goodbye\n";
    return 0;
}
