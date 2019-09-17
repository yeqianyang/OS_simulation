#ifndef __RANDNUM_H__
#define __RANDNUM_H__
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

class RandNum
{
public:
    
    RandNum(const char * filename)
    {
        m_f_ptr.open(filename);
        int n_line;
        m_f_ptr >> n_line;
        int num;
        while(m_f_ptr >> num) {
            m_number.push_back(num);
        }
        m_f_ptr.close();
        iter = 0;
    }

    int get()
    {
        int num = m_number[iter];
        iter ++;
        if (iter >= m_number.size())        
        {
            iter = 0;
        }
        return num;   
    }

private:
    std::vector<int> m_number;
    ifstream m_f_ptr;
    int iter;
};

#endif