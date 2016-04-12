#include "main.h"
#include "runnable.h"
#include "threadable.h"
#include "event.h"

class Thread : public Threadable
{
public:
    Thread();
    Thread(const RunnableRef routine, int& err);
    void Start(const RunnableRef routine, int& err);
    void Stop();
    void Wait();
    void StopAndWait();
    bool IsStopping() const;
    void *GetId() const;
    virtual ~Thread();
    static void Sleep(int milliseconds);
private:
    static int StartRoutine(void* context);
    int ExecuteRoutine();
    RunnableRef Routine;
    Event CompEvent;
    void* Task;
    bool Stopping;
};
