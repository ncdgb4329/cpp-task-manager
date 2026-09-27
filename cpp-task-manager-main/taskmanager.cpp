#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include <queue>
#include <stack>
#include <limits>
#include <algorithm>

using namespace std;

// Task structure

struct Task {
    int id;
    string title;
    string description;
    string category;
    bool done = false;
};

// Category tree, root to child, taks ids refer to category

struct CategoryNode {
    string name;
    vector<CategoryNode*> children;
    vector<int> taskIds;
};

// Graph for dependencies is dependencyGraph[taskId] and gives a list of prerequisite task IDs

unordered_map<int, vector<int>> dependencyGraph;

// main program with list, hash table for map id, queue for tasks by id, and a category tree
// stack is supposed to give undos for add/deletes (DO THIS)


list<Task> tasks;
unordered_map<int, list<Task>::iterator> taskById;
queue<int> dailyQueue;
CategoryNode* rootCategory = nullptr;

enum class ActionType {
    ADD_TASK,
    DELETE_TASK
};

struct UndoAction {
    ActionType type;
    Task taskBackup;
};

stack<UndoAction> undoStack;

void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

CategoryNode* findCategory(CategoryNode* root, const string& name) {
    if (!root) return nullptr;
    for (CategoryNode* child : root->children) {
        if (child->name == name) {
            return child;
        }
    }
    return nullptr;
}

CategoryNode* getOrCreateCategory(CategoryNode* root, const string& name) {
    CategoryNode* existing = findCategory(root, name);
    if (existing) return existing;

    CategoryNode* node = new CategoryNode();
    node->name = name;
    root->children.push_back(node);
    return node;
}

void removeTaskFromCategory(int id) {
    if (!rootCategory) return;
    for (CategoryNode* child : rootCategory->children) {
        auto& vec = child->taskIds;
        vec.erase(remove(vec.begin(), vec.end(), id), vec.end());
    }
}

void printCategoryTree(CategoryNode* root, int depth = 0) {
    if (!root) return;
    for (int i = 0; i < depth; ++i) cout << "  ";
    cout << "- " << root->name << "\n";
    for (CategoryNode* child : root->children) {
        printCategoryTree(child, depth + 1);
    }
}

bool prerequisitesSatisfied(int taskId) {
    auto it = dependencyGraph.find(taskId);
    if (it == dependencyGraph.end()) return true;

    for (int prereqId : it->second) {
        auto pit = taskById.find(prereqId);
        if (pit == taskById.end()) {
            return false;
        }
        if (!pit->second->done) return false;
    }
    return true;
}

void printTask(const Task& t) {
    cout << "[" << t.id << "] "
        << (t.done ? "(DONE) " : "(TODO) ")
        << t.title << " | category: " << t.category << "\n";
    if (!t.description.empty()) {
        cout << "    " << t.description << "\n";
    }
}

void addTask(int& nextId) {
    Task t;
    t.id = nextId++;

    cout << "Enter task title: ";
    clearInput();
    getline(cin, t.title);

    cout << "Enter task description: ";
    getline(cin, t.description);

    cout << "Enter category name (leave blank for 'Uncategorized'): ";
    string cat;
    getline(cin, cat);
    if (cat.empty()) cat = "Uncategorized";
    t.category = cat;

    // Insert into lis
    tasks.push_back(t);
    auto it = tasks.end();
    --it;

    // Insert into table
    taskById[t.id] = it;

    // Insert into tree

    CategoryNode* catNode = getOrCreateCategory(rootCategory, cat);
    catNode->taskIds.push_back(t.id);

    // undo

    UndoAction ua;
    ua.type = ActionType::ADD_TASK;
    ua.taskBackup = t;
    undoStack.push(ua);

    cout << "Task added with ID " << t.id << ".\n";
}

void listTasks() {
    if (tasks.empty()) {
        cout << "No tasks.\n";
        return;
    }
    for (const auto& t : tasks) {
        printTask(t);
    }
}

void deleteTask() {
    cout << "Enter task ID to delete:";
    int id;
    cin >> id;
    if (!cin) {
        clearInput();
        cout << "Invalid input.\n";
        return;
    }

    auto it = taskById.find(id);
    if (it == taskById.end()) {
        cout << "Task with ID " << id << " not found\n";
        return;
    }

    // deletes and cleanup

    Task backup = *(it->second);
    removeTaskFromCategory(id);
    dependencyGraph.erase(id);
    tasks.erase(it->second);
    taskById.erase(it);
    UndoAction ua;
    ua.type = ActionType::DELETE_TASK;
    ua.taskBackup = backup;
    undoStack.push(ua);

    cout << "Task " << id << " deleted\n";
}

void markTaskDone() {
    cout << "Enter task ID to mark as done:";
    int id;
    cin >> id;
    if (!cin) {
        clearInput();
        cout << "Invalid input\n";
        return;
    }

    auto it = taskById.find(id);
    if (it == taskById.end()) {
        cout << "Task with ID " << id << " not found\n";
        return;
    }

    if (!prerequisitesSatisfied(id)) {
        cout << "Cannot mark task " << id << " as done"
            << "Not all prerequisites are complete\n";
        return;
    }

    it->second->done = true;
    cout << "Task " << id << " marked as done\n";
}

// Assign a task to a category (just write it to reassign leafs)

void assignTaskToCategory() {
    cout << "Enter task ID: ";
    int id;
    cin >> id;
    if (!cin) {
        clearInput();
        cout << "Invalid input\n";
        return;
    }
    auto it = taskById.find(id);
    if (it == taskById.end()) {
        cout << "Task with ID " << id << " not found\n";
        return;
    }

    cout << "Enter new category name: ";
    string cat;
    clearInput();
    getline(cin, cat);
    if (cat.empty()) cat = "Uncategorized";

    removeTaskFromCategory(id);
    CategoryNode* catNode = getOrCreateCategory(rootCategory, cat);
    catNode->taskIds.push_back(id);

    it->second->category = cat;
    cout << "Task " << id << " assigned to category '" << cat << "'\n";
}

void showTasksByCategory() {
    cout << "Category tree:\n";
    printCategoryTree(rootCategory);

    cout << "\nEnter category name to view tasks (must be exact match): ";
    string cat;
    clearInput();
    getline(cin, cat);

    CategoryNode* node = findCategory(rootCategory, cat);
    if (!node) {
        cout << "Category not found.\n";
        return;
    }

    if (node->taskIds.empty()) {
        cout << "No tasks in this category.\n";
        return;
    }

    cout << "Tasks in category '" << cat << "':\n";
    for (int id : node->taskIds) {
        auto it = taskById.find(id);
        if (it != taskById.end()) {
            printTask(*(it->second));
        }
    }
}

// put the dependencies structured as A before B

void addDependency() {
    int a, b;
    cout << "Enter ID of prerequisite task A: ";
    cin >> a;
    if (!cin) {
        clearInput();
        cout << "Invalid input.\n";
        return;
    }
    cout << "Enter ID of dependent task B: ";
    cin >> b;
    if (!cin) {
        clearInput();
        cout << "Invalid input\n";
        return;
    }

    if (taskById.find(a) == taskById.end() ||
        taskById.find(b) == taskById.end()) {
        cout << "Both tasks must exist\n";
        return;
    }

    dependencyGraph[b].push_back(a);
    cout << "Dependency added: Task " << a << " must be done before Task " << b << "!\n";
}

void showDependencies() {
    cout << "Enter task ID to view its prerequisites: ";
    int id;
    cin >> id;
    if (!cin) {
        clearInput();
        cout << "Invalid input\n";
        return;
    }

    auto it = dependencyGraph.find(id);
    if (it == dependencyGraph.end() || it->second.empty()) {
        cout << "Task " << id << " has no prerequisites.\n";
        return;
    }

    cout << "Task " << id << " depends on:\n";
    for (int prereq : it->second) {
        auto pit = taskById.find(prereq);
        if (pit != taskById.end()) {
            printTask(*(pit->second));
        }
        else {
            cout << "  [missing task " << prereq << "]\n";
        }
    }
}

void addTaskToDailyQueue() {
    cout << "Enter task ID to add to daily queue: ";
    int id;
    cin >> id;
    if (!cin) {
        clearInput();
        cout << "Invalid input\n";
        return;
    }
    if (taskById.find(id) == taskById.end()) {
        cout << "Task with ID " << id << " not found.\n";
        return;
    }
    dailyQueue.push(id);
    cout << "Task " << id << " added to daily queue\n";
}

void viewDailyQueue() {
    if (dailyQueue.empty()) {
        cout << "Daily queue is empty\n";
        return;
    }

    cout << "Daily queue (in order):\n";
    queue<int> temp = dailyQueue;
    while (!temp.empty()) {
        int id = temp.front();
        temp.pop();
        auto it = taskById.find(id);
        if (it != taskById.end()) {
            printTask(*(it->second));
        }
        else {
            cout << "[unknown task " << id << "]\n";
        }
    }
}

void processNextDailyTask() {
    if (dailyQueue.empty()) {
        cout << "Daily queue is empty\n";
        return;
    }
    int id = dailyQueue.front();
    dailyQueue.pop();

    auto it = taskById.find(id);
    if (it == taskById.end()) {
        cout << "Next task in queue no longer exists (ID " << id << ").\n";
        return;
    }

    cout << "Next daily task:\n";
    printTask(*(it->second));

    cout << "Mark it as done? (y/n): ";
    char c;
    cin >> c;
    if (c == 'y' || c == 'Y') {
        if (!prerequisitesSatisfied(id)) {
            cout << "Cannot mark as done; prerequisites not satisfied.\n";
        }
        else {
            it->second->done = true;
            cout << "Task " << id << " marked as done.\n";
        }
    }
    else {
        cout << "Task left as-is\n";
    }
}

void undoLastAction() {
    if (undoStack.empty()) {
        cout << "Nothing to undo\n";
        return;
    }

    UndoAction ua = undoStack.top();
    undoStack.pop();

    if (ua.type == ActionType::ADD_TASK) {
        int id = ua.taskBackup.id;
        auto it = taskById.find(id);
        if (it != taskById.end()) {
            removeTaskFromCategory(id);
            tasks.erase(it->second);
            taskById.erase(it);
        }
        cout << "Undo: added task " << id << " removed.\n";
    }

    else if (ua.type == ActionType::DELETE_TASK) {
        Task t = ua.taskBackup;
        tasks.push_back(t);
        auto it = tasks.end();
        --it;
        taskById[t.id] = it;

        CategoryNode* catNode = getOrCreateCategory(rootCategory, t.category);
        catNode->taskIds.push_back(t.id);

        cout << "Undo: deleted task " << t.id << " restored.\n";
    }
}

// linear search through titles
void searchTasksByTitle() {
    cout << "Enter text to search in task titles: ";
    string query;
    clearInput();
    getline(cin, query);

    if (query.empty()) {
        cout << "Empty\n";
        return;
    }

    bool found = false;
    for (const auto& t : tasks) {
        if (t.title.find(query) != string::npos) {
            printTask(t);
            found = true;
        }
    }

    if (!found) {
        cout << "No tasks found matching \"" << query << "\".\n";
    }
}

// sort tasks by title by using sort on a vector (DO OR DELETE)

void listTasksSortedByTitle() {
    if (tasks.empty()) {
        cout << "No tasks\n";
        return;
    }

    vector<Task> v(tasks.begin(), tasks.end());

    sort(v.begin(), v.end(), [](const Task& a, const Task& b) {
        return a.title < b.title;
        });

    cout << "Tasks sorted by title:\n";
    for (const auto& t : v) {
        printTask(t);
    }
}

void showMenu() {
    cout << "TASK MANAGER\n";
    cout << "1. Add task\n";
    cout << "2. View all tasks\n";
    cout << "3. Delete task\n";
    cout << "4. Mark task as done\n";
    cout << "5. Assign task to category\n";
    cout << "6. Show tasks by category\n";
    cout << "7. Add dependency (A before B)\n";
    cout << "8. Show dependencies for a task\n";
    cout << "9. Add task to daily queue\n";
    cout << "10. View daily queue\n";
    cout << "11. Process next daily task\n";
    cout << "12. Undo last action\n";
    cout << "13. Search tasks by title\n";
    cout << "14. View tasks sorted by title\n";
    cout << "0. Exit\n";
}

int main() {
    rootCategory = new CategoryNode();
    rootCategory->name = "All categories";
    int nextId = 1;
    int choice = -1;

    while (true) {
        showMenu();
        cin >> choice;
        if (!cin) {
            clearInput();
            cout << "Invalid input\n";
            continue;
        }

        switch (choice) {
        case 1:
            addTask(nextId);
            break;
        case 2:
            listTasks();
            break;
        case 3:
            deleteTask();
            break;
        case 4:
            markTaskDone();
            break;
        case 5:
            assignTaskToCategory();
            break;
        case 6:
            showTasksByCategory();
            break;
        case 7:
            addDependency();
            break;
        case 8:
            showDependencies();
            break;
        case 9:
            addTaskToDailyQueue();
            break;
        case 10:
            viewDailyQueue();
            break;
        case 11:
            processNextDailyTask();
            break;
        case 12:
            undoLastAction();
            break;
        case 13:
            searchTasksByTitle();
            break;
        case 14:
            listTasksSortedByTitle();
            break;
        case 0:
            cout << "Exiting...\n";
            return 0;
        default:
            cout << "Unknown option\n";
            break;
        }
    }

    return 0;
}
