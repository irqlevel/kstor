#include "main.h"
#include "runnable.h"
#include "threadable.h"
#include "event.h"

class Thread : public Threadable
{
public:
    Thread(Runnable *routine, int& err);
    void Start(Runnable *routine, int& err);
    void Stop();
    void Wait();
    void StopAndWait();
    bool IsStopping() const;
    void *GetId() const;
    virtual ~Thread();
    static void Sleep(int milliseconds);

private:
    Thread() = delete;
    Thread(const Thread& other) = delete;
    Thread& operator=(const Thread& other) = delete;
    Thread& operator=(Thread&& other) = delete;
    static int StartRoutine(void* context);
    int ExecuteRoutine();
    Runnable* Routine;
    void* Task;
    bool Stopping;
    Event CompEvent;
};
