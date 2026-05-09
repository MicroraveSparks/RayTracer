#include "Status.h"

Status::Status(bool running, SafetyStatus safetyStatus) : running(running), safetyStatus(safetyStatus) {};

void Status::setSafetyStatus(SafetyStatus status) {
    this->safetyStatus = status;
}

SafetyStatus Status::getSafetyStatus() {
    return this->safetyStatus;
}

void Status::setRunning(bool isRunning) {
    this->running = isRunning;
}

bool Status::isRunning() {
    return this->running;
}