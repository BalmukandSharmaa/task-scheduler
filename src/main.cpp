#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

struct Task {
    string id, name;
    int priority, duration;
    vector<string> depends;
};

struct Compare {
    bool operator()(Task* a, Task* b) {
        return a->priority > b->priority;
    }
};

map<string, Task> tasks;
map<string, vector<string>> graph;
map<string, int> indegree;
map<string, chrono::steady_clock::time_point> startTime, endTime;

mutex mtx;
condition_variable cv;
priority_queue<Task*, vector<Task*>, Compare> ready;
int completed = 0;
bool stopWorkers = false;

string nowTime() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    string s = ctime(&t);
    s.pop_back();
    return s;
}

bool detectCycle() {
    queue<string> q;
    map<string, int> temp = indegree;
    int count = 0;

    for (auto &p : temp)
        if (p.second == 0) q.push(p.first);

    while (!q.empty()) {
        string u = q.front();
        q.pop();
        count++;

        for (string v : graph[u]) {
            temp[v]--;
            if (temp[v] == 0) q.push(v);
        }
    }

    return count != tasks.size();
}

void worker(int workerId) {
    while (true) {
        Task* task;

        {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [] {
                return !ready.empty() || stopWorkers;
            });

            if (stopWorkers && ready.empty()) return;

            task = ready.top();
            ready.pop();
            startTime[task->id] = chrono::steady_clock::now();

            cout << "[" << nowTime() << "] Worker " << workerId
                 << " STARTED: " << task->name << endl;
        }

        this_thread::sleep_for(chrono::milliseconds(task->duration));

        {
            lock_guard<mutex> lock(mtx);
            endTime[task->id] = chrono::steady_clock::now();

            cout << "[" << nowTime() << "] Worker " << workerId
                 << " COMPLETED: " << task->name << endl;

            completed++;

            for (string next : graph[task->id]) {
                indegree[next]--;
                if (indegree[next] == 0) {
                    ready.push(&tasks[next]);
                }
            }

            if (completed == tasks.size()) {
                stopWorkers = true;
            }
        }

        cv.notify_all();
    }
}

int criticalPath() {
    map<string, int> dp;
    queue<string> q;
    map<string, int> temp;

    for (auto &p : tasks) {
        temp[p.first] = p.second.depends.size();
        dp[p.first] = p.second.duration;
        if (temp[p.first] == 0) q.push(p.first);
    }

    while (!q.empty()) {
        string u = q.front();
        q.pop();

        for (string v : graph[u]) {
            dp[v] = max(dp[v], dp[u] + tasks[v].duration);
            temp[v]--;
            if (temp[v] == 0) q.push(v);
        }
    }

    int ans = 0;
    for (auto &p : dp) ans = max(ans, p.second);
    return ans;
}

int main(int argc, char* argv[]) {
    int workers = 4;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--workers" && i + 1 < argc) {
            workers = stoi(argv[i + 1]);
        }
    }

    ifstream file("tasks.json");
    if (!file.is_open()) {
        cout << "Error: tasks.json not found" << endl;
        return 1;
    }

    json data;
    file >> data;

    for (auto &item : data["tasks"]) {
        Task t;
        t.id = item["id"];
        t.name = item["name"];
        t.priority = item["priority"];
        t.duration = item["duration_ms"];

        for (auto &d : item["depends_on"])
            t.depends.push_back(d);

        tasks[t.id] = t;
        indegree[t.id] = t.depends.size();
    }

    for (auto &p : tasks) {
        for (string dep : p.second.depends) {
            graph[dep].push_back(p.first);
        }
    }

    if (detectCycle()) {
        cout << "Circular dependency detected. Execution stopped." << endl;
        return 1;
    }

    for (auto &p : tasks) {
        if (indegree[p.first] == 0)
            ready.push(&tasks[p.first]);
    }

    auto totalStart = chrono::steady_clock::now();

    vector<thread> pool;
    for (int i = 1; i <= workers; i++) {
        pool.push_back(thread(worker, i));
    }

    cv.notify_all();

    for (auto &th : pool) th.join();

    auto totalEnd = chrono::steady_clock::now();
    auto totalTime = chrono::duration_cast<chrono::milliseconds>(totalEnd - totalStart).count();

    cout << "\nFinal Report\n";
    cout << "Total wall-clock time: " << totalTime << " ms\n";
    cout << "Critical path length: " << criticalPath() << " ms\n\n";

    cout << "Per-task timings:\n";
    for (auto &p : tasks) {
        auto s = chrono::duration_cast<chrono::milliseconds>(startTime[p.first] - totalStart).count();
        auto e = chrono::duration_cast<chrono::milliseconds>(endTime[p.first] - totalStart).count();

        cout << p.second.name << " | Start: " << s << " ms | End: " << e << " ms\n";
    }

    return 0;
}
