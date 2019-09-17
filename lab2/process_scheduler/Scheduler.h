#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <vector>
#include <string>
#include "Process.h"
#include "stdio.h"

class Scheduler
{
public:
    Scheduler(int quantum, int max_prios)
    {        
        m_quantum = quantum;
        m_max_prios = max_prios;
    }

    virtual int get(std::vector<Process> & procs) = 0;
    virtual void insert(int pid, std::vector<Process> & procs) = 0;
    virtual std::string get_name() = 0;
    int get_quantum()
    {
        return m_quantum;
    }
protected:
    std::vector<int> m_queue;
    int m_quantum;
    int m_max_prios;
};


class FCFS: public Scheduler
{
public:
    FCFS(int quantum, int max_prios): Scheduler(quantum, max_prios)
    {
        
    }

    int get(std::vector<Process> & procs)
    {
        // get the event with the smallest time stamp
        int pid = m_queue[0];
        m_queue.erase(m_queue.begin());

        return pid;
    }
    void insert(int pid, std::vector<Process> & procs)
    {        
        m_queue.push_back(pid);
    }
    std::string get_name()
    {
        return std::string("FCFS");
    }
    
};


class LCFS: public Scheduler
{
public:
    LCFS(int quantum, int max_prios): Scheduler(quantum, max_prios)
    {
        //todo
    }

    int get(std::vector<Process> & procs)
    {        
        // get the event with the smallest time stamp
        int pid = m_queue.back();
        m_queue.pop_back();

        return pid;
    }
    void insert(int pid, std::vector<Process> & procs)
    {        
        m_queue.push_back(pid);
    }
    std::string get_name()
    {
        return std::string("LCFS");
    }
};

class SRTF: public Scheduler
{
public:
    SRTF(int quantum, int max_prios): Scheduler(quantum, max_prios)
    {
        //todo
    }

    int get(std::vector<Process> & procs)
    {
        int min_pid = m_queue[0];
        int loc = 0;
        // get the event with the least remaining execution time
        for(int i = 1; i < m_queue.size(); i++)
        {
            int this_pid = m_queue[i];
            if (procs[this_pid].get_waytogo() < procs[min_pid].get_waytogo())
            {
                min_pid = this_pid;
                loc = i;
            }
        }

        m_queue.erase(m_queue.begin() + loc);

        return min_pid;
    }
    void insert(int pid, std::vector<Process> & procs)
    {        
       m_queue.push_back(pid);
    }
    std::string get_name()
    {
        return std::string("SRTF");
    }
};


class RoundRobin: public Scheduler
{
public:
    RoundRobin(int quantum, int max_prios): Scheduler(quantum, max_prios)
    {        
    }

    int get(std::vector<Process> & procs)
    {        
        int pid = m_queue[0];
        m_queue.erase(m_queue.begin());

        return pid;       
    }
    void insert(int pid, std::vector<Process> & procs)
    {        
       m_queue.push_back(pid);
    }
    std::string get_name()
    {
        char buffer[100];
        sprintf(buffer, "RR %d", m_quantum);
        return std::string(buffer);

        // return std::string("RR");
    }
};


// struct package
// {
//     int priority;
//     int ts;
// }

// bool func(std::pair<int, int> a, std::pair<int, int> b) 
// {
//     if (a.first == b.first)
//     {
//         return a.second < b.second;
//     }
//     else
//     {
//         return a.first < b.first;
//     }        
// }

class PRIO: public Scheduler
{
public:
    PRIO(int quantum, int max_prios): Scheduler(quantum, max_prios)
    {        
        for (int i = 0; i < max_prios; i++)
        {
            std::vector<int> new_queue_a;
            m_active_queue.push_back(new_queue_a);

            std::vector<int> new_queue_b;
            m_expire_queue.push_back(new_queue_b);            
        }
    }
    int get(std::vector<Process> & procs)
    {        
        bool empty = true;        
        for (int prio = 0; prio < m_max_prios; prio++)
        {
            if (m_active_queue[prio].size() > 0)
            {
                empty = false;                
            }
        }

        if (empty)
        {
            m_active_queue.swap(m_expire_queue);            
        }

        int top_prio = 0;
        for (int prio = 0; prio < m_max_prios; prio++)
        {
            if (m_active_queue[prio].size() > 0)
            {                    
                top_prio = prio;
            }
        }

        int pid = m_active_queue[top_prio].front();
        m_active_queue[top_prio].erase(m_active_queue[top_prio].begin());

        return pid;        
    }
    void insert(int pid, std::vector<Process> & procs)
    {        
        int this_priority = procs[pid].get_dym_PRIO();
        if (this_priority >= 0)
        {
            m_active_queue[this_priority].push_back(pid);
            // update the dynamic priority
            procs[pid].update_dym_PRIO(this_priority - 1);
        }
        else
        {
            int static_prioity = procs[pid].get_PRIO();
            m_expire_queue[static_prioity - 1].push_back(pid);
            // reset the dynamic priority
            procs[pid].update_dym_PRIO(static_prioity - 2);
        }          
    }
    std::string get_name()
    {
        char buffer[100];
        sprintf(buffer, "PRIO %d", m_quantum);
        
        return std::string(buffer);

        // return std::string("PRIO %d", m_quantum);
    }

private:
    std::vector< std::vector<int> > m_active_queue;
    std::vector< std::vector<int> > m_expire_queue;
};



class PREPRIO: public Scheduler
{
public:
    PREPRIO(int quantum, int max_prios): Scheduler(quantum, max_prios)
    {        
        for (int i = 0; i < max_prios; i++)
        {
            std::vector<int> new_queue_a;
            m_active_queue.push_back(new_queue_a);

            std::vector<int> new_queue_b;
            m_expire_queue.push_back(new_queue_b);            
        }
    }
    int get(std::vector<Process> & procs)
    {        
        bool empty = true;        
        for (int prio = 0; prio < m_max_prios; prio++)
        {
            if (m_active_queue[prio].size() > 0)
            {
                empty = false;                
            }
        }

        if (empty)
        {
            m_active_queue.swap(m_expire_queue);            
        }

        empty = true;
        int top_prio = 0;
        for (int prio = 0; prio < m_max_prios; prio++)
        {
            if (m_active_queue[prio].size() > 0)
            {                    
                top_prio = prio;
                empty = false;  
            }
        }
        if (empty)
        {
            printf("EMPTY==============================================!!!\n");
        }

        int pid = m_active_queue[top_prio].front();
        m_active_queue[top_prio].erase(m_active_queue[top_prio].begin());

        return pid;        
    }
    void insert(int pid, std::vector<Process> & procs)
    {        
        int this_priority = procs[pid].get_dym_PRIO();
        if (this_priority >= 0)
        {
            m_active_queue[this_priority].push_back(pid);
            // update the dynamic priority
            procs[pid].update_dym_PRIO(this_priority - 1);
        }
        else
        {
            int static_prioity = procs[pid].get_PRIO();
            m_expire_queue[static_prioity - 1].push_back(pid);
            // reset the dynamic priority
            procs[pid].update_dym_PRIO(static_prioity - 2);
        }          
    }
    std::string get_name()
    {
        char buffer[100];
        sprintf(buffer, "PREPRIO %d", m_quantum);
        // return std::string("PREPRIO ") ;
        return std::string(buffer);
    }

private:
    std::vector< std::vector<int> > m_active_queue;
    std::vector< std::vector<int> > m_expire_queue;
};
#endif