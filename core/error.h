#pragma once

class Error
{
public:
    Error();

    Error(int code);

    virtual ~Error();

    int GetCode() const;

    const char* GetDescription() const;

    static const int Success = 0;

    static const int InvalidValue = 22;

    static const int NoMemory = 12;

    static const int Cancelled = 125;

    static const int NotExecuted = 500;

    bool operator!= (const Error& other) const;

    bool operator== (const Error& other) const;

private:
    int Code;
};
