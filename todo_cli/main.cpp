#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

struct Task {
    int id;
    std::string text;
    bool completed;
};

enum class ActionType { Added, Completed, Deleted };

struct UndoAction {
    ActionType type;
    Task task;           // The task as it was before the change.
    std::size_t index;   // Its original position in the list.
};

// The vector stores tasks in display order. The second vector acts as a stack:
// push_back records an action, and pop_back undoes the most recent action.
std::vector<Task> tasks;
std::vector<UndoAction> undo_stack;
int next_id = 1;

bool load_tasks(const std::filesystem::path& file) {
    if (!std::filesystem::exists(file)) {
        return true;
    }

    std::ifstream input(file);
    int stored_next_id = 0;
    std::size_t count = 0;
    if (!(input >> stored_next_id >> count) || stored_next_id < 1 || count > 1000000) {
        return false;
    }

    std::vector<Task> loaded;
    for (std::size_t i = 0; i < count; ++i) {
        Task task{};
        int completed = 0;
        if (!(input >> task.id >> completed >> std::quoted(task.text)) ||
            task.id < 1 || task.id >= stored_next_id ||
            (completed != 0 && completed != 1)) {
            return false;
        }
        task.completed = (completed == 1);
        loaded.push_back(task);
    }

    tasks = std::move(loaded);
    next_id = stored_next_id;
    return true;
}

bool save_tasks(const std::filesystem::path& file) {
    std::ofstream output(file);
    if (!output) {
        return false;
    }
    output << next_id << ' ' << tasks.size() << '\n';
    for (const Task& task : tasks) {
        output << task.id << ' ' << task.completed << ' '
               << std::quoted(task.text) << '\n';
    }
    return static_cast<bool>(output);
}

bool parse_id(const std::string& argument, int& id) {
    std::istringstream input(argument);
    std::string extra;
    return (input >> id) && id > 0 && !(input >> extra);
}

std::size_t find_task(int id) {
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        if (tasks[i].id == id) {
            return i;
        }
    }
    return tasks.size();
}

void list_tasks() {
    if (tasks.empty()) {
        std::cout << "No tasks yet.\n";
        return;
    }
    for (const Task& task : tasks) {
        std::cout << task.id << ". [" << (task.completed ? 'x' : ' ') << "] "
                  << task.text << '\n';
    }
}

void print_help() {
    std::cout << "Commands:\n"
              << "  add TEXT   Add a task\n"
              << "  list       Show all tasks\n"
              << "  done ID    Mark a task complete\n"
              << "  delete ID  Delete a task\n"
              << "  undo       Undo the last change in this session\n"
              << "  help       Show commands\n"
              << "  quit       Exit\n";
}

int main(int argc, char* argv[]) {
    const std::filesystem::path data_file =
        std::filesystem::absolute(argc > 0 ? argv[0] : "todo_cli")
            .parent_path() / "tasks.txt";

    if (!load_tasks(data_file)) {
        std::cerr << "Could not read tasks.txt. Fix or back up the file before retrying.\n";
        return 1;
    }

    std::cout << "Todo CLI (type help for commands)\n";
    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        std::istringstream input(line);
        std::string command;
        input >> command;
        std::string argument;
        std::getline(input, argument);

        bool changed = false;
        if (command == "add") {
            const auto first = argument.find_first_not_of(" \t");
            if (first == std::string::npos) {
                std::cout << "Usage: add TEXT\n";
                continue;
            }
            Task task{next_id++, argument.substr(first), false};
            undo_stack.push_back({ActionType::Added, task, tasks.size()});
            tasks.push_back(task);
            std::cout << "Added task " << task.id << ".\n";
            changed = true;
        } else if (command == "list") {
            list_tasks();
        } else if (command == "done" || command == "delete") {
            int id = 0;
            if (!parse_id(argument, id)) {
                std::cout << "Usage: " << command << " ID\n";
                continue;
            }
            const std::size_t index = find_task(id);
            if (index == tasks.size()) {
                std::cout << "Task " << id << " does not exist.\n";
                continue;
            }
            if (command == "done") {
                if (tasks[index].completed) {
                    std::cout << "Task " << id << " is already complete.\n";
                    continue;
                }
                undo_stack.push_back({ActionType::Completed, tasks[index], index});
                tasks[index].completed = true;
                std::cout << "Completed task " << id << ".\n";
            } else {
                undo_stack.push_back({ActionType::Deleted, tasks[index], index});
                tasks.erase(tasks.begin() + index);
                std::cout << "Deleted task " << id << ".\n";
            }
            changed = true;
        } else if (command == "undo") {
            if (undo_stack.empty()) {
                std::cout << "Nothing to undo.\n";
                continue;
            }
            UndoAction action = undo_stack.back();
            undo_stack.pop_back();
            if (action.type == ActionType::Added) {
                tasks.erase(tasks.begin() + action.index);
            } else if (action.type == ActionType::Completed) {
                tasks[action.index] = action.task;
            } else {
                tasks.insert(tasks.begin() + action.index, action.task);
            }
            std::cout << "Undid the last change.\n";
            changed = true;
        } else if (command == "help") {
            print_help();
        } else if (command == "quit" || command == "exit") {
            break;
        } else if (!command.empty()) {
            std::cout << "Unknown command. Type help.\n";
        }

        if (changed && !save_tasks(data_file)) {
            std::cerr << "Warning: could not save tasks.txt.\n";
        }
    }
}
