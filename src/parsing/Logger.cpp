#include "Logger.hpp"

std::ofstream Logger::logFile;

void Logger::init(const std::string &logFilePath) {
    struct stat fileStat;
    if (stat(logFilePath.c_str(), &fileStat) != 0) {
        throw std::runtime_error("Log file could not be accessed");
    }
    if (!(fileStat.st_mode & S_IRUSR)) {
        throw std::runtime_error("Log file is not readable");
    }
    if (!(fileStat.st_mode & S_IWUSR)) {
        throw std::runtime_error("Log file is not writable");
    }
    logFile.open(logFilePath.c_str(), std::ios::app);
    if (!logFile.is_open()) {
        throw std::runtime_error("Log file not accessible");
    }
}

void Logger::logError(const std::string &message) {
    if (logFile.is_open()) {
        logFile << "Error: " << message << std::endl;
    } else {
        std::cerr << "Error: " << message << std::endl;
    }
}

void Logger::close() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

bool Logger::isLogFileAccessible() {
    return logFile.is_open();
}