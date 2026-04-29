#include <bits/stdc++.h>
#include <thread>
#include <chrono>
using namespace std;

struct Task {
    int id;
    string name;
    int priority;
    int duration;
    vector<int> deps;
};
struct Compare {
    bool operator()(Task* a, Task* b) {
        return a->priority > b->priority;
    }
};

class Scheduler {
private:
    unordered_map<int, Task> tasks;
    unordered_map<int, vector<int>> adj;
    unordered_map<int, int> indegree;

public:
    void addTask(Task t) {
        tasks[t.id] = t;
        indegree[t.id] = 0;
    }

    void buildGraph() {
        for (auto &p : tasks) {
            for (int dep : p.second.deps) {
                adj[dep].push_back(p.first);
                indegree[p.first]++;
            }
        }
    }

    bool detectCycle() {
        queue<int> q;
        unordered_map<int, int> temp = indegree;

        for (auto &p : temp) {
            if (p.second == 0)
                q.push(p.first);
        }

        int count = 0;

        while (!q.empty()) {
            int node = q.front(); q.pop();
            count++;

            for (int nei : adj[node]) {
                temp[nei]--;
                if (temp[nei] == 0)
                    q.push(nei);
            }
        }

        return count != tasks.size();
    }

    void run() {
        priority_queue<Task*, vector<Task*>, Compare> pq;

        // Push all tasks with 0 indegree
        for (auto &p : tasks) {
            if (indegree[p.first] == 0)
                pq.push(&tasks[p.first]);
        }

        while (!pq.empty()) {
            Task* t = pq.top();
            pq.pop();

            cout << "Starting Task " << t->id << " (" << t->name << ")\n";

            this_thread::sleep_for(chrono::milliseconds(t->duration));

            cout << "Completed Task " << t->id << "\n";

            for (int nei : adj[t->id]) {
                indegree[nei]--;
                if (indegree[nei] == 0) {
                    pq.push(&tasks[nei]);
                }
            }
        }
    }
};

int main() {
    Scheduler scheduler;

    // Sample tasks
    scheduler.addTask({1, "Task A", 1, 1000, {}});
    scheduler.addTask({2, "Task B", 2, 800, {1}});
    scheduler.addTask({3, "Task C", 1, 500, {1}});
    scheduler.addTask({4, "Task D", 3, 700, {2, 3}});

    scheduler.buildGraph();

    if (scheduler.detectCycle()) {
        cout << "Cycle detected! Cannot proceed.\n";
        return 0;
    }

    scheduler.run();

    return 0;
} 
