#include "scheduler.h"
#include <algorithm>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <iostream>


bool parse_arguments(int argc, char * argv [], 
                     char & algorithm,
                     bool & print_v, bool & print_q, bool & print_f,
                     std::string & input_file)
{
    if (argc < 3) {
        std::cout << "Invalid number of parameters." << std::endl;
        return false;
    }    
    for (int i = 1; i < argc; i++) 
    {
        std::string arg(argv[i]);
        // check verbose option
        if (arg.find("-s") == 0) 
        {
            algorithm = arg.at(2);                   
        }         
        else if (arg.find("-v") == 0) 
        {
            print_v = true;                 
        }   
        else if (arg.find("-f") == 0) 
        {                
            print_f = true;                    
        }         
        else if (arg.find("-q") == 0) 
        {                
            print_q = true;                    
        }    
        else 
        {
            input_file = arg;
        }
    }
}

void parse_input_file(std::string & filename, std::vector<IO_Operation> & io_ops)
{
    std::ifstream f_ptr;
    f_ptr.open(filename.c_str());
    std::string line;

    while (std::getline(f_ptr,line))
    {
        if (line.at(0) != '#')
        {
            int arrival_time;
            int track_id;
            sscanf(line.c_str(), "%d %d", & arrival_time, & track_id);
            IO_Operation io_op(io_ops.size(), arrival_time, track_id);
            io_ops.push_back(io_op);
        }
    }
}



int main(int argc, char * argv[])
{
    char algorithm;
    bool print_v = false;
    bool print_f = false;
    bool print_q = false;    
    std::string input_file;

    parse_arguments( argc, argv, 
                     algorithm, 
                     print_v, print_q, print_f,
                     input_file);
    

    std::vector<IO_Operation>  io_ops;
    parse_input_file(input_file, io_ops);

    Scheduler * scheduler;
    switch (algorithm)
    {
    case 'i':
        scheduler = new FIFO(io_ops);
        break;
    case 'j':
        scheduler = new SSTF(io_ops);
        break;
    case 's':
        scheduler = new LOOK(io_ops);
        break;
    case 'c':
        scheduler = new CLOOK(io_ops);
        break;
    case 'f':
        scheduler = new FLOOK(io_ops);
        break;    
    default:
        break;
    }
    
    int tick = io_ops[0].arrival_time;    
    io_ops[0].b_added = true;
    
    if (print_v) 
    {
        printf("%d:%6d add %d\n", tick, io_ops[0].op_id, io_ops[0].track_id);
    }
    scheduler->add(0);
    
    
    int current_track = 0;
    int tot_movement = 0;
    int tot_wait_time = 0;
    int max_wait_time = 0;
    int tot_turnaround_time = 0;
    while (! scheduler->is_empty()) 
    {
        int op_id = scheduler->get(current_track, tick);
        if (op_id < 0)
        {
            continue;
        }
        IO_Operation & current_op = io_ops[op_id];


        

        if (tick < current_op.arrival_time)
        {
            tick = current_op.arrival_time;
        }
        current_op.start_time = tick;       

        if (print_v) 
        {
            printf("%d:%6d issue %d %d\n", tick, current_op.op_id, current_op.track_id, current_track);
        }
        
        int time_spent = abs(current_track - current_op.track_id);
        tot_movement += time_spent;        
        
        int time_waiting = tick - current_op.arrival_time;
        tot_wait_time += time_waiting;

        max_wait_time = std::max(max_wait_time, time_waiting);
        
        tick += time_spent;

        tot_turnaround_time += tick - current_op.arrival_time;
        current_track = current_op.track_id;
        

        for (int j = 0; j < io_ops.size(); j++)
        {
            if (!io_ops[j].b_added  && io_ops[j].arrival_time <= tick && io_ops[j].arrival_time >= tick - time_spent)
            {
                io_ops[j].b_added = true;
                
                if (print_v) 
                {
                    printf("%d:%6d add %d\n", io_ops[j].arrival_time, io_ops[j].op_id, io_ops[j].track_id);
                }
                scheduler->add(io_ops[j].op_id);
            }
            
        }



        current_op.end_time = tick;
        
        if(print_v) 
        {
            printf("%d:%6d finish %d\n", tick, current_op.op_id, tick - current_op.arrival_time);
        }        
    }


    for (int op_id = 0; op_id < io_ops.size(); op_id++)
    {
        IO_Operation & current_op = io_ops[op_id];
        printf("%5d: %5d %5d %5d\n",op_id, current_op.arrival_time, current_op.start_time, current_op.end_time);
    }

    printf("SUM: %d %d %.2lf %.2lf %d\n",
			tick, tot_movement, double(tot_turnaround_time) / double(io_ops.size()), double(tot_wait_time) / double(io_ops.size()), max_wait_time);

    return 0;
}