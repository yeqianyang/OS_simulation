#ifndef __DES_H__
#define __DES_H__
#include <queue>
#include <vector>
#include <iostream>

using namespace std;


enum TRANSITIONS
{
    Ready2Running,
    Running2Block,
    Block2Ready,
    Create2Ready,
    Running2Ready,
    Done
};

class Event
{
public:
    int timestamp;          // rougly means the time this event is finised running and ready for next event.
    int time_created;       // the time this event is create.
    int pid;                // process id.
    int transition;

    Event(int ts, int tc, int id, int trans)
    {
        timestamp = ts;
        time_created = tc;
        pid = id;
        transition = trans;
    }
};


class DES
{
private:
    std::vector<Event> m_event_queue;

public:
    DES()
    {
        //todo
    }

    Event get_event()
    {
        // get the event with the smallest time stamp
        Event e = m_event_queue[0];
        m_event_queue.erase(m_event_queue.begin());

        return e;
    }
    void insert_event(Event e)
    {
        // insert event according to their time stamp
        std::vector<Event>::iterator loc = m_event_queue.begin();
        loc += m_event_queue.size();
        for (std::vector<Event>::iterator iter = m_event_queue.begin(); iter < m_event_queue.end(); iter++)
        {
            if (e.timestamp < iter->timestamp)
            {
                loc = iter;
                break;
            }
        }
        m_event_queue.insert(loc, e);
    }
    void push_event_to_front(Event e)
    {
        // std::cout <<  "m_event_queue.size():" << m_event_queue.size() << std::endl;
        if (m_event_queue.size() == 0)
        {
            m_event_queue.push_back(e);
        }
        else
        {
            // insert event according to their time stamp
            std::vector<Event>::iterator loc = m_event_queue.begin();        
            m_event_queue.insert(loc, e);    
        }
        
        
    }
    void remove_future_events(int pid, int ts)
    {
        while(true)
        {
            bool found = false;
            int i = 0;
            for (std::vector<Event>::iterator iter = m_event_queue.begin(); iter < m_event_queue.end(); iter++, i++)
            {
                if (iter->pid == pid && iter->timestamp > ts && iter->transition != Ready2Running)
                {                
                    found = true;
                    break;
                }
            }
            if (found)
            {
                m_event_queue.erase(m_event_queue.begin() + i);
            }            
            // std::cout <<  "after delete: m_event_queue.size() is " << m_event_queue.size() << std::endl;
            if (!found || m_event_queue.size() <= 0)
            {
                break;
            }
        }
    }
    bool find_by_pid_ts(int pid, int ts)
    {
        for (std::vector<Event>::iterator iter = m_event_queue.begin(); iter < m_event_queue.end(); iter++)
        {
            if (iter->pid == pid && iter->timestamp == ts)
            {                
                return true;
            }
        }
        return false;
    }
    bool is_empty()
    {
        return m_event_queue.size() == 0;
    }

};

#endif
