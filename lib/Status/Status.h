#ifndef SAFETYSTATUS_H
#define SAFETYSTATUS_H

enum class SafetyStatus { Safe, Unsafe, Paused, Error };

class Status {
private:
    SafetyStatus safetyStatus;
    bool running;    
public:
    Status(bool running, SafetyStatus safetyStatus);
    void setSafetyStatus(SafetyStatus status);
    SafetyStatus getSafetyStatus();
    void setRunning(bool isRunning);
    bool isRunning();
};

#endif // SAFETYSTATUS_H