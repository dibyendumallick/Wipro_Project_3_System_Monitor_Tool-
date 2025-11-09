Abstract
The System Monitor Tool is a Linux-based command-line utility developed in C++ to track and manage system performance in real time. It displays critical system metrics such as CPU usage, Memory utilization, System uptime, and Active processes. Additionally, it allows the user to sort processes dynamically by CPU or memory usage and even terminate (kill) unwanted processes directly from the interface.

Objectives
• Develop a real-time System Monitor Tool that analyzes CPU, memory, and process statistics.
• Implement process-level control (e.g., kill functionality).
• Enhance knowledge of Linux internals and /proc filesystem.
• Utilize C++ multithreading and file I/O for performance monitoring.
• Provide an interactive terminal interface for better user experience.

Tools and Technologies Used
Language: C++ (C++17 standard)
Operating System: Linux (Ubuntu)
Libraries: <iostream>, <fstream>, <dirent.h>, <unistd.h>, <thread>, <chrono>
Concepts Used: File I/O, Process handling, Multithreading, System calls, Sorting algorithms
Build Tool: g++ Compiler
