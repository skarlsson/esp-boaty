#pragma once

namespace csi
{
class timer
{
  public:
  timer() : _startTime(0), _timeout(0) {}
  inline bool elapsed() const {  
    if (!enabled())
      return false;
    unsigned long now = millis();
    if (_startTime <= now)
    {
      if ( (unsigned long)(now - _startTime )  < _timeout ) 
        return false;
    }
    else
    {
      if ( (unsigned long)(_startTime - now) < _timeout ) 
        return false;
    }
    return true;
    }
    
    inline void reset(unsigned long timeout) { _startTime = millis(); _timeout=timeout; }
    inline void clear() { _startTime==0; _timeout=0; }
    inline bool enabled() const { return _startTime || _timeout; }
    
    unsigned long remaining() const {
       if (!enabled())
      return 0;
    unsigned long now = millis();
    if (_startTime <= now)
    {
      return _timeout - (unsigned long)(now - _startTime );
    }
    else
    {
      return _timeout - (unsigned long)(_startTime - now); 
    }
    }
    
    private:
    unsigned long _startTime;
    unsigned long _timeout;
};
};

