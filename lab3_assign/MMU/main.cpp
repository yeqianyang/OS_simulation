#include "RandNum.h"
#include "PTE.h"
#include "Pager.h"
#include <algorithm>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <iostream>


bool parse_arguments(int argc, char * argv [], 
                     char & pager_type, int & num_frames,
                     bool & print_O,
                     bool & print_P, bool & print_F,
                     bool & print_S,  bool & print_x,
                     bool & print_y, bool & print_f, bool & print_a,
                     std::string & input_file, 
                     std::string & rfile)
{
    if (argc < 3) {
        std::cout << "Invalid number of parameters." << std::endl;
        return false;
    }    
    for (int i = 1; i < argc; i++) 
    {
        std::string arg(argv[i]);
        // check verbose option
        if (arg.find("-o") == 0) 
        {
            for (int i = 2 ; i < arg.length(); i++)
            {
                switch (arg.at(i))
                {
                case 'O': 
                    print_O = true;                   
                    break;
                case 'P':    
                    print_P = true;
                    break;
                case 'F':   
                    print_F = true;
                    break;
                case 'S':       
                    print_S = true;             
                    break;
                case 'x':         
                    print_x = true;           
                    break;
                case 'y':        
                    print_y = true;            
                    break;
                case 'f':    
                    print_f = true;                
                    break;
                case 'a':                  
                    print_a = true;          
                    break;
                default:
                    break;
                }
            }            
        }            
        else if (arg.find("-f") == 0) 
        {                
            num_frames = std::atoi (arg.substr(2, string::npos).c_str());           
        }         
        else if (arg.find("-a") == 0) 
        {
            pager_type = arg.at(2);                   
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

class Instruct
{
public:
    int id;
    char cmd;
    int vpage_id;
    Instruct(int _id, char _cmd, int _vpage_id)
    {
        id = _id;
        cmd = _cmd;
        vpage_id = _vpage_id;
    }
};



void parse_input_file(std::string & filename, std::vector<Instruct> & instructions, std::vector<Process> & procs)
{
    std::ifstream f_ptr;
    f_ptr.open(filename.c_str());
    char cmd;
    int addr;
    int id;
    std::string line;

    std::vector<int> numbers;

    std::vector<std::string> all_lines;
    while (std::getline(f_ptr,line))
    {
        if (line.at(0) != '#')
        {
            all_lines.push_back(line);
        }
    }

    int n_process = std::atoi (all_lines[0].c_str());           
    int line_ptr = 1;
    for (int i = 0; i < n_process; i++)
    {
        int n_VMAS;
        n_VMAS = std::atoi (all_lines[line_ptr].c_str());  
        line_ptr ++;

        Process new_proc(i);
        for (int j = 0; j < n_VMAS; j++)
        {
            int svp;
            int evp;
            int write_protected;
            int filemapped;
            sscanf(all_lines[line_ptr].c_str(), "%d %d %d %d", & svp, & evp, & write_protected, & filemapped);
            VMA new_vma(svp, evp, write_protected, filemapped);
            new_proc.vma.push_back(new_vma);
            line_ptr ++;
        }
        new_proc.sync_vma();
        procs.push_back(new_proc);
    }
    
    // begin the instructions
    for(int cid = 0; line_ptr < all_lines.size(); line_ptr++, cid++)
    {
        char cmd;
        int addr;
        sscanf(all_lines[line_ptr].c_str(), "%c %d", & cmd, & addr);
        Instruct new_instr(cid, cmd, addr);
        instructions.push_back(new_instr);
    }

}



int main(int argc, char * argv[])
{
    char pager_type;
    int num_frames;
    bool print_O = false;
    bool print_P = false;
    bool print_F = false;
    bool print_S = false;
    bool print_x = false;
    bool print_y = false;
    bool print_f = false;
    bool print_a = false;
    std::string input_file;
    std::string rfile;

    parse_arguments( argc, argv, 
                     pager_type, num_frames,
                     print_O, 
                     print_P, print_F,
                     print_S,  print_x,
                     print_y, print_f, print_a,
                     input_file, rfile);
    
    int n_exit = 0;
    int n_rw = 0;
    int n_ctx_switches = 0;       //??

    std::vector<Instruct>  instructions;
    std::vector<Process>   procs;
    parse_input_file(input_file, instructions, procs);
    Process * current_proc = & procs[0];
    RandNum rand(rfile.c_str());
   
    FrameTable FT(num_frames);

    Pager * pager;
    switch (pager_type)
    {
    case 'f':
        pager = new FIFO();
        break;
    case 'r':
        pager = new Random(rand);
        break;
    case 'c':
        pager = new Clock();
        break;
    case 'e':
        pager = new NRU();
        break;
    case 'a':
        pager = new Aging();
        break;
    case 'w':
        pager = new WorkingSet();
        break;
    default:
        break;
    }

    for (int i = 0; i < instructions.size(); i++)
    {
        int cid = instructions[i].id;
        char cmd = instructions[i].cmd;
        int vpage_id = instructions[i].vpage_id;

        if (print_O)
        {
            printf("%d: ==> %c %d\n", cid, cmd, vpage_id);
        }

        if (cmd == 'c')
        {
            current_proc = &procs[vpage_id];
            n_ctx_switches ++;
            continue;
        }
        else if(cmd == 'e')
        {
            int pid = vpage_id;

            printf("EXIT current process %d\n", pid);

            for (int i = 0 ; i < procs[pid].page_table.size(); i++)
            {
                
                if (procs[pid].page_table[i].present) 
                {
                    if (print_O)
                    {
                        printf(" UNMAP %d:%d\n", pid, i);
                    }                    
                    procs[pid].n_unmap++;

                    if (procs[pid].page_table[i].modified) 
                    {
                        // clean respective bits
                        procs[pid].page_table[i].modified = 0;

                        if (print_O) 
                        {
                            if (procs[pid].file_mapped[i])
                            {
                                printf(" FOUT\n");
                            }
                        }                    
                        if (print_O) 
                        {
                            if (procs[pid].file_mapped[i])
                            {
                                procs[pid].page_table[i].pagedout = 1;
                                procs[pid].n_fout ++;
                            }                                                   
                        }       
                        
                    }


                    FT.return_frame(procs[pid].page_table[i].frame_id);
                    pager->reset_age(procs[pid].page_table[i].frame_id);
                }
                procs[pid].page_table[i].present = 0;
                procs[pid].page_table[i].pagedout = 0;
                procs[pid].page_table[i].referenced = 0;
            }

            n_exit ++;
            continue;
        }

        n_rw ++;
        if ( ! current_proc->accessible[vpage_id] )
        {
            printf(" SEGV\n");
            current_proc->n_segv++;
            continue;
        }
        
        
        // if the page is not in the memory
        if( ! current_proc->page_table[vpage_id].present)
        {
            // if the frame table is not full, then create an entry and map
            if( ! FT.is_full() )
            {
                int frame_id = FT.get_free_frame();
                FT.frames[frame_id] = frame_id;
                FT.rev_frames[frame_id] = vpage_id;
                FT.rev_pid[frame_id] = current_proc->pid;
                if (FT.time_last_used[frame_id] < 0)
                {
                    FT.time_last_used[frame_id] = cid;
                }
                


                current_proc->page_table[vpage_id].frame_id = frame_id;

                if (current_proc->file_mapped[vpage_id])
                {
                    current_proc->n_fin++;
                    if (print_O) printf(" FIN\n");
                }
                else if (current_proc->page_table[vpage_id].pagedout)
                {                    
                    current_proc->n_pageins++;
                    if (print_O) printf(" IN\n");
                }
                else
                {
                    current_proc->n_zero ++;
                    if (print_O) printf(" ZERO\n");
                }
                

                if (print_O) printf(" MAP %d\n", frame_id);
                current_proc->n_map ++;

                // if (current_proc->write_protected[vpage_id] && cmd == 'w')
                // {
                //     current_proc->n_segprot ++;
                //     if (print_O)
                //     {
                //         printf(" SEGPROT\n");
                //     }
                // }
            }
            else
            {
                // call the pager
                int rep_frameid = pager->select_victim_frame(FT, procs, cid);
                int rep_pageid = FT.rev_frames[rep_frameid];
                int rep_pid = FT.rev_pid[rep_frameid];

                if (print_O) 
                {
                    printf(" UNMAP %d:%d\n", rep_pid, rep_pageid);
                }
                procs[rep_pid].n_unmap ++;

                // reset bit
                // current_proc->page_table[rep_pageid].present = 0; 
                // current_proc->page_table[rep_pageid].referenced = 0; 
                procs[rep_pid].page_table[rep_pageid].present = 0; 
                procs[rep_pid].page_table[rep_pageid].referenced = 0; 

                // previous page is dirty 
                if (procs[rep_pid].page_table[rep_pageid].modified) 
                {
                    // clean respective bits
                    procs[rep_pid].page_table[rep_pageid].modified = 0;
                    procs[rep_pid].page_table[rep_pageid].pagedout = 1;

                    if (print_O) 
                    {
                        if (procs[rep_pid].file_mapped[rep_pageid])
                        {
                            printf(" FOUT\n");
                        }
                        else
                        {
                            printf(" OUT\n");
                        }                        
                    }                    
                    if (print_O) 
                    {
                        if (procs[rep_pid].file_mapped[rep_pageid])
                        {
                            procs[rep_pid].n_fout ++;
                        }
                        else
                        {
                           procs[rep_pid].n_pageouts ++;
                        }                        
                    }       
                    
                }

                // current page has been paged out
                if (current_proc->page_table[vpage_id].pagedout == 1) 
                {
                    if (current_proc->file_mapped[vpage_id])
                    {
                        printf(" FIN\n");
                    }
                    else
                    {
                        printf(" IN\n");
                    }    
                    if (current_proc->file_mapped[vpage_id])
                    {
                        current_proc->n_fin ++;
                    }
                    else
                    {
                        current_proc->n_pageins ++;
                    }      

                } 
                else 
                {
                    if (current_proc->file_mapped[vpage_id])
                    {
                        current_proc->n_fin ++;
                        if (print_O) printf(" FIN\n");
                    }
                    else
                    {
                        current_proc->n_zero ++;
                        if (print_O) printf(" ZERO\n");
                    }                    
                }
                
                current_proc->n_map ++;
                if (print_O) printf(" MAP %d\n", rep_frameid);               

                // map the page
                current_proc->page_table[vpage_id].frame_id = rep_frameid;
                FT.rev_frames[rep_frameid] = vpage_id;
                FT.rev_pid[rep_frameid] = current_proc->pid;
                FT.time_last_used[rep_frameid] = cid;
            }

            // set bits
            current_proc->page_table[vpage_id].present = 1;            
            if (cmd == 'r') 
            {
                current_proc->page_table[vpage_id].referenced = 1;            
            } 
            else if (cmd == 'w' )
            {
                current_proc->page_table[vpage_id].referenced = 1;    
                if (! current_proc->write_protected[vpage_id])
                {
                    current_proc->page_table[vpage_id].modified = 1;
                }                
                else
                {
                    current_proc->n_segprot ++;
                    if (print_O) printf(" SEGPROT\n");
                }
                
            }


        }
        // the page is already in the frame table
        else
        {
            // if the corresponding entry is already in the frame table
                        // algo->update(frames, getFrameNumber(page));      ????????
            if (cmd == 'r') 
            {
                current_proc->page_table[vpage_id].referenced = 1;
            } 
            else if (cmd == 'w')
            {
                current_proc->page_table[vpage_id].referenced = 1;
                if (! current_proc->write_protected[vpage_id])
                {
                    current_proc->page_table[vpage_id].modified = 1;
                }
                else
                {
                    printf(" SEGPROT\n");
                    current_proc->n_segprot++;
                }
                
            }
        }
        

       

    }

    if (print_P)
    {
        for (int i = 0; i < procs.size(); i++)
        {
            procs[i].print_page_table();
        }
    }
    if (print_F)
    {
        FT.print();
    }
    
    if (print_S)
    {
        unsigned long long cost = 0;               //??
        for (int i = 0; i < procs.size(); i++)
        {
            printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                procs[i].pid,
                procs[i].n_unmap, procs[i].n_map, procs[i].n_pageins, procs[i].n_pageouts,
                procs[i].n_fin, procs[i].n_fout, procs[i].n_zero,
                procs[i].n_segv, procs[i].n_segprot);
            cost += 400 * (procs[i].n_map + procs[i].n_unmap);
            cost += 3000 * (procs[i].n_pageins + procs[i].n_pageouts);
            cost += 2500 * (procs[i].n_fin + procs[i].n_fout);
            cost += 150 * (procs[i].n_zero);
            cost += 240 * (procs[i].n_segv);
            cost += 300 * (procs[i].n_segprot);
        }
        
        

        cost += n_ctx_switches * 121;
        cost += n_exit * 175;
        cost += n_rw;

        printf("TOTALCOST %lu %lu %lu %llu\n", instructions.size(), n_ctx_switches, n_exit, cost);
    }

    return 0;
}