#include "framework.h"
#include "SuperSimpleTime.h"

namespace SuperSimpleTime
{
    static ULONGLONG _time;

    void SuperSimpleTime::Initialize()
    {
        _time = GetTickCount64();
    }

    void SuperSimpleTime::Update()
    {
        _time = GetTickCount64();
    }

    double SuperSimpleTime::GetDeltaTime()
    {
        return (GetTickCount64() - _time) / 1000.0;
    }
}
