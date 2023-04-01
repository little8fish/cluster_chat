#include "json.hpp"
using json = nlohmann::json;
#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

string func3() {
    json js;
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);

    js["list"] = vec;

    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});

    js["path"] = m;

    string sendBuf = js.dump();
    return sendBuf;
}

int main() {
    string recvBuf = func3();
    json js = json::parse(recvBuf);
    for (auto &e: js["list"]) {
        cout << e << endl;
    }
    map<int, string> m = js["path"];
    for (auto &e: js["path"]) {
        cout << e.first << '\t' << e.second << endl;
    }
    return 0;
}