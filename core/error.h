#pragma once

class Error
{
public:
    Error();

    Error(int code);

    virtual ~Error();

    int GetCode() const;

    void SetCode(int code);

    const char* GetDescription() const;

    static const int Success = 0;

    static const int InvalidValue = -22;

    static const int NoMemory = -12;

    static const int Cancelled = -125;

    static const int NotExecuted = -500;

    static const int InvalidState = -501;

    static const int Unsuccessful = -502;

    static const int NotImplemented = -503;

    static const int UnknownCode = -504;

    bool operator!= (const Error& other) const;

    bool operator== (const Error& other) const;

private:
    int Code;
};
