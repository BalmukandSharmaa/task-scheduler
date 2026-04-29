# Task Scheduler with Priority and Dependencies
This is a simple concurrent task scheduler built in C++.
It reads tasks from a JSON file and runs them using multiple worker threads.  
Each task can have dependencies, priority, and simulated execution time.
The project is similar to a small build system where some tasks can only start after other tasks are completed.
## Features
- Reads task details from `tasks.json`
- Supports task dependencies
- Detects circular dependencies before execution
- Runs independent tasks in parallel
- Supports priority-based scheduling
- Configurable number of workers
- Prints real-time start and completion logs
- Shows final execution report
- Calculates critical path length

## Task Fields
Each task contains:
- `id` - unique task id
- `name` - task name
- `priority` - task priority, where 1 is highest
- `depends_on` - list of task ids that must complete first
- `duration_ms` - simulated execution time in milliseconds

## How to Run
First, download the JSON library file:
Use this header file:

```cpp
#include "json.hpp"
