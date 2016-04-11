#pragma once

#include "main.h"
#include "shared_ptr.h"

class KObj
{
public:
  KObj(int value);
  int GetValue();
  virtual ~KObj();
private:
  int Value;
};

typedef shared_ptr<KObj> KObjRef;
