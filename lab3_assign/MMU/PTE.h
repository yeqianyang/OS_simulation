#ifndef __PTE_H__
#define __PTE_H__

#include <vector>
#include "stdio.h"

class VMA
{
public:
    int svp;
    int evp;
    int write_protected;
    int filemapped;
    VMA(int _svp, char _evp, int _write_protected, int _filemapped)
    {
        svp = _svp;
        evp = _evp;
        write_protected = _write_protected;
        filemapped = _filemapped;
    }
};


class PageTableEntry
{
public:
    unsigned int present;
    unsigned int modified;
    unsigned int referenced;
    unsigned int pagedout;
    int frame_id;
    PageTableEntry()
    {
        present = 0;
        modified = 0;
        referenced = 0;
        pagedout = 0;
        frame_id = -1;
    }
};



class Process
{
public:
    unsigned long int n_segv;
    unsigned long int n_segprot;
    unsigned long int n_unmap;
    unsigned long int n_map;
    unsigned long int n_pageins;
    unsigned long int n_pageouts; 
    unsigned long int n_zero;
    unsigned long int n_fin;
    unsigned long int n_fout;

    
    int pid;    // process id
    std::vector<VMA> vma;
    std::vector<PageTableEntry> page_table;
    std::vector<bool> write_protected;
    std::vector<bool> file_mapped;
    std::vector<bool> accessible;
    Process(int _pid)
    {
        pid = _pid;
        page_table = std::vector<PageTableEntry>(64,PageTableEntry());

        n_segv = 0;
        n_segprot = 0;
        n_unmap = 0;
        n_map = 0;
        n_pageins = 0;
        n_pageouts = 0;
        n_zero = 0;
        n_fin = 0;
        n_fout = 0;
    }

    void sync_vma()
    {
        write_protected = std::vector<bool>(64,false);
        file_mapped = std::vector<bool>(64,false);
        accessible = std::vector<bool>(64,false);
        for (int i = 0 ; i < vma.size(); i++)
        {
            for (int j = vma[i].svp; j <= vma[i].evp; j++)
            {
                write_protected[j] = vma[i].write_protected > 0;
                file_mapped[j] = vma[i].filemapped > 0;
                accessible[j] = true;
            }
        }
    }
    void print_page_table()
    {
        printf("PT[%d]: ",  pid);
        for (int i = 0; i < page_table.size(); ++i) {
            if (page_table[i].present) 
            {
                printf("%d:", i);
                if (page_table[i].referenced) 
                {
                    printf("R");
                } 
                else 
                {
                    printf("-");
                }

                if (page_table[i].modified) 
                {
                    printf("M");
                } 
                else 
                {
                    printf("-");
                }

                if (page_table[i].pagedout) 
                {
                    if (file_mapped[i])
                    {
                        printf("- ");
                    }
                    else
                    {
                        printf("S ");
                    }
                    
                } 
                else 
                {
                    printf("- ");
                }

            } 
            else 
            {
                if (page_table[i].pagedout && !file_mapped[i]) 
                {
                    printf("# ");
                } 
                else 
                {
                    printf("* ");
                }
            }
        }
        printf("\n");
    }
};

#endif