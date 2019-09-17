#ifndef __RANDNUM_H__
#define __RANDNUM_H__
#include <fstream>
#include <iostream>

using namespace std;

class RandNum
{
public:
    RandNum(const char * filename)
    {
        m_f_ptr.open(filename);
        int n_line;
        m_f_ptr >> n_line;        
    }

    int get()
    {
        int num;
        m_f_ptr >> num;
        return num;   
    }

private:
    ifstream m_f_ptr;
};

#endif