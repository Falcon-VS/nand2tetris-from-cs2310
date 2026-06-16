#include <bits/stdc++.h>
#include <filesystem>
using namespace std;
namespace fs = filesystem;

struct Tok { string t, v; };

bool sym(char c) {
    static string s = "{}()[].,;+-*/&|<>=~";
    return s.find(c) != string::npos;
}

bool key(const string &x) {
    static unordered_set<string> k = {
        "class","constructor","function","method","field","static","var",
        "int","char","boolean","void","true","false","null","this",
        "let","do","if","else","while","return"
    };
    return k.count(x);
}

vector<Tok> lex(const string &path) {
    ifstream in(path);
    string s((istreambuf_iterator<char>(in)), {});
    vector<string> raw;
    string tmp;
    bool inStr = 0, inBlock = 0;

    for (size_t i = 0; i < s.size(); i++) {
        if (!inBlock && i + 1 < s.size() && s[i] == '/' && s[i + 1] == '*') { inBlock = 1; i++; continue; }
        if (inBlock && i + 1 < s.size() && s[i] == '*' && s[i + 1] == '/') { inBlock = 0; i++; continue; }
        if (inBlock) continue;
        if (!inBlock && i + 1 < s.size() && s[i] == '/' && s[i + 1] == '/') { while (i < s.size() && s[i] != '\n') i++; continue; }

        char c = s[i];
        if (inStr) {
            tmp += c;
            if (c == '"') { raw.push_back(tmp); tmp.clear(); inStr = 0; }
        } else if (isspace(c)) {
            if (!tmp.empty()) { raw.push_back(tmp); tmp.clear(); }
        } else if (sym(c)) {
            if (!tmp.empty()) { raw.push_back(tmp); tmp.clear(); }
            raw.push_back(string(1, c));
        } else if (c == '"') {
            if (!tmp.empty()) { raw.push_back(tmp); tmp.clear(); }
            tmp = "\""; inStr = 1;
        } else tmp += c;
    }
    if (!tmp.empty()) raw.push_back(tmp);

    vector<Tok> toks;
    for (auto &x : raw) {
        Tok t;
        if (key(x)) t.t = "kw";
        else if (x.size() == 1 && sym(x[0])) t.t = "sym";
        else if (isdigit(x[0])) t.t = "int";
        else if (x[0] == '"') { t.t = "str"; t.v = x.substr(1, x.size() - 2); toks.push_back(t); continue; }
        else t.t = "id";
        t.v = x;
        toks.push_back(t);
    }
    return toks;
}

struct Entry { string tp, kd; int id; };

struct Table {
    unordered_map<string, Entry> cls, sub;
    int st = 0, fld = 0, arg = 0, var = 0;

    void start() { sub.clear(); arg = var = 0; }
    void add(string n, string t, string k) {
        if (k == "static") cls[n] = {t, k, st++};
        else if (k == "field") cls[n] = {t, k, fld++};
        else if (k == "arg") sub[n] = {t, k, arg++};
        else if (k == "var") sub[n] = {t, k, var++};
    }
    bool has(const string &n) { return sub.count(n) || cls.count(n); }
    Entry get(const string &n) { return sub.count(n) ? sub[n] : cls[n]; }
    int cnt(const string &k) { return k == "field" ? fld : (k == "var" ? var : 0); }
};

string segm(const string &k) {
    if (k == "static") return "static";
    if (k == "field") return "this";
    if (k == "arg") return "argument";
    if (k == "var") return "local";
    return "";
}

struct VM {
    ofstream out;
    VM(string f) { out.open(f); }
    void func(string n, int l) { out << "function " << n << " " << l << "\n"; }
    void push(string s, int i) { out << "push " << s << " " << i << "\n"; }
    void pop(string s, int i) { out << "pop " << s << " " << i << "\n"; }
    void op(string s) { out << s << "\n"; }
    void call(string f, int n) { out << "call " << f << " " << n << "\n"; }
    void ret() { out << "return\n"; }
    void lbl(string l) { out << "label " << l << "\n"; }
    void go(string l) { out << "goto " << l << "\n"; }
    void ifgo(string l) { out << "if-goto " << l << "\n"; }
};

struct Comp {
    vector<Tok> t;
    size_t p = 0;
    Table tb;
    VM *vm;
    string cls;
    int L = 0;

    Comp(vector<Tok> T, VM *V) : t(T), vm(V) {}

    Tok cur() { return p < t.size() ? t[p] : Tok(); }
    Tok nxt() { return p < t.size() ? t[p++] : Tok(); }
    bool eq(const string &s) { return p < t.size() && t[p].v == s; }
    void must(const string &s) { if (!eq(s)) cerr << "Expected '" << s << "' but got '" << (p < t.size() ? t[p].v : "<eof>") << "'\n"; else p++; }

    bool isType() {
        if (p >= t.size()) return false;
        auto &x = t[p];
        return x.t == "id" || (x.t == "kw" && (x.v == "int" || x.v == "char" || x.v == "boolean"));
    }

    void klass() {
        must("class");
        cls = nxt().v;
        must("{");
        while (eq("static") || eq("field")) cvar();
        while (eq("constructor") || eq("function") || eq("method")) subr();
        must("}");
    }

    void cvar() {
        string k = nxt().v, tp = nxt().v, n = nxt().v;
        tb.add(n, tp, k);
        while (eq(",")) { nxt(); tb.add(nxt().v, tp, k); }
        must(";");
    }

    void subr() {
        string sk = nxt().v;
        if (eq("void") || isType()) nxt();
        string name = nxt().v;
        tb.start();
        if (sk == "method") tb.add("this", cls, "arg");
        must("("); params(); must(")");
        must("{");
        while (eq("var")) vard();
        vm->func(cls + "." + name, tb.cnt("var"));
        if (sk == "constructor") {
            vm->push("constant", tb.cnt("field"));
            vm->call("Memory.alloc", 1);
            vm->pop("pointer", 0);
        } else if (sk == "method") {
            vm->push("argument", 0);
            vm->pop("pointer", 0);
        }
        stats();
        must("}");
    }

    void params() {
        if (isType()) {
            string tp = nxt().v, n = nxt().v;
            tb.add(n, tp, "arg");
            while (eq(",")) { nxt(); tb.add(nxt().v, nxt().v, "arg"); }
        }
    }

    void vard() {
        must("var");
        string tp = nxt().v, n = nxt().v;
        tb.add(n, tp, "var");
        while (eq(",")) { nxt(); tb.add(nxt().v, tp, "var"); }
        must(";");
    }

    void stats() {
        while (true) {
            if (eq("let")) let_();
            else if (eq("if")) if_();
            else if (eq("while")) while_();
            else if (eq("do")) do_();
            else if (eq("return")) ret_();
            else break;
        }
    }

    void do_() { must("do"); subCall(); must(";"); vm->pop("temp", 0); }

    void let_() {
        must("let");
        string n = nxt().v; bool arr = 0;
        if (eq("[")) {
            nxt(); expr(); must("]");
            auto s = tb.get(n);
            vm->push(segm(s.kd), s.id); vm->op("add");
            arr = 1;
        }
        must("=");
        expr(); must(";");
        if (arr) {
            vm->pop("temp", 0); vm->pop("pointer", 1);
            vm->push("temp", 0); vm->pop("that", 0);
        } else {
            auto s = tb.get(n); vm->pop(segm(s.kd), s.id);
        }
    }

    void while_() {
        must("while");
        int a = L++, b = L++;
        vm->lbl("WHILE_EXP" + to_string(a));
        must("("); expr(); must(")");
        vm->op("not"); vm->ifgo("WHILE_END" + to_string(b));
        must("{"); stats(); must("}");
        vm->go("WHILE_EXP" + to_string(a)); vm->lbl("WHILE_END" + to_string(b));
    }

    void if_() {
        must("if"); must("("); expr(); must(")");
        int f = L++, e = L++;
        vm->op("not"); vm->ifgo("IF_FALSE" + to_string(f));
        must("{"); stats(); must("}");
        if (eq("else")) {
            vm->go("IF_END" + to_string(e));
            vm->lbl("IF_FALSE" + to_string(f));
            nxt(); must("{"); stats(); must("}"); vm->lbl("IF_END" + to_string(e));
        } else vm->lbl("IF_FALSE" + to_string(f));
    }

    void ret_() {
        must("return");
        if (!eq(";")) expr(); else vm->push("constant", 0);
        must(";"); vm->ret();
    }

    int exprList() {
        int n = 0;
        if (!eq(")")) {
            expr(); n++;
            while (eq(",")) { nxt(); expr(); n++; }
        }
        return n;
    }

    void subCall() {
        string n = nxt().v; int nargs = 0;
        if (eq(".")) {
            nxt(); string s = nxt().v; must("(");
            if (tb.has(n)) {
                auto sym = tb.get(n);
                vm->push(segm(sym.kd), sym.id);
                nargs = 1 + exprList(); must(")");
                vm->call(sym.tp + "." + s, nargs);
            } else {
                nargs = exprList(); must(")");
                vm->call(n + "." + s, nargs);
            }
        } else {
            must("("); vm->push("pointer", 0);
            nargs = 1 + exprList(); must(")");
            vm->call(cls + "." + n, nargs);
        }
    }

    void expr() {
        term();
        while (eq("+") || eq("-") || eq("*") || eq("/") || eq("&") || eq("|") || eq("<") || eq(">") || eq("=")) {
            string op = nxt().v; term();
            if (op == "+") vm->op("add");
            else if (op == "-") vm->op("sub");
            else if (op == "*") vm->call("Math.multiply", 2);
            else if (op == "/") vm->call("Math.divide", 2);
            else if (op == "&") vm->op("and");
            else if (op == "|") vm->op("or");
            else if (op == "<") vm->op("lt");
            else if (op == ">") vm->op("gt");
            else if (op == "=") vm->op("eq");
        }
    }

    void term() {
        Tok x = nxt();
        if (x.t == "int") { vm->push("constant", stoi(x.v)); return; }
        if (x.t == "str") {
            vm->push("constant", x.v.size());
            vm->call("String.new", 1);
            for (char c : x.v) { vm->push("constant", (int)c); vm->call("String.appendChar", 2); }
            return;
        }
        if (x.t == "kw") {
            if (x.v == "true") { vm->push("constant", 1); vm->op("neg"); }
            else if (x.v == "false" || x.v == "null") vm->push("constant", 0);
            else if (x.v == "this") vm->push("pointer", 0);
            return;
        }
        if (x.v == "(") { expr(); must(")"); return; }
        if (x.v == "-" || x.v == "~") { term(); vm->op(x.v == "-" ? "neg" : "not"); return; }
        if (x.t == "id") {
            string n = x.v;
            if (eq("[")) {
                nxt(); expr(); must("]");
                auto s = tb.get(n);
                vm->push(segm(s.kd), s.id);
                vm->op("add"); vm->pop("pointer", 1);
                vm->push("that", 0);
                return;
            }
            if (eq("(") || eq(".")) { p--; subCall(); return; }
            if (tb.has(n)) {
                auto s = tb.get(n);
                vm->push(segm(s.kd), s.id);
            }
        }
    }
};

int main(int c, char **v) {
    if (c < 2) { cerr << "Usage: jack <file|dir>\n"; return 1; }
    fs::path in(v[1]);
    vector<fs::path> fslist;

    if (fs::is_directory(in)) {
        for (auto &f : fs::directory_iterator(in))
            if (f.path().extension() == ".jack") fslist.push_back(f.path());
    } else if (in.extension() == ".jack") fslist.push_back(in);

    for (auto &f : fslist) {
        auto toks = lex(f.string());
        VM vm(f.stem().string() + ".vm");
        Comp cpl(toks, &vm);
        cpl.klass();
        cout << "Built " << f.filename().string() << " -> " << f.stem().string() << ".vm\n";
    }
}
