#include <bits/stdc++.h>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

struct Task {
    int id, priority, duration;
    string name;
    vector<int> deps;
};

struct Cmp {
    bool operator()(Task* a, Task* b) {
        return a->priority > b->priority;
    }
};

int main(int argc, char* argv[]) {
    string file = "tasks.json";
    int workers = 4;

    if (argc >= 2) file = argv[1];

    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--workers" && i + 1 < argc)
            workers = stoi(argv[++i]);
    }

    ifstream in(file);
    if (!in) {
        cout << "tasks.json not found\n";
        return 1;
    }

    json data;
    in >> data;

    vector<Task> tasks;
    unordered_map<int, Task*> mp;
    unordered_map<int, vector<int>> graph;
    unordered_map<int, int> indeg;
    unordered_map<int, long long> startTime, endTime;

    for (auto &x : data["tasks"]) {
        Task t;
        t.id = x["id"];
        t.name = x["name"];
        t.priority = x["priority"];
        t.duration = x["duration_ms"];

        for (auto &d : x["depends_on"])
            t.deps.push_back(d);

        tasks.push_back(t);
    }

    for (auto &t : tasks) {
        mp[t.id] = &t;
        indeg[t.id] = 0;
    }

    for (auto &t : tasks) {
        for (int d : t.deps) {
            graph[d].push_back(t.id);
            indeg[t.id]++;
        }
    }

    queue<int> q;
    auto temp = indeg;
    int count = 0;

    for (auto &x : temp)
        if (x.second == 0) q.push(x.first);

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        count++;

        for (int v : graph[u]) {
            temp[v]--;
            if (temp[v] == 0) q.push(v);
        }
    }

    if (count != tasks.size()) {
        cout << "Cycle detected\n";
        return 0;
    }

    priority_queue<Task*, vector<Task*>, Cmp> ready;

    for (auto &x : indeg)
        if (x.second == 0)
            ready.push(mp[x.first]);

    mutex mtx;
    int completed = 0;

    auto base = chrono::steady_clock::now();

    auto now = [&]() {
        return chrono::duration_cast<chrono::milliseconds>(
            chrono::steady_clock::now() - base
        ).count();
    };

    auto worker = [&](int wid) {
        while (true) {
            Task* task = nullptr;

            {
                lock_guard<mutex> lock(mtx);

                if (ready.empty()) {
                    if (completed == tasks.size()) return;
                    continue;
                }

                task = ready.top();
                ready.pop();

                startTime[task->id] = now();

                cout << "[" << startTime[task->id] << " ms] "
                     << "Worker " << wid << " START "
                     << task->name << "\n";
            }

            this_thread::sleep_for(chrono::milliseconds(task->duration));

            {
                lock_guard<mutex> lock(mtx);

                endTime[task->id] = now();

                cout << "[" << endTime[task->id] << " ms] "
                     << "Worker " << wid << " DONE "
                     << task->name << "\n";

                completed++;

                for (int child : graph[task->id]) {
                    indeg[child]--;

                    if (indeg[child] == 0)
                        ready.push(mp[child]);
                }
            }
        }
    };

    vector<thread> pool;

    for (int i = 1; i <= workers; i++)
        pool.push_back(thread(worker, i));

    for (auto &t : pool)
        t.join();

    cout << "\nFinal Report\n";
    cout << "Total Time: " << now() << " ms\n";

    for (auto &t : tasks) {
        cout << "Task " << t.id << " " << t.name
             << " Start: " << startTime[t.id]
             << " End: " << endTime[t.id] << "\n";
    }

    return 0;
}
