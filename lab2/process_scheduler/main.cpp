#include "DES.h"
#include "RandNum.h"
#include "Scheduler.h"
#include "Process.h"
#include <algorithm>
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstdlib>


bool func(std::pair<int, int> a, std::pair<int, int> b) 
{
    if (a.first == b.first)
    {
        return a.second < b.second;
    }
    else
    {
        return a.first < b.first;
    }        
}

float get_total_IO_utilization(std::vector< std::pair <int, int> > & IO_record)
{    

    std::sort(IO_record.begin(), IO_record.end(), func);    
 
 
    // merge overlapping intervals
    float IOutilization = 0.0;        
    int i = 0;
    int j = 1;
    int end;
    int front;
    for (; i < IO_record.size();)
    {
        front = IO_record[i].first;
        end = IO_record[i].second;
        j = i + 1;
        while( j < IO_record.size() )
        {
            if (IO_record[j].first > end)
            {
                break;
            }
            else
            {
                end = max(IO_record[j].second, end);
            }
            j += 1;
        }
        IOutilization += end - front;
        i = j;
    }

    return IOutilization;

}

void verbose(int timestamp, const Event & event, Process & process, int burst = 0)
{
    std::string transition_string;
    char buffer[200];
    switch (event.transition)
    {
    case Ready2Running:
        transition_string = "READY -> RUNNING";        
        sprintf(buffer, " cb=%d, rem=%d, prio=%d", burst, process.get_waytogo(), process.get_dym_PRIO() + 1);
        transition_string.append(buffer);
        break;    
    case Create2Ready:
        transition_string = "CREATE -> READY";
        break;
    case Running2Block:
        transition_string = "RUNNING -> BLOCK";        
        sprintf(buffer, " ib=%d, rem=%d", burst, process.get_waytogo());
        transition_string.append(buffer);
        break;
    case Running2Ready:
        transition_string = "RUNNING -> READY";
        sprintf(buffer, " cb=%d, rem=%d, prio=%d", burst, process.get_waytogo(), process.get_dym_PRIO() + 1);
        transition_string.append(buffer);
        break;
    case Block2Ready:
        transition_string = "BLOCK -> READY";
        break;
    case Done:
        transition_string = "Done!";
        break;
    default:
        printf("Error!!!!!!!!!!!!!!!!!!!!!!!\n");
        break;
    }

    
    printf("%d %d %d: ", timestamp, process.pid, timestamp - event.time_created);
    printf("%s\n", transition_string.c_str());

}

std::vector<Process> parse_process(const char * filename, const int & max_prios, RandNum & rand)
{
    std::vector<Process> procs;
    ifstream f_ptr(filename);
    int temp;
    std::vector<int> numbers;
    while(f_ptr >> temp)
    {   
        numbers.push_back(temp);
    }
    int i = 0;
    while (i < numbers.size()) 
    {        
        int static_prio = 1 + rand.get() % max_prios;
        Process p(numbers[i], numbers[i+1], numbers[i+2], numbers[i+3], i / 4, static_prio);   
        procs.push_back(p);
        i += 4;
    }

    return procs;
}

bool parse_arguments(int argc, char * argv [], 
                     char & scheduler_type,  bool & verbose,
                     int & quantumn, int & max_prios,
                     std::string & input_file, std::string & rfile)
{
    if (argc < 3) {
        std::cout << "Invalid number of parameters." << std::endl;
        return false;
    }
    max_prios = 4;
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        // check verbose option
        if (arg.compare("-v") == 0) 
        {
            verbose = true;
        }            
        // check scheduler type
        else if (arg.find("-s") == 0) 
        {
            scheduler_type = arg.at(2);       
            if (scheduler_type == 'R')
            {
                quantumn = std::atoi (arg.substr(3, string::npos).c_str());
            }
            else if (scheduler_type == 'P' || scheduler_type == 'E')
            {
                int colon_pos = arg.find(":");
                if (colon_pos > 0)
                {
                    quantumn = std::atoi (arg.substr(3, colon_pos - 3 + 1).c_str());
                    max_prios = std::atoi (arg.substr(colon_pos+1, string::npos).c_str());
                }
                else
                {
                    quantumn = std::atoi (arg.substr(3, string::npos).c_str());
                }
            }
        }         
        else if (input_file.empty()) 
        {
            input_file = arg;            
        } 
        else 
        {
            rfile = arg;
        }
    }

     
}

int main(int argc, char * argv[])
{
    char scheduler_type = 'F';
    bool b_verbose = false;
    int quantumn = 100000;
    int max_prios = 0;    
    std::string input_file;
    std::string rfile;
    int next_start_ts = 0;
    int CURRENT_RUNNING_PROCESS = -1;
    int CURRENT_RUNNING_PROCESS_PRIO = -1;

    parse_arguments( argc, argv, 
                     scheduler_type, b_verbose,
                     quantumn, max_prios,
                     input_file, rfile);

    RandNum rand(rfile.c_str());
    DES des;
    Scheduler * scheduler;
    switch (scheduler_type)
    {
    case 'F':
        scheduler = new FCFS(quantumn, max_prios);
        break;    
    case 'L':
        scheduler = new LCFS(quantumn, max_prios);
        break;      
    case 'S':
        scheduler = new SRTF(quantumn, max_prios);
        break;  
    case 'R':
        scheduler = new RoundRobin(quantumn, max_prios);
        break;   
    case 'P':
        scheduler = new PRIO(quantumn, max_prios);
        break;  
    case 'E':
        scheduler = new PREPRIO(quantumn, max_prios);
        break;    
    default:
        break;
    }
    std::vector<Process> procs = parse_process(input_file.c_str(), max_prios, rand);
    std::vector< std::pair <int, int> > IO_record;






    // for each process, create an event
    for (int i = 0; i < procs.size(); i++) 
    {
        Event new_event(procs[i].get_AT(), procs[i].get_AT(), procs[i].pid, Create2Ready);
        des.insert_event(new_event);
    }

    int tick = procs[0].get_AT();
    while (!des.is_empty())
    {
        Event this_event = des.get_event();
        if (tick < this_event.timestamp)
        {
            tick = this_event.timestamp;
        }
        int pid = this_event.pid;

        if (this_event.transition == Create2Ready)
        {
            // it is now in the ready state, and will enter the running state
            if (b_verbose) verbose(tick, this_event, procs[pid]);
            
            procs[pid].prev_Ready_time = tick;


            if (scheduler_type != 'E')
            {
                Event new_event(procs[pid].get_AT(), procs[pid].get_AT(), pid, Ready2Running);
                des.insert_event(new_event);
                scheduler->insert(pid, procs);
            }
            else
            {                              
                int this_proc_prio = procs[pid].get_dym_PRIO();
                if (CURRENT_RUNNING_PROCESS != -1 )
                {
                    if(CURRENT_RUNNING_PROCESS_PRIO < this_proc_prio &&
                        !des.find_by_pid_ts(CURRENT_RUNNING_PROCESS, tick))
                    {
                        // Preemption in this case happens if the unblocking process’s dynamic priority 
                        // is higher than the currently running processes dynamic priority                     
                        // AND the currently running process does not have an event pending for the same time stamp.
                        
                        //preempt!
                        
                        //you have to remove the future event for the currently running process 
                        des.remove_future_events(CURRENT_RUNNING_PROCESS, this_event.timestamp);

                        // and add a preemption event for the current time stamp
                        Event new_event(this_event.timestamp, this_event.timestamp, CURRENT_RUNNING_PROCESS, Running2Ready);
                        des.push_event_to_front(new_event);
                        
                        // update time remaining for this process
                        int run_till_now = this_event.timestamp - procs[CURRENT_RUNNING_PROCESS].prev_Start_Run_time;
                        procs[CURRENT_RUNNING_PROCESS].restore_waytogo();                            
                        procs[CURRENT_RUNNING_PROCESS].update_waytogo(run_till_now);
                        // update the prev_CPU_burst for this process
                        if (procs[CURRENT_RUNNING_PROCESS].get_prev_CB() >= 0)
                        {
                            procs[CURRENT_RUNNING_PROCESS].restore_prev_CB();
                            // procs[CURRENT_RUNNING_PROCESS].update_prev_CB(procs[CURRENT_RUNNING_PROCESS].get_prev_CB() + 
                            //                                               scheduler->get_quantum() - run_till_now);
                            procs[CURRENT_RUNNING_PROCESS].update_prev_CB( procs[CURRENT_RUNNING_PROCESS].get_prev_CB() - run_till_now);
                        }

                        // update next_start_ts
                        next_start_ts = tick;
                        if (b_verbose) 
                        {
                            printf("---> PRIO preemption %d by %d ? 1 TS=XX now=%d) --> YES\n", 
                                                                CURRENT_RUNNING_PROCESS, pid,  tick);
                        }  
                    }  
                    else
                    {
                        if (b_verbose) 
                            printf("---> PRIO preemption %d by %d ? 1 TS=XX now=%d) --> NO\n", 
                                                                CURRENT_RUNNING_PROCESS, pid,  tick);
                    }                   
                    
                }
                Event new_event(procs[pid].get_AT(), procs[pid].get_AT(), pid, Ready2Running);
                des.insert_event(new_event);
                scheduler->insert(pid, procs);            
            }
            
            
        }
        else if (this_event.transition == Ready2Running)
        {     

           
            // first check if this event is ready to start,
            // because something else might be running now.
            // This is where the preemption can happen
            if (this_event.timestamp < next_start_ts || CURRENT_RUNNING_PROCESS >= 0) 
            {
                this_event.timestamp = next_start_ts;
                des.insert_event(this_event);
                continue;    
            }


            // it is now in the running state
            int pid = scheduler->get(procs);
            
            this_event.time_created = procs[pid].prev_Ready_time;
            
            
            // accumulate the waiting time for CPU in the ready state
            procs[pid].accum_CW( tick - procs[pid].prev_Ready_time );


            // update currently running process and its priority
            CURRENT_RUNNING_PROCESS = pid;
            CURRENT_RUNNING_PROCESS_PRIO = procs[pid].get_dym_PRIO() + 1;

            
            if (scheduler->get_name().compare("FCFS") == 0 || 
                scheduler->get_name().compare("LCFS") == 0 || 
                scheduler->get_name().compare("SRTF") == 0)
            {                
                // the schedule cannot preempt process

                //CPU burst
                int CPU_burst = 1 + rand.get() % procs[pid].get_CB();

                if (CPU_burst < procs[pid].get_waytogo())
                {
                    // this process will enter the block state
                    if (b_verbose) verbose(tick, this_event, procs[pid], CPU_burst);
                
                    Event new_event(tick + CPU_burst, tick, pid, Running2Block);
                    des.insert_event(new_event);

                    next_start_ts = tick + CPU_burst; 
                    // update time remaining for this process
                    procs[pid].update_waytogo(CPU_burst);
                }
                else if (CPU_burst >= procs[pid].get_waytogo())
                {
                    // this process will enter the Done state
                    if (b_verbose) verbose(tick, this_event, procs[pid], procs[pid].get_waytogo());
                    
                    Event new_event(tick + procs[pid].get_waytogo(), tick, pid, Done);
                    des.insert_event(new_event);

                    
                    // update time remaining for this process
                    next_start_ts = tick + procs[pid].get_waytogo(); 
                    procs[pid].update_waytogo(procs[pid].get_waytogo());                    
                }
            }
            else
            {
                // The scheduler can preempt process according to quantum
                int this_quantum = scheduler->get_quantum();
                int CPU_burst = procs[pid].get_prev_CB();
                if ( CPU_burst <= 0)
                {
                    CPU_burst = std::min(1 + rand.get() % procs[pid].get_CB(), procs[pid].get_waytogo());
                }

                if (this_quantum > CPU_burst)
                {
                    // The allocated CPU_burst can finish within this quantum
                    // Then it is the same procedure as in FCFS/LCFS etc.

                    // First reset the prev_CB for this process 
                    procs[pid].update_prev_CB(0);
                    procs[pid].m_prev_prev_CB = CPU_burst;
                    
                    // then compare the allocated CPU_burst with the process's way to go
                    if (CPU_burst < procs[pid].get_waytogo())
                    {
                        // this process will enter the block state
                        if (b_verbose) verbose(tick, this_event, procs[pid], CPU_burst);
                    
                        Event new_event(tick + CPU_burst, tick, pid, Running2Block);
                        des.insert_event(new_event);

                        next_start_ts = tick + CPU_burst; 
                        // update time remaining for this process
                        procs[pid].update_waytogo(CPU_burst);
                    }
                    else if (CPU_burst >= procs[pid].get_waytogo())
                    {
                        // this process will enter the Done state
                        if (b_verbose) verbose(tick, this_event, procs[pid], CPU_burst);
                        
                        Event new_event(tick + procs[pid].get_waytogo(), tick, pid, Done);
                        des.insert_event(new_event);

                        
                        // update time remaining for this process
                        next_start_ts = tick + procs[pid].get_waytogo(); 
                        procs[pid].update_waytogo(procs[pid].get_waytogo());                    
                    }
                }

                else
                {                    
                    // The quantum is not enough for the allocated CPU_burst to run
                    // Then we can only split the allocated CPU_burst and preempt
                    // the process when the quantum is used up, unless the allocated
                    // CPU_burst is more than enough to run this process
                    if (procs[pid].get_waytogo() <= this_quantum)
                    {
                        // Case 1. CPU_burst is more than enough to run this process
                        // this process will enter the Done state
                        if (b_verbose) verbose(tick, this_event, procs[pid], CPU_burst);                    
                        
                        Event new_event(tick + procs[pid].get_waytogo(), tick, pid, Done);
                        des.insert_event(new_event);
                        
                        // update time remaining for this process
                        next_start_ts = tick + procs[pid].get_waytogo(); 
                        procs[pid].update_waytogo(procs[pid].get_waytogo());
                        // reset the prev_CB for this process
                        procs[pid].update_prev_CB(0);
                        procs[pid].m_prev_prev_CB = CPU_burst;
                    }
                    else
                    {
                        // Case 2. The quantum is not enough for the process to finish
                        //          its allocated CPU_burst.
                        // Then consider two cases:
                        // 1. if the quantum exactly the same as CPU burst, then it will enter
                        //    the blocked state.
                        // 2. the quantum is not enough for the CPU burst, then it will
                        //    be preempted and enter the Ready state.
                        int next_state;
                        if (CPU_burst == this_quantum)
                        {
                            next_state = Running2Block;
                        }
                        else
                        {
                            next_state = Running2Ready;
                        }
                        if (b_verbose) verbose(tick, this_event, procs[pid], CPU_burst);
                    
                        Event new_event(tick + this_quantum, tick, pid, next_state);
                        des.insert_event(new_event);

                        next_start_ts = tick + this_quantum; 
                        // update time remaining for this process
                        procs[pid].update_waytogo(this_quantum);
                        // update the preve_CPU_burst for this process
                        procs[pid].update_prev_CB(CPU_burst - this_quantum);
                        procs[pid].m_prev_prev_CB = CPU_burst;
                    }
                }

                // update the time this process start running
                procs[pid].prev_Start_Run_time = tick;
            }


            
            
        }
        else if (this_event.transition == Running2Block)
        {
            // update currently running process and its priority
            CURRENT_RUNNING_PROCESS = -1;
            CURRENT_RUNNING_PROCESS_PRIO = -1;


            //IO burst
            int IO_burst = 1 + rand.get() % procs[pid].get_IO();

            // record this IO interval for the final IO utilization calculation
            std::pair<int, int> record(tick, tick + IO_burst);
            IO_record.push_back(record);


            // this process is now in the block state, the next stage is Ready
            if (b_verbose) verbose(tick, this_event, procs[pid], IO_burst);
        
            Event new_event(tick + IO_burst, tick, pid, Block2Ready);
            des.insert_event(new_event);

            // update the total IO blockk time for this process
            procs[pid].accum_IT(IO_burst);
        }
        else if (this_event.transition == Block2Ready)
        {
            // reset the dynamic priority to static_prio - 1
            procs[pid].update_dym_PRIO(procs[pid].get_PRIO() - 1);
            procs[pid].prev_Ready_time = tick;

            // it is now in the ready state, and will enter the running state            
            if (b_verbose) verbose(tick, this_event, procs[pid]);
            
            if (scheduler_type != 'E')
            {
                Event new_event(tick, tick, pid, Ready2Running);
                des.insert_event(new_event);
                scheduler->insert(pid, procs);
            }
            else
            {                
                int this_proc_prio = procs[pid].get_dym_PRIO();
                if (CURRENT_RUNNING_PROCESS != -1 )
                {
                    if(CURRENT_RUNNING_PROCESS_PRIO < this_proc_prio &&
                        !des.find_by_pid_ts(CURRENT_RUNNING_PROCESS, tick))
                    {
                        // Preemption in this case happens if the unblocking process’s dynamic priority 
                        // is higher than the currently running processes dynamic priority                     
                        // AND the currently running process does not have an event pending for the same time stamp.
                        
                        //preempt!
                        
                        //you have to remove the future event for the currently running process 
                        des.remove_future_events(CURRENT_RUNNING_PROCESS, this_event.timestamp);

                        // and add a preemption event for the current time stamp
                        Event new_event(this_event.timestamp, this_event.timestamp, CURRENT_RUNNING_PROCESS, Running2Ready);
                        des.push_event_to_front(new_event);
                        
                        // update time remaining for this process
                        int run_till_now = this_event.timestamp - procs[CURRENT_RUNNING_PROCESS].prev_Start_Run_time;
                        procs[CURRENT_RUNNING_PROCESS].restore_waytogo();                            
                        procs[CURRENT_RUNNING_PROCESS].update_waytogo(run_till_now);
                        // update the prev_CPU_burst for this process
                        if (procs[CURRENT_RUNNING_PROCESS].get_prev_CB() >= 0)
                        {
                            procs[CURRENT_RUNNING_PROCESS].restore_prev_CB();
                            // procs[CURRENT_RUNNING_PROCESS].update_prev_CB(procs[CURRENT_RUNNING_PROCESS].get_prev_CB() + 
                            //                                               scheduler->get_quantum() - run_till_now);
                            procs[CURRENT_RUNNING_PROCESS].update_prev_CB( procs[CURRENT_RUNNING_PROCESS].get_prev_CB() - run_till_now);
                        }

                        // update next_start_ts
                        next_start_ts = tick;
                        if (b_verbose) 
                        {
                            printf("---> PRIO preemption %d by %d ? 1 TS=XX now=%d) --> YES\n", 
                                                                CURRENT_RUNNING_PROCESS, pid,  tick);
                        }  
                    }  
                    else
                    {
                        if (b_verbose) 
                            printf("---> PRIO preemption %d by %d ? 1 TS=XX now=%d) --> NO\n", 
                                                                CURRENT_RUNNING_PROCESS, pid,  tick);
                    }
                    
                    
                }

                Event new_event(tick, tick, pid, Ready2Running);
                des.insert_event(new_event);
                scheduler->insert(pid, procs);
            }
        }
        else if (this_event.transition == Done)
        {
            // update currently running process and its priority
            CURRENT_RUNNING_PROCESS = -1;
            CURRENT_RUNNING_PROCESS_PRIO = -1;


            // The process has finished!            
            if (b_verbose) verbose(tick, this_event, procs[pid]);
            procs[pid].set_FT(tick);
        }
        else if (this_event.transition == Running2Ready)
        {
            // update currently running process and its priority
            CURRENT_RUNNING_PROCESS = -1;
            CURRENT_RUNNING_PROCESS_PRIO = -1;


            // the process is preempted and goes from Running into Ready state.
            if (b_verbose) verbose(tick, this_event, procs[pid], procs[pid].get_prev_CB());
            procs[pid].prev_Ready_time = tick;
            
            Event new_event(tick, tick, pid, Ready2Running);
            des.insert_event(new_event);
            scheduler->insert(pid, procs);
        }
        
        
    }

    // print final results    
    float IOutilization = get_total_IO_utilization(IO_record);

    printf("%s\n", scheduler->get_name().c_str());
    int max_FT = 0;
    float total_turnaround = 0;
    float total_CW = 0;
    float total_TC = 0;
    for(int pid = 0; pid < procs.size(); pid++)
    {
        int AT = procs[pid].get_AT();
        int TC = procs[pid].get_TC();
        int CB = procs[pid].get_CB();
        int IO = procs[pid].get_IO();
        int FT = procs[pid].get_FT();
        int IT = procs[pid].get_IT();
        int CW = procs[pid].get_CW();
        int TT = FT - AT;       // turaround time
        int PRIO = procs[pid].get_PRIO();

        max_FT = max(FT, max_FT);
        total_CW += CW;
        total_TC += TC;
        total_turnaround += TT;

        
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n", pid, AT, TC, CB, IO, PRIO, FT, TT, IT, CW);
    }
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", max_FT, 
                                                      total_TC / max_FT * 100.0, 
                                                      IOutilization / max_FT * 100.0,
                                                      total_turnaround / procs.size(),
                                                      total_CW / procs.size(),
                                                      float(procs.size()) / max_FT * 100.0);

    return 0;
}