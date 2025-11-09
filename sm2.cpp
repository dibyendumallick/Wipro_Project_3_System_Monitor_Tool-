#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <csignal>
#include <dirent.h>
#include <unistd.h>
#include <map>

using namespace std;

struct Process {
    int pid;
    string name;
    double cpuUsage;
    double prevCpuUsage;
    double memUsage;
};

// Function to read CPU utilization (overall)
double getCPUUsage() {
    ifstream f1("/proc/stat");
    string line1; getline(f1, line1); f1.close();

    string cpu;
    unsigned long long user1, nice1, sys1, idle1, iow1, irq1, sirq1, steal1;
    istringstream s1(line1);
    s1 >> cpu >> user1 >> nice1 >> sys1 >> idle1 >> iow1 >> irq1 >> sirq1 >> steal1;
    unsigned long long total1 = user1 + nice1 + sys1 + idle1 + iow1 + irq1 + sirq1 + steal1;
    unsigned long long idleAll1 = idle1 + iow1;

    this_thread::sleep_for(chrono::milliseconds(500));

    ifstream f2("/proc/stat");
    string line2; getline(f2, line2); f2.close();

    unsigned long long user2, nice2, sys2, idle2, iow2, irq2, sirq2, steal2;
    istringstream s2(line2);
    s2 >> cpu >> user2 >> nice2 >> sys2 >> idle2 >> iow2 >> irq2 >> sirq2 >> steal2;
    unsigned long long total2 = user2 + nice2 + sys2 + idle2 + iow2 + irq2 + sirq2 + steal2;
    unsigned long long idleAll2 = idle2 + iow2;

    double diffIdle  = (double)(idleAll2 - idleAll1);
    double diffTotal = (double)(total2 - total1);
    if (diffTotal <= 0) return 0.0;

    double usage = (1.0 - diffIdle / diffTotal) * 100.0;
    return usage;
}

// Function to read Memory utilization (overall)
double getMemoryUsage() {
    ifstream file("/proc/meminfo");
    string label, unit;
    long totalMem = 0, freeMem = 0;
    file >> label >> totalMem >> unit;
    file >> label >> freeMem >> unit;
    file.close();
    return 100.0 * (totalMem - freeMem) / totalMem;
}

// Function to read uptime
double getUptime() {
    ifstream file("/proc/uptime");
    double uptime;
    file >> uptime;
    file.close();
    return uptime / 3600;
}

// Function to generate ASCII progress bar
string getBar(double percent) {
    int bars = (int)(percent / 5);
    string bar = "[";
    for (int i = 0; i < 20; ++i)
        bar += (i < bars ? '#' : '-');
    bar += "] ";
    return bar + to_string((int)percent) + "%";
}

// Function to get per-process info
vector<Process> getProcesses(map<int, double>& prevCpuMap) {
    vector<Process> processes;
    DIR* dir = opendir("/proc");
    if (!dir) return processes;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (isdigit(entry->d_name[0])) {
            int pid = stoi(entry->d_name);
            string statPath = "/proc/" + string(entry->d_name) + "/stat";
            string statusPath = "/proc/" + string(entry->d_name) + "/status";

            ifstream statFile(statPath);
            ifstream statusFile(statusPath);
            if (!statFile.is_open() || !statusFile.is_open()) continue;

            string comm, token;
            long utime, stime;
            statFile >> pid >> comm;
            for (int i = 0; i < 11; ++i) statFile >> token;
            statFile >> utime >> stime;

            long total_time = utime + stime;
            double prevTotal = prevCpuMap[pid];
            double cpuUsage = total_time - prevTotal;
            prevCpuMap[pid] = total_time;

            string line;
            long vmrss = 0;
            while (getline(statusFile, line)) {
                if (line.find("VmRSS:") == 0) {
                    istringstream ss(line);
                    string label, unit;
                    ss >> label >> vmrss >> unit;
                    break;
                }
            }
            double memUsage = vmrss / 1024.0; // MB

            comm.erase(remove(comm.begin(), comm.end(), '('), comm.end());
            comm.erase(remove(comm.begin(), comm.end(), ')'), comm.end());

            processes.push_back({pid, comm, cpuUsage, prevTotal, memUsage});
        }
    }
    closedir(dir);
    return processes;
}

// Display process list with trends
void displayProcesses(vector<Process>& processes, bool sortByMem = false) {
    if (sortByMem) {
        sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
            return a.memUsage > b.memUsage;
        });
    } else {
        sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
            return a.cpuUsage > b.cpuUsage;
        });
    }

    cout << left << setw(8) << "PID"
         << setw(25) << "Process Name"
         << setw(12) << "CPU (Δ)"
         << setw(12) << "Trend"
         << setw(12) << "MEM (MB)" << endl;
    cout << string(70, '-') << endl;

    for (int i = 0; i < min((int)processes.size(), 10); ++i) {
        string trendArrow = "→";
        if (processes[i].cpuUsage > processes[i].prevCpuUsage + 0.05)
            trendArrow = "↑";
        else if (processes[i].cpuUsage < processes[i].prevCpuUsage - 0.05)
            trendArrow = "↓";

        if (i == 0)
            cout << "\033[1;31m"; // highlight top

        cout << left << setw(8) << processes[i].pid
             << setw(25) << processes[i].name
             << setw(12) << fixed << setprecision(2) << processes[i].cpuUsage
             << setw(12) << trendArrow
             << setw(12) << fixed << setprecision(2) << processes[i].memUsage
             << endl;

        if (i == 0)
            cout << "\033[0m";
    }
}

// Kill process safely
void killProcess(int pid) {
    if (kill(pid, SIGTERM) == 0)
        cout << "Process " << pid << " terminated successfully.\n";
    else
        perror("Error terminating process");
}

int main() {
    map<int, double> prevCpuMap;
    bool sortByMem = false;

    while (true) {
        system("clear");

        double cpuUsage = getCPUUsage();
        double memUsage = getMemoryUsage();
        double uptime = getUptime();

        double health = 100 - (cpuUsage * 0.4 + memUsage * 0.6);
        if (health < 0) health = 0;

        cout << "==================== SYSTEM MONITOR TOOL ====================" << endl;
        cout << "CPU Usage:   " << getBar(cpuUsage) << endl;
        cout << "Memory Usage:" << getBar(memUsage) << endl;
        cout << "System Uptime: " << fixed << setprecision(2) << uptime << " hours" << endl;
        cout << "System Health Index: " << fixed << setprecision(2) << health << "%" << endl;
        cout << "=============================================================" << endl;

        vector<Process> processes = getProcesses(prevCpuMap);

        displayProcesses(processes, sortByMem);

        cout << "\nOptions: [1] Sort by CPU  [2] Sort by Memory  [PID] Kill process  [0] Refresh\n> ";
        int choice;
        if (!(cin >> choice)) { cin.clear(); cin.ignore(10000, '\n'); choice = 0; }

        if (choice == 1) sortByMem = false;
        else if (choice == 2) sortByMem = true;
        else if (choice > 2) killProcess(choice);

        this_thread::sleep_for(chrono::seconds(3));
    }

    return 0;
}
