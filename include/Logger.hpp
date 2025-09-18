#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <sys/stat.h> // for stat
#include <unistd.h> // for getcwd

class Logger {
public:
    static void init(const std::string &logFilePath);
    static void logError(const std::string &message);
    static void close();
    static bool isLogFileAccessible();

private:
    static std::ofstream logFile;
};

#endif // LOGGER_HPP