#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__
#include <vector>
#include <cstdlib> 
#include <climits>
#include "stdio.h"
#include <map>
#include <vector>
#include <algorithm>


class IO_Operation
{
public:
    int op_id;
    int track_id;
    int arrival_time;
    int start_time;
    int end_time;
    bool b_added;
    IO_Operation(int _id, int _arrival_time, int _track_id)
    {
        op_id = _id;
        arrival_time = _arrival_time;
        track_id = _track_id; 
        b_added = false;       
    }
};


class Scheduler
{
protected:
    std::vector<IO_Operation> & m_io_ops;
    std::vector<int> m_io_op_idx;

public:
    Scheduler(std::vector<IO_Operation> & _io_ops): m_io_ops(_io_ops)
    {
        for (int i = 0 ; i < m_io_ops.size(); i++)
        {
            m_io_op_idx.push_back(i);
        }
    }
	virtual int get(const int head, int & current_time) = 0;
    virtual bool is_empty() = 0;
    virtual void add(int op_idx)
    {

    }
};



class FIFO: public Scheduler
{
    
public:
    FIFO(std::vector<IO_Operation> & _io_ops):Scheduler(_io_ops)
    {

    }

	int get(const int head, int & current_time)
    {
       	int op_id = m_io_op_idx.front();
        m_io_op_idx.erase(m_io_op_idx.begin());	
        return op_id;
    }

    bool is_empty()
    {
        return m_io_op_idx.size() == 0;
    }
	
   
};


class SSTF: public Scheduler
{
    
public:
    SSTF(std::vector<IO_Operation> & _io_ops):Scheduler(_io_ops)
    {

    }

	int get(const int head, int & current_time)
    {
        int min_op_id = -1;
        int min_dist = INT_MAX;
        int pointer = 0;

            
        for ( int i = 0; i < m_io_op_idx.size(); i++)
        {
            int this_op_idx = m_io_op_idx[i];
            if (m_io_ops[this_op_idx].arrival_time <= current_time)
            {
                int this_dist = abs(head - m_io_ops[this_op_idx].track_id);
                if (this_dist < min_dist)
                {
                    min_dist = this_dist;
                    min_op_id = this_op_idx;
                    pointer = i;
                }
            }
        }

        if (min_op_id <0)
        {
            // then find the one closes in time?
            current_time = m_io_ops[m_io_op_idx[0]].arrival_time;
            return -1;
        }
        m_io_op_idx.erase(m_io_op_idx.begin() + pointer);

        return min_op_id;	
    }

    bool is_empty()
    {
        return m_io_op_idx.size() == 0;
    }
};


class LOOK: public Scheduler
{
private:
    int direction;

public:
    LOOK(std::vector<IO_Operation> & _io_ops):Scheduler(_io_ops)
    {
        direction = 1;  // 1 is increment, -1 is decrement
    }

	int get(const int head, int & current_time)
    {


        std::map<int,int> track_2_io_id;
        std::vector<int> track_ids;

            
        for ( int i = 0; i < m_io_op_idx.size(); i++)
        {
            int this_op_idx = m_io_op_idx[i];
            if (m_io_ops[this_op_idx].arrival_time <= current_time)
            {
                int this_track_id = m_io_ops[this_op_idx].track_id;
                
                if (this_track_id == head)
                {
                    m_io_op_idx.erase(m_io_op_idx.begin() + i);
                    return this_op_idx;	
                }

                if (track_2_io_id.find(this_track_id) == track_2_io_id.end())
                {
                    track_ids.push_back(this_track_id);    
                    track_2_io_id[m_io_ops[this_op_idx].track_id] = this_op_idx;
                }
            }
        }

        // if (track_ids.size() > 0)
        // {
        //     printf("	Get: (");
        //     for (int i = 0 ;i < track_ids.size(); i++)
        //     {
        //         printf("%d:%d ", track_2_io_id[track_ids[i]], abs(track_ids[i] - head));
        //     }
        //     printf(") dir=%d\n", direction);
        // }
        
        if (track_ids.size() == 0)
        {
            current_time = m_io_ops[ m_io_op_idx[0] ].arrival_time;
            return -1;
        }

        track_ids.push_back(head);




        
        std::sort(track_ids.begin(), track_ids.end());
        // upstair
        std::vector<int>::iterator head_pos = std::find (track_ids.begin(), track_ids.end(), head);
        std::vector<int>::iterator res_iter;
        if (direction == 1 && head_pos != (track_ids.begin() + track_ids.size() - 1))
        {
            res_iter = head_pos + 1;
        }
        else if(direction == 1 && head_pos == (track_ids.begin() + track_ids.size() - 1))
        {
            res_iter = head_pos - 1;
            direction = -1;
        }
        else if(direction == -1 && head_pos != track_ids.begin())
        {
            res_iter = head_pos - 1;
        }
        else if(direction == -1 && head_pos == track_ids.begin())
        {
            res_iter = head_pos + 1;
            direction = 1;
        }

        int track_id = *res_iter;
        int op_idx = track_2_io_id[track_id];
        m_io_op_idx.erase(std::find (m_io_op_idx.begin(), m_io_op_idx.end(), op_idx));

        return op_idx;	
    }

    bool is_empty()
    {
        return m_io_op_idx.size() == 0;
    }
};




class CLOOK: public Scheduler
{
private:
    int direction;

public:
    CLOOK(std::vector<IO_Operation> & _io_ops):Scheduler(_io_ops)
    {
        direction = 1;  // 1 is increment, -1 is decrement
    }

	int get(const int head, int & current_time)
    {


        std::map<int,int> track_2_io_id;
        std::vector<int> track_ids;

            
        for ( int i = 0; i < m_io_op_idx.size(); i++)
        {
            int this_op_idx = m_io_op_idx[i];
            if (m_io_ops[this_op_idx].arrival_time <= current_time)
            {
                int this_track_id = m_io_ops[this_op_idx].track_id;
                
                if (this_track_id == head)
                {
                    m_io_op_idx.erase(m_io_op_idx.begin() + i);
                    return this_op_idx;	
                }

                if (track_2_io_id.find(this_track_id) == track_2_io_id.end())
                {
                    track_ids.push_back(this_track_id);    
                    track_2_io_id[m_io_ops[this_op_idx].track_id] = this_op_idx;
                }
            }
        }

        // if (track_ids.size() > 0)
        // {
        //     printf("	Get: (");
        //     for (int i = 0 ;i < track_ids.size(); i++)
        //     {
        //         printf("%d:%d ", track_2_io_id[track_ids[i]], abs(track_ids[i] - head));
        //     }
        //     printf(") dir=%d\n", direction);
        // }
        
        if (track_ids.size() == 0)
        {
            current_time = m_io_ops[ m_io_op_idx[0] ].arrival_time;
            return -1;
        }

        track_ids.push_back(head);




        
        std::sort(track_ids.begin(), track_ids.end());
        // upstair
        std::vector<int>::iterator head_pos = std::find (track_ids.begin(), track_ids.end(), head);
        std::vector<int>::iterator res_iter;
        if (direction == 1 && head_pos != (track_ids.begin() + track_ids.size() - 1))
        {
            res_iter = head_pos + 1;
        }
        else if(direction == 1 && head_pos == (track_ids.begin() + track_ids.size() - 1))
        {
            res_iter = track_ids.begin();
            // printf("\t\t\t\tgo to bottom\n");
        }      

        int track_id = *res_iter;
        int op_idx = track_2_io_id[track_id];
        m_io_op_idx.erase(std::find (m_io_op_idx.begin(), m_io_op_idx.end(), op_idx));

        return op_idx;	
    }

    bool is_empty()
    {
        return m_io_op_idx.size() == 0;
    }
};



class FLOOK: public Scheduler
{
private:
    int direction;
    int active_queue_idx;
    int add_queue_idx;    
    std::vector< std::vector<int> > queues;    
public:
    FLOOK(std::vector<IO_Operation> & _io_ops):Scheduler(_io_ops)
    {
        direction = 1;  // 1 is increment, -1 is decrement
        active_queue_idx = 0;
        add_queue_idx = 0;
        for (int i = 0 ; i < 2; i++)
        {
            std::vector<int> new_queue;
            queues.push_back(new_queue);
        }
    }
    void add(int op_idx)
    {
        queues[add_queue_idx].push_back(op_idx);
        m_io_ops[op_idx].b_added = true;
        // printf("    Q=%d (", add_queue_idx);
        // for (int i = 0 ; i < queues[add_queue_idx].size(); i++)
        // {
        //     printf("%d:%d ", queues[add_queue_idx][i], m_io_ops[queues[add_queue_idx][i]].track_id);
        // }
        // printf(")\n");
         
        if (add_queue_idx == active_queue_idx)
        {
            add_queue_idx = 1 - active_queue_idx;
        }
    }
	int get(const int head, int & current_time)
    {
        std::map<int,int> track_2_io_id;
        std::vector<int> track_ids;

        if (queues[active_queue_idx].size() == 0)
        {            
            active_queue_idx = 1 - active_queue_idx;
            add_queue_idx = 1 - active_queue_idx;
            // printf("switch queue, active:%d, add:%d\n", active_queue_idx, add_queue_idx);
        }

        for (int i = 0; i < queues[active_queue_idx].size(); i++)
        {
            int this_op_idx = queues[active_queue_idx][i];

            int this_track_id = m_io_ops[this_op_idx].track_id;
            
            if (this_track_id == head)
            {
                queues[active_queue_idx].erase(queues[active_queue_idx].begin() + i);
                m_io_op_idx.erase(std::find (m_io_op_idx.begin(), m_io_op_idx.end(), this_op_idx));
                return this_op_idx;	
            }

            if (track_2_io_id.find(this_track_id) == track_2_io_id.end())
            {
                track_ids.push_back(this_track_id);    
                track_2_io_id[this_track_id] = this_op_idx;
            }          
        }


        if (track_ids.size() == 0)
        {
            // current_time = m_io_ops[ m_io_op_idx[0] ].arrival_time;
            add(m_io_op_idx[0]);
            add_queue_idx = 1 - add_queue_idx;
            active_queue_idx = 1 - add_queue_idx;
            return -1;
        }

        // printf("AQ=%d, dir=%d, current_track=%d: ", active_queue_idx, direction, head);
        // for (int queue_idx = 0; queue_idx < 2; queue_idx++)
        // {
        //     printf(" Q[%d] = ( ", queue_idx);
        //     for (int i = 0 ; i < queues[queue_idx].size(); i++)
        //     {
        //         printf("%d:%d ", queues[queue_idx][i], m_io_ops[queues[queue_idx][i]].track_id);
        //     }
        //     printf(")");
        // }
        // printf("\n");
        
        // printf("	Get: (");
        // for (int i = 0 ;i < track_ids.size(); i++)
        // {
        //     if (direction == 1)
        //     {
        //         if (track_ids[i] - head >= 0)
        //         {
        //             printf("%d:%d ", track_2_io_id[track_ids[i]], abs(track_ids[i] - head));
        //         }
        //     }
        //     if (direction == -1)
        //     {
        //         if (track_ids[i] - head <= 0)
        //         {
        //             printf("%d:%d ", track_2_io_id[track_ids[i]], abs(track_ids[i] - head));
        //         }
        //     }
            
        // }
        // printf(") dir=%d\n", direction);
        
        
        // if (track_ids.size() == 0)
        // {
        //     current_time = m_io_ops[ m_io_op_idx[0] ].arrival_time;
        //     return -1;
        // }

        track_ids.push_back(head);

        
        std::sort(track_ids.begin(), track_ids.end());
        // upstair
        std::vector<int>::iterator head_pos = std::find (track_ids.begin(), track_ids.end(), head);
        std::vector<int>::iterator res_iter;
        if (direction == 1 && head_pos != (track_ids.begin() + track_ids.size() - 1))
        {
            res_iter = head_pos + 1;
        }
        else if(direction == 1 && head_pos == (track_ids.begin() + track_ids.size() - 1))
        {
            res_iter = head_pos - 1;
            direction = -1;
            // printf("        --> change direction to -1\n");
        }
        else if(direction == -1 && head_pos != track_ids.begin())
        {
            res_iter = head_pos - 1;
        }
        else if(direction == -1 && head_pos == track_ids.begin())
        {
            res_iter = head_pos + 1;
            direction = 1;
            // printf("        --> change direction to 1\n");
        }

        int track_id = *res_iter;
        int op_idx = track_2_io_id[track_id];        
        queues[active_queue_idx].erase(std::find (queues[active_queue_idx].begin(), queues[active_queue_idx].end(), op_idx));
        m_io_op_idx.erase(std::find (m_io_op_idx.begin(), m_io_op_idx.end(), op_idx));
        return op_idx;	
    }

    bool is_empty()
    {
        return m_io_op_idx.size() == 0 ;
    }
};
#endif