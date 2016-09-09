#pragma once

extern "C" {
#include "user_interface.h"
}


// there can only be seven of those
// void (*functionName)(void *pArg)
namespace csi
{
class os_timer
{
  public:
  os_timer() {};
  void init(unsigned int delay, os_timer_func_t *pFunction, void* user_ptr){
    os_timer_setfn(&_myTimer, pFunction, user_ptr);    
    os_timer_arm(&_myTimer, delay, true); 
  }
  private:
  os_timer_t _myTimer;
};
};

