#ifndef __SOFTRTC_H__
#define __SOFTRTC_H__

/// @brief simulated realtime clock
class SoftRTC
{
  private: 
    unsigned long _t_lastupdate_ms;
    unsigned long _t_initial_s;
    unsigned long _t_elapsed_s;

  public:
    SoftRTC() { Init(0); }
    /// @brief init clock
    /// @param t actual time in seconds
    void Init(unsigned long t) { _t_initial_s=t; _t_elapsed_s=0; _t_lastupdate_ms =0;}
    /// @brief get actual time 
    /// @return actual time in seconds
    unsigned long getTimeSeconds() 
    {
      unsigned long _t_current = millis();
      _t_elapsed_s += (_t_current - _t_lastupdate_ms) / 1000; // Convert milliseconds to seconds
      _t_lastupdate_ms = _t_current;
      // Calculate the actual time
      return _t_initial_s + _t_elapsed_s;
    }
};

#endif  // __SOFTRTC_H__