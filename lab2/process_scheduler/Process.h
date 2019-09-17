#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <algorithm>

class Process
{
private:
    int m_AT;
    int m_TC;
    int m_CB;
    int m_IO;    

    int m_IT;   // IT as in total time spent in I/O blcok state.
    int m_FT;   // FT is the time stamp at which the process finish.
    int m_CW;   // CW is the time this process spend waiting in the ready state. CPU wait time.
    int m_PRIO; // static priority assigned to the process ( note this only has meaning in PRIO/PREPRIO case )

    int m_dym_PRIO; // dynamic priority is defined between [ 0 .. (static_priority-1) ]

    int m_prev_CB;      // the CPU burst from the previous round of running.
    int m_way_to_go;    // the work load yet to finish, measured in time.
    int m_prev_way_to_go;
public:
    int pid;
    int prev_Ready_time;      // the last time this process enters Ready state
    int prev_Start_Run_time;  // the last time this process enters Running state
    int m_prev_prev_CB;
public:    
    Process(int AT, int TC, int CB, int IO, int id, int PRIO)
    {
        m_AT = AT;
        m_TC = TC;
        m_CB = CB;
        m_IO = IO;
        pid = id;
        m_PRIO = PRIO;

        m_way_to_go = m_TC;
        m_IT = 0;
        m_CW = 0;
        m_prev_CB = -1;
        m_dym_PRIO = m_PRIO -1; // With every quantum expiration the dynamic priority decreases by one. When “-1” is reached the prio is reset to (static_priority-1). 
    }
    int get_TC()
    {
        return m_TC;
    }    
    int get_AT()
    {
        return m_AT;
    }        
    int get_CB()
    {
        return m_CB;
    }
    int get_IO()
    {
        return m_IO;
    }
    int get_waytogo()
    {
        return m_way_to_go;
    }
    void update_waytogo(int CPU_burst)
    {
        m_prev_way_to_go = m_way_to_go;
        m_way_to_go -= CPU_burst;
    }
    void restore_waytogo()
    {
        m_way_to_go = m_prev_way_to_go;
    }
        
    int get_IT()
    {
        return m_IT;
    }
    void accum_IT(int IO_burst)
    {
        m_IT += IO_burst;
    }
    
    int get_FT()
    {
        return m_FT;
    }
    void set_FT(int finishing_time)
    {
        m_FT = finishing_time;
    }

    int get_CW()
    {
        return m_CW;
    }
    void accum_CW(int CPU_wait_time)
    {
        m_CW += CPU_wait_time;
    }

    int get_PRIO()
    {
        return m_PRIO;
    }

    int get_prev_CB()
    {
        return m_prev_CB;
    }
    void update_prev_CB(int value)
    {
        m_prev_prev_CB = m_prev_CB;
        m_prev_CB = std::min(value, m_way_to_go);         
    }
    void restore_prev_CB()
    {
        m_prev_CB = m_prev_prev_CB;
    }
    int get_dym_PRIO()
    {
        return m_dym_PRIO;
    }
    int update_dym_PRIO(int value)
    {
        m_dym_PRIO = value;
    }
};

#endif