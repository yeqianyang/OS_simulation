#ifndef __PAGER_H__
#define __PAGER_H__
#include <vector>
#include <algorithm>
#include "RandNum.h"
#include "PTE.h"
#include <assert.h> 

class FrameTable
{
public:
    int size;
    std::vector<int> frames;
    std::vector<int> rev_frames;
    std::vector<int> rev_pid;
    std::vector<int> time_last_used;
    std::vector<int> avail_frame_idx;

    FrameTable(int _size)
    {
        size = _size;        
        frames = std::vector<int>(size,-1);
        rev_frames = std::vector<int>(size,-1);
        rev_pid = std::vector<int>(size,-1);
        time_last_used = std::vector<int>(size,-1);
        for (int i = 0 ; i < size ; i++)
        {
            avail_frame_idx.push_back(i);
        }
    }
    void return_frame(int frame_idx)
    {
        frames[frame_idx] = -1;
        rev_frames[frame_idx] = -1;
        rev_pid[frame_idx] = -1;
        // time_last_used[frame_idx] = -1;
        
        avail_frame_idx.push_back(frame_idx);
    }
    int get_free_frame()
    {
        int idx = avail_frame_idx.front();
        avail_frame_idx.erase(avail_frame_idx.begin());
        return idx;
    }
    bool is_full()
    {        
        for (int i = 0 ; i < size; i++)
        {
            if (frames[i] < 0)
            {
                return false;
            }
        }
        return true;
    }
    void print()
    {
        printf("FT: ");
        for (int i = 0; i < rev_frames.size(); ++i) 
        {
            if (rev_frames[i] == -1) 
            {
                printf("* ");
            } 
            else 
            {
                printf("%d:%d ", rev_pid[i], rev_frames[i]);
            }
        }

        if (rev_frames.size() < size)
        {
            for (int i = rev_frames.size(); i < size; i++ )
            {
                printf("* ");
            }
        }
                
        printf("\n");
    }
};


class Pager {
public:
    virtual int select_victim_frame(FrameTable & FT, std::vector<Process> & procs, int inst_id) = 0; // virtual base class
    void reset_age(int frame_idx)
    {
        if (ages.size() > 0)
        {
            ages[frame_idx] = 0;
        }
    }
public:
    std::vector<unsigned int> ages;
};


class FIFO: public Pager
{
public:
    FIFO():Pager()
    {

    }
    int select_victim_frame(FrameTable & FT, std::vector<Process> & procs, int inst_id){
        int min_frame_idx = 0;
        for (int i = 1; i < FT.size; i++) 
        {
            if (FT.frames[i] >= 0  && FT.time_last_used[i] < FT.time_last_used[min_frame_idx]) 
            {
                min_frame_idx = i;
            } 
        }

        return min_frame_idx;
    }
};


class Random : public Pager {

public:
    RandNum & rand_gen;
    Random(RandNum & _rand_gen):Pager(), rand_gen(_rand_gen)
    {        
    }
    int select_victim_frame(FrameTable & FT, std::vector<Process> & procs, int inst_id)
    {
        int rand = rand_gen.get();
        return FT.frames[rand % FT.frames.size()];
    }
};




class Clock : public Pager {

public:
    int count;
    Clock():Pager()
    {
        count = 0;        
    }
    int select_victim_frame(FrameTable & FT, std::vector<Process> & procs, int inst_id)
    {
        bool found = false;
        int f_id;
        int vpage_id;
        int pid;
        while (!found) 
        {
            f_id = FT.frames[count];
            vpage_id = FT.rev_frames[f_id];
            pid = FT.rev_pid[f_id];
            if ( procs[pid].page_table[vpage_id].referenced ) 
            {
                procs[pid].page_table[vpage_id].referenced = 0;
            } 
            else 
            {
                found = true;
            }
            
            count = (count + 1) % FT.frames.size();
        }
        return f_id;
    }
};


class NRU : public Pager {

public:
    std::vector<int> priority_hand;
    int hand;
    int last_reset_iter;
    NRU():Pager()
    {
        priority_hand = std::vector<int>(4, 0);
        hand = 0;
        last_reset_iter = -1;
    }
    int select_victim_frame(FrameTable & FT, std::vector<Process> & procs, int inst_id)
    {
        int init_hand = hand;
        
        std::vector<std::vector<int> > priority(4, std::vector<int>());
        
        if (hand >= FT.frames.size())
        {
            hand = 0;
        }
        for (int k = hand; k < hand + FT.frames.size(); k++)
        {
            int i = k % FT.frames.size();
            int frame_id = FT.frames[i];
            int vpage_id = FT.rev_frames[i];
            int pid = FT.rev_pid[i];            
            if (procs[pid].page_table[vpage_id].present)
            {
                int referenced = procs[pid].page_table[vpage_id].referenced;
                int modified   = procs[pid].page_table[vpage_id].modified;

                if (referenced && modified) {
                    priority[3].push_back(frame_id);
                } 
                else if (referenced && ! modified) 
                {
                    priority[2].push_back(frame_id);
                } 
                else if (!referenced && modified ) 
                {
                    priority[1].push_back(frame_id);
                } 
                else 
                {
                    priority[0].push_back(frame_id);
                }
            }               
        }

        // the first frame that falls into each class
        int result = -1;
        int lowest_class = 0;
        for (int i = 0; i < 4; ++i) 
        {
            if (priority[i].size() > 0) 
            {
                // result = priority[i][rand_gen.get() % priority[i].size()];
                // result = priority[i][0];                
                lowest_class = i;
                result = priority[i][0];  
                break;
            }
        }

        int reset = 0;
        if (inst_id - last_reset_iter >= 50) 
        {
            last_reset_iter = inst_id;
            reset = 1;
            for (int pid = 0; pid < procs.size(); pid++)
            {
                for (int vpage_id = 0; vpage_id < procs[pid].page_table.size(); vpage_id++) 
                {
                    if (procs[pid].page_table[vpage_id].present)
                    {
                        procs[pid].page_table[vpage_id].referenced = 0;
                    }
                }
            }
        }

        // printf("ASELECT hand=%d %d| %d %d x\n", init_hand, reset, lowest_class, result);

        for (int i = 0; i < FT.frames.size(); i++)
        {
            if (FT.frames[i] == result)
            {
                hand = i + 1;
                if (hand >= FT.frames.size())
                {
                    hand = 0;
                }
                break;
            }

            
        }

        // std::vector<int>::iterator loc = std::find (FT.frames.begin(), FT.frames.end(), result);
        
        // FT.frames.erase(loc);
        // FT.frames.push_back(result);

        return result;
    }
};






class Aging : public Pager {

public:
    int count;
    unsigned int hand;
    
    Aging():Pager()
    {
        count = 0;
        hand = 0;        
    }
    int select_victim_frame(FrameTable & FT, std::vector<Process> & procs, int inst_id)
    {
        unsigned int min_age = 0xffffffff;
        unsigned int min_frame = -1;
        unsigned int min_index = -1;
        
        if (ages.size() == 0) 
        {
            ages = std::vector<unsigned int>(procs[0].page_table.size(), 0);
        }

        for (int k = hand; k < FT.frames.size() + hand; k++) 
        {
            int i = k % FT.frames.size();
            unsigned int ref_bit = procs[FT.rev_pid[i]].page_table[FT.rev_frames[i]].referenced;

            ages[i] = (ages[i] >> 1) | (ref_bit << 31);

            if (ages[i] < min_age) 
            {
                min_age = ages[i];
                min_frame = FT.frames[i];
                min_index = i;
                
            }

            procs[FT.rev_pid[i]].page_table[FT.rev_frames[i]].referenced = 0;

        }
        ages[min_index] = 0;

        hand = min_index + 1;
        if (hand > FT.frames.size())
        {
            hand = hand % FT.frames.size();
        }
        return min_frame;
    }
};






class WorkingSet : public Pager {

public:
    std::vector<int> priority_hand;
    int hand;
    int last_reset_iter;
    WorkingSet():Pager()
    {
        priority_hand = std::vector<int>(4, 0);
        hand = 0;
        last_reset_iter = -1;
    }
    int select_victim_frame(FrameTable & FT, std::vector<Process> & procs, int inst_id)
    {
        int init_hand = hand;
        
        if (hand >= FT.frames.size())
        {
            hand = 0;
        }
        // printf("ASELECT %d-%d | ", init_hand, (init_hand + FT.frames.size() - 1) % FT.frames.size());

        int bin_id = 0;        
        int longest_time = -10000000;
        int n_frame_iter = 0;
        for (int k = hand; k < hand + FT.frames.size(); k++, n_frame_iter++)
        {
            int i = k % FT.frames.size();
            int frame_id = FT.frames[i];
            assert(frame_id >= 0);
            int vpage_id = FT.rev_frames[i];
            int pid = FT.rev_pid[i];            
            int time_last_used = FT.time_last_used[i];
            if (procs[pid].page_table[vpage_id].present)
            {
                int referenced = procs[pid].page_table[vpage_id].referenced;
                // printf("%d(%d %d:%d %d) ", frame_id, referenced, pid, vpage_id, FT.time_last_used[i]);

                if (referenced)
                {
                    procs[pid].page_table[vpage_id].referenced = 0;
                    FT.time_last_used[i] = inst_id;
                }
                int time_lapse = inst_id - time_last_used;
                if (!referenced && time_lapse>= 50)
                {
                    bin_id = i;
                    // printf("STOP(%d) ", n_frame_iter + 1);
                    break;
                }
                else if(!referenced && time_lapse < 50)
                {
                    if (time_lapse > longest_time)
                    {                        
                        longest_time = time_lapse;
                        bin_id = i;
                    }
                }
            }               
        }

    
        // printf("| %d\n", FT.frames[bin_id]);

        
        
        hand = bin_id + 1;
        if (hand >= FT.frames.size())
        {
            hand = 0;
        }
       
        return FT.frames[bin_id];
    }
};







#endif