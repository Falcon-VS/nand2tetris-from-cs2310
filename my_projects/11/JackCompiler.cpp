#include <bits/stdc++.h>
#include <filesystem>
using namespace std;
namespace fs = filesystem;

struct Token { string type, val; };
bool isSymbol(char c){ 
    return string("{}()[].,;+-*/&|<>=~").find(c)!=string::npos; 
}
bool isKeyword(const string &s){
    static unordered_set<string> k={"class","constructor","function","method","field","static","var", "int","char","boolean","void","true","false","null","this","let","do","if","else","while","return"};
    return k.count(s);
}

vector<Token> tokenize(const string &path){
    ifstream in(path);
    string s((istreambuf_iterator<char>(in)),{});
    vector<string> raw;
    string t;
    bool str=0,blk=0;
    for(size_t i=0;i<s.size();i++){
        if(!blk && i+1<s.size()&&s[i]=='/'&&s[i+1]=='*'){blk=1;i++;continue;}
        if(blk && i+1<s.size()&&s[i]=='*'&&s[i+1]=='/'){blk=0;i++;continue;}
        if(blk)continue;
        if(!blk && i+1<s.size()&&s[i]=='/'&&s[i+1]=='/'){while(i<s.size()&&s[i]!='\n')i++;continue;}
        char c=s[i];
        if(str){t+=c;if(c=='"'){raw.push_back(t);t.clear();str=0;}}
        else if(isspace(c)){if(!t.empty()){raw.push_back(t);t.clear();}}
        else if(isSymbol(c)){if(!t.empty()){raw.push_back(t);t.clear();}raw.push_back(string(1,c));}
        else if(c=='"'){if(!t.empty()){raw.push_back(t);t.clear();}t="\"";str=1;}
        else t+=c;
    }
    if(!t.empty())raw.push_back(t);
    vector<Token> tok;
    for(auto &x:raw){
        Token tk;
        if(isKeyword(x)) tk.type="keyword";
        else if(x.size()==1&&isSymbol(x[0])) tk.type="symbol";
        else if(isdigit(x[0])) tk.type="int";
        else if(x[0]=='"'){tk.type="string";tk.val=x.substr(1,x.size()-2);tok.push_back(tk);continue;}
        else tk.type="ident";
        tk.val=x; tok.push_back(tk);
    }
    return tok;
}

struct Sym { string type,kind; int idx; };
struct SymTable {
    unordered_map<string,Sym> cls,sub;
    int stat=0,field=0,arg=0,var=0;
    void startSub(){sub.clear();arg=var=0;}
    void define(string n,string t,string k){
        if(k=="static")cls[n]={t,k,stat++};
        else if(k=="field")cls[n]={t,k,field++};
        else if(k=="arg")sub[n]={t,k,arg++};
        else if(k=="var")sub[n]={t,k,var++};
    }
    bool has(string n){return sub.count(n)||cls.count(n);}
    Sym get(string n){return sub.count(n)?sub[n]:cls[n];}
    int count(string k){return k=="field"?field:k=="var"?var:0;}
};
string seg(string k){
    if(k=="static")return"static";
    if(k=="field")return"this";
    if(k=="arg")return"argument";
    if(k=="var")return"local";
    return"";
}

struct VM {
    ofstream o;
    VM(string f){o.open(f);}
    void fdef(string n,int l){
        o<<"function "<<n<<" "<<l<<"\n";
    }
    void push(string s,int i){
        o<<"push "<<s<<" "<<i<<"\n";
    }
    void pop(string s,int i){
        o<<"pop "<<s<<" "<<i<<"\n";
    }
    void op(string c){
        o<<c<<"\n";
    }
    void call(string n,int a){
        o<<"call "<<n<<" "<<a<<"\n";
    }
    void ret(){
        o<<"return\n";
    }
    void label(string l){
        o<<"label "<<l<<"\n";
    }
    void go(string l){
        o<<"goto "<<l<<"\n";
    }
    void ifgo(string l){
        o<<"if-goto "<<l<<"\n";
    }
};

struct Compiler {
    vector<Token> t;
    size_t p=0; 
    SymTable st; 
    VM *vm; 
    string cls; 
    int L=0;
    Compiler(vector<Token> T,VM*V) : t(T),vm(V){}
    Token cur(){
        return p<t.size()?t[p]:Token();
    }
    Token adv(){
        return p<t.size()?t[p++]:Token();
    }
    bool eq(string s){
        return p<t.size()&&t[p].val==s;
    }
    void need(string s){
        if(!eq(s)) cerr << "Parse error: expected '" << s << "' got '" << (p<t.size()?t[p].val:"<eof>") << "'\n";
        else p++;
    }
    bool type(){
        return p<t.size() && (t[p].type=="ident"||(t[p].type=="keyword"&&(t[p].val=="int"||t[p].val=="char"||t[p].val=="boolean")));
    }

    void class_(){
        need("class"); cls=adv().val; need("{");
        while(eq("static")||eq("field"))classVar();
        while(eq("constructor")||eq("function")||eq("method"))subr();
        need("}");
    }
    void classVar(){
        string k=adv().val; 
        string ty=adv().val; 
        string n=adv().val; 
        st.define(n,ty,k);
        while(eq(",")){adv();string n2=adv().val;st.define(n2,ty,k);} need(";");
    }
    void subr(){
        string sk=adv().val; 
        if(eq("void")||type()) adv();
        string name=adv().val; st.startSub(); 
        if(sk=="method") st.define("this",cls,"arg");
        need("("); params(); need(")"); need("{");
        while(eq("var"))varDec();
        vm->fdef(cls+"."+name,st.count("var"));
        if(sk=="constructor"){
            vm->push("constant",st.count("field"));
            vm->call("Memory.alloc",1);
            vm->pop("pointer",0);
        }
        else if(sk=="method"){
            vm->push("argument",0);
            vm->pop("pointer",0);
        }
        stats(); 
        need("}");
    }
    void params(){
        if(type()){
            string ty=adv().val;
            string n=adv().val;
            st.define(n,ty,"arg");
            while(eq(",")){
                adv();
                string ty2=adv().val;
                string n2=adv().val;
                st.define(n2,ty2,"arg");
            }
        }
    }
    void varDec(){
        need("var"); 
        string ty=adv().val; 
        string n=adv().val; 
        st.define(n,ty,"var");
        while(eq(",")){
            adv();
            string n2=adv().val;
            st.define(n2,ty,"var");
        } 
        need(";");
    }
    void stats(){
        while(true){
            if(eq("let"))let_();
            else if(eq("if"))if_();
            else if(eq("while"))while_();
            else if(eq("do"))do_();
            else if(eq("return"))ret_();
            else break;
        }
    }
    void do_(){
        need("do"); 
        subCall(); 
        need(";"); 
        vm->pop("temp",0);
    }
    void let_() {
    need("let");
    string name = adv().val;
    bool isArray = false;
    if (eq("[")) {
        adv();
        expr();
        need("]");
        Sym s = st.get(name);
        vm->push(seg(s.kind), s.idx);
        vm->op("add");
        isArray = true;
    }

    need("=");
    expr();
    need(";");

    if (isArray) {
        vm->pop("temp", 0);
        vm->pop("pointer", 1);
        vm->push("temp", 0);
        vm->pop("that", 0);
    } else {
        Sym s = st.get(name);
        vm->pop(seg(s.kind), s.idx);
    }
}

    void while_(){
        need("while"); 
        int a=L++,b=L++; 
        vm->label("WHILE_EXP"+to_string(a));
        need("("); 
        expr(); 
        need(")"); 
        vm->op("not"); 
        vm->ifgo("WHILE_END"+to_string(b));
        need("{"); 
        stats(); 
        need("}"); 
        vm->go("WHILE_EXP"+to_string(a)); 
        vm->label("WHILE_END"+to_string(b));
    }
    void if_(){
        need("if"); 
        need("("); 
        expr(); 
        need(")");
        int f=L++,e=L++; 
        vm->op("not"); 
        vm->ifgo("IF_FALSE"+to_string(f));
        need("{"); 
        stats(); 
        need("}");
        if(eq("else")){
            vm->go("IF_END"+to_string(e)); 
            vm->label("IF_FALSE"+to_string(f));
            adv(); 
            need("{"); 
            stats(); 
            need("}"); 
            vm->label("IF_END"+to_string(e));
        }
        else vm->label("IF_FALSE"+to_string(f));
    }
    void ret_(){
        need("return"); 
        if(!eq(";"))expr(); 
        else vm->push("constant",0); 
        need(";"); 
        vm->ret();
    }

    int exprList(){
        int n=0; 
        if(!eq(")")){
            expr(); 
            n++; 
            while(eq(",")){
                adv();
                expr();
                n++;
            }
        } 
        return n;
    }

    void subCall(){
        string n=adv().val; 
        int nargs=0;
        if(eq(".")){
            adv(); 
            string sub=adv().val; 
            need("(");
            if(st.has(n)){
                Sym s=st.get(n); 
                vm->push(seg(s.kind),s.idx); 
                nargs=1+exprList(); need(")"); 
                vm->call(s.type+"."+sub,nargs);
            }
            else {
                nargs=exprList(); 
                need(")"); 
                vm->call(n+"."+sub,nargs);
            }
        } else {
            need("("); 
            vm->push("pointer",0); 
            nargs=1+exprList(); need(")"); 
            vm->call(cls+"."+n,nargs);
        }
    }

    void expr(){
        term(); 
        while(eq("+")||eq("-")||eq("*")||eq("/")||eq("&")||eq("|")||eq("<")||eq(">")||eq("=")){
            string op=adv().val; term();
            if(op=="+") vm->op("add"); 
            else if(op=="-")vm->op("sub");
            else if(op=="*")vm->call("Math.multiply",2); 
            else if(op=="/")vm->call("Math.divide",2);
            else if(op=="&")vm->op("and"); 
            else if(op=="|")vm->op("or");
            else if(op=="<")vm->op("lt"); 
            else if(op==">")vm->op("gt"); 
            else if(op=="=")vm->op("eq");
        }
    }

    void term() {
    Token x = adv();

    if (x.type == "int") {
        vm->push("constant", stoi(x.val));
        return;
    }

    if (x.type == "string") {
        vm->push("constant", x.val.size());
        vm->call("String.new", 1);
        for (char c : x.val) {
            vm->push("constant", (int)c);
            vm->call("String.appendChar", 2);
        }
        return;
    }

    if (x.type == "keyword") {
        if (x.val == "true") { vm->push("constant", 1); vm->op("neg"); }
        else if (x.val == "false" || x.val == "null") vm->push("constant", 0);
        else if (x.val == "this") vm->push("pointer", 0);
        return;
    }

    if (x.val == "(") {
        expr();
        need(")");
        return;
    }

    if (x.val == "-" || x.val == "~") {
        term();
        if (x.val == "-") vm->op("neg");
        else vm->op("not");
        return;
    }

    if (x.type == "ident") {
        string name = x.val;
        if (eq("[")) {
            adv();
            expr();
            need("]");
            Sym s = st.get(name);
            vm->push(seg(s.kind), s.idx);
            vm->op("add");
            vm->pop("pointer", 1);
            vm->push("that", 0);
            return;
        }
        if (eq("(") || eq(".")) {
            p--; 
            subCall();
            return;
        }
        if (st.has(name)) {
            Sym s = st.get(name);
            vm->push(seg(s.kind), s.idx);
        }
        return;
    }
}

};

int main(int c,char**v){
    if(c<2){ cerr<<"Usage: JackCompiler <file|dir>\n";return 1; }
    fs::path in(v[1]); 
    vector<fs::path> files;
    if(fs::is_directory(in)){
        for(auto &f:fs::directory_iterator(in)) 
            if(f.path().extension()==".jack") files.push_back(f.path());
    }
    else if(in.extension()==".jack")files.push_back(in);
    for(auto &f:files){
        auto tok=tokenize(f.string());
        VM vm(f.stem().string()+".vm");
        Compiler comp(tok,&vm);
        comp.class_();
        cout<<"Compiled "<<f.filename().string()<<" -> "<<f.stem().string()<<".vm\n";
    }
}
