#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>    // std::all_of
#include <array>        // std::array
#include <map>
#include <vector>
#include <iomanip>
#include <string.h>
using namespace std;




struct Token {
	std::string str;
	int line_number;
	int line_offset;
} ;

void printError(int errcode, const Token & token) 
{
	static char* errstr[] = {
							"NUM_EXPECTED",		// 0. Number expect
							"SYM_EXPECTED",		// 1. Symbol Expected							
							"ADDR_EXPECTED",	// 2. Addressing Expected which is A/E/I/R							
							"SYM_TOO_LONG",		// 3. Symbol Name is too long
							"TOO_MANY_DEF_IN_MODULE",// 4.    >16
							"TOO_MANY_USE_IN_MODULE",// 5.    > 16
							"TOO_MANY_INSTR",	// 6. total num_instr exceeds memory size (512)
							};
	printf("Parse Error line %d offset %d: %s\n", token.line_number, token.line_offset, errstr[errcode]);
	exit(-1);
}



struct DefinEntry{
	std::string sym;
	int rel_addr;
	int abs_addr;
};
struct ProgramEntry{
	std::string type;
	int instruction;
	int offset;
};
struct UseEntry{
	std::string symbol;
	bool used_in_module;	
};
struct Symbol{
	std::string symbol;
	int abs_addr;
	int defined_module;
	bool used_global;
	bool defined_mul_times;
};

typedef std::vector<UseEntry> UseList;
typedef std::vector<DefinEntry> DefinList;
typedef std::vector<ProgramEntry> ProgramList;

struct Module{
	DefinList defin_list;
	UseList use_list;
	ProgramList prog_list;
	int module_offset;
};
std::ostream& operator<< (std::ostream &out, Module const& data) {
	out << "+" << data.module_offset << std::endl;
	for (int i = 0; i < data.prog_list.size(); i++) 
	{
		out << i << ":" << data.prog_list[i].instruction << "\t" << data.prog_list[i].type  << std::endl;
	}
	
	return out;
}

int parse_number(const Token & token)
{
	for (int i = 0; i < token.str.length(); i++)
	{
		if (token.str.c_str()[i] > '9' || token.str.c_str()[i] < '0')
		{
			printError(0, token);
		}
	}

	return stoi(token.str);
}

std::string parse_symbol(const Token & token) 
{    
    const string & str = token.str;
	if (str.length() == 0)
	{
		printError(1, token);
	}
	if (str.length() > 16)
	{
		printError(3, token);
	}
    if ( !isalpha(str[0]) ) 
	{
        printError(1, token);
    }    
    for (int i = 1; i < str.length(); i++) 
	{
        if ( !isalnum(str[i]) ) 
			printError(1, token);
    }
    return str;		
}

std::string parse_type(const Token & token)
{
	std::string str = parse_symbol(token) ;
	if (str == "R" || str == "I" || str == "E" || str == "A")
		return str;
	printError(2, token);
}

std::vector<Token> getToken(ifstream & in_f)
{
    char line[255];
    char * pch;
    std::vector<Token> list_of_tokens;

    int line_number = 1;
    while (in_f){        
        in_f.getline(line,255);
        
		std::string str_line(line);

        pch = strtok(line, " \t\n");
		int line_offset = 0;		
        while (pch != NULL)
        {
            Token temp_token;
            temp_token.str = string(pch);
            temp_token.line_number = line_number;
			
            line_offset = str_line.find(temp_token.str, line_offset);
			
			temp_token.line_offset = line_offset + 1;
            list_of_tokens.push_back(temp_token);
			line_offset += temp_token.str.length();

            pch = strtok (NULL, " \t\n");
        }        

        ++line_number;
    }
	
	// trick
	for (int i = list_of_tokens.size()-1; i >= 0; i--)
	{
		if (list_of_tokens[i].str.length() > 0)
		{
			Token temp_token;
			temp_token.str = string("__EOF");
			temp_token.line_number = list_of_tokens[i].line_number;
			temp_token.line_offset = list_of_tokens[i].line_offset + list_of_tokens[i].str.length();
			list_of_tokens.push_back(temp_token);
			break;
		}
	}


    return list_of_tokens;
}

void parse_def_list(DefinList & def_list, int & iter, const std::vector<Token> & list_of_tokens, const int & module_offset)
{
	
	int def_count = parse_number(list_of_tokens[iter]);
	if (def_count > 16)
	{
		printError(4, list_of_tokens[iter]);
	}
	iter++;

	if (iter >=  list_of_tokens.size())
	{
		printError(1, list_of_tokens[list_of_tokens.size()-1]);
	}

	for (int i = 0; i < def_count; i++) {	

		const Token & sym_token = list_of_tokens[iter];
		string symbol = parse_symbol(sym_token);
		iter++;

		const Token & rel_addr_token = list_of_tokens[iter];
		int rel_addr = parse_number(rel_addr_token);
		iter++;

		DefinEntry this_entry;
		this_entry.rel_addr = rel_addr;
		this_entry.abs_addr = rel_addr + module_offset;
		this_entry.sym = symbol;
		def_list.push_back(this_entry);

		// cout << "Define list: symbol & rel_addr:" << this_entry.sym << " " << this_entry.rel_addr << std::endl;
	}

}

void parse_use_list(UseList & use_list, int & iter, const std::vector<Token> & list_of_tokens)
{
	int use_count = parse_number(list_of_tokens[iter]);	
	if (use_count > 16)
	{
		printError(5, list_of_tokens[iter]);
	}
	iter++;
	for (int i = 0; i < use_count; i++) {	

		string symbol = parse_symbol(list_of_tokens[iter]);
		iter++;

		UseEntry this_entry;
		this_entry.symbol = symbol;
		this_entry.used_in_module = false;
		use_list.push_back(this_entry);
		// cout << "Use list: symbol " << symbol << std::endl;
	}
}

void parse_program_list(ProgramList & prog_list, int & iter, const std::vector<Token> & list_of_tokens, const int & module_offset)
{
	int use_count = stoi(list_of_tokens[iter].str);		
	if (module_offset + use_count > 512)
	{
		printError(6, list_of_tokens[iter]);
	}
	iter++;

	if (iter == list_of_tokens.size() - 1)
	{
		printError(2, list_of_tokens[iter]);
	}
	for (int i = 0; i < use_count; i++) {	

		string type = parse_type(list_of_tokens[iter]);
		iter++;

		int instruction = parse_number(list_of_tokens[iter]);
		// if (list_of_tokens[iter].str.length() < 4)
		// {
		// 	printError(2, list_of_tokens[iter]);
		// }		
		iter++;

		ProgramEntry this_entry;
		this_entry.type = type;
		this_entry.instruction = instruction;
		this_entry.offset = i;

		prog_list.push_back(this_entry);
		// cout << "Program list: type instruc " << this_entry.type << " " << this_entry.instruction << std::endl;
	}
}

int parse_tokens(const std::vector<Token> & list_of_tokens, std::vector<Module> & list_of_modules)
{	
	DefinList def_list;
	UseList use_list;
	ProgramList prog_list;

	int iter = 0;
	int module_offset = 0;
	while (iter < list_of_tokens.size())
	{
		if(list_of_tokens[iter].str == "__EOF")
		{
			break;
		}
		Module new_module;
		parse_def_list(new_module.defin_list , iter, list_of_tokens, module_offset);
		parse_use_list(new_module.use_list, iter, list_of_tokens);
		parse_program_list(new_module.prog_list, iter, list_of_tokens, module_offset);
		new_module.module_offset = module_offset;

		module_offset += new_module.prog_list.size();
		
		list_of_modules.push_back(new_module);

		// cout << new_module;
	}

	return module_offset;
}

void A_types(const ProgramEntry & entry, const int & id)
{
	bool error = false;
	int final_abs_addr;
	if (entry.instruction >= 10000){
		error = true;
		final_abs_addr = 9999;
	}

	int optor = entry.instruction / 1000;
	int oprand = entry.instruction % 1000;
	if (oprand >= 512)
	{
		error = true;
		final_abs_addr = 0;
	}


	if (error && final_abs_addr == 9999)
	{
		// rule 11
		cout << setfill('0') << setw(3) << id << ": " << final_abs_addr;
		printf(" Error: Illegal opcode; treated as 9999\n");
	}
	else if(error && final_abs_addr == 0)
	{
		// rule 8
		cout << setfill('0') << setw(3) << id << ": " << optor * 1000;
		printf(" Error: Absolute address exceeds machine size; zero used\n");
	}
	else 
	{
		cout << setfill('0') << setw(3) << id << ": " << entry.instruction << std::endl;
	}
}
void I_types(const ProgramEntry & entry, const int & id)
{
	int error_code = 0;
	if (entry.instruction >= 10000){
		error_code = 10;		
	}
	if (error_code == 10)
	{
		// rule 10
		cout << setfill('0') << setw(3) << id << ": " << 9999 << " Error: Illegal immediate value; treated as 9999" << std::endl;
	}
	else 
	{
		printf("%03d: %04d\n", id, entry.instruction);
		// cout << setfill('0') << setw(3) << id << ": " << abs_addr << std::endl;
	}	
}
void R_types(const ProgramEntry & entry, const int & module_offset, const int & id, const int & module_size)
{
	int error_code = false;	
	if (entry.instruction >= 10000){
		// rule 11
		error_code = 11;	
	}
	
	int optor = entry.instruction / 1000;
	int oprand = entry.instruction % 1000;
	if (oprand > module_size)
	{
		//rule 9
		error_code = 9;
	}
	


	if (error_code == 11)
	{
		// rule 11
		cout << setfill('0') << setw(3) << id << ": " << 9999;
		printf(" Error: Illegal opcode; treated as 9999\n");
	}
	else if (error_code == 9)
	{
		// rule 9
		cout << setfill('0') << setw(3) << id << ": " << optor * 1000 + module_offset;
		printf(" Error: Relative address exceeds module size; zero used\n");
	}
	else 
	{
		cout << setfill('0') << setw(3) << id << ": " << setfill('0') << setw(4) << entry.instruction + module_offset << std::endl;
	}
}


int E_types(const ProgramEntry & entry, std::map<std::string,Symbol> & sym_records, Module & module, const int & id)
{
	int error_code = 0;	
	std::string sym;
	if (entry.instruction >= 10000)
	{
		error_code = 9;		
	}

	int optor = entry.instruction / 1000;
	int oprand = entry.instruction % 1000;

	if (oprand >= module.use_list.size())
	{
		// rule 6
		error_code = 6;		
	}
	else
	{
		sym = module.use_list[oprand].symbol;

		// rule 3
		if (sym_records.find(sym) == sym_records.end())
		{
			error_code = 3;		

			// update symbol usage,   rule 7.		
			module.use_list[oprand].used_in_module = true;		
		}
		else
		{
			// update symbol usage,   rule 7.		
			module.use_list[oprand].used_in_module = true;
			
			// update symbol usages,  rule 4. 
			sym_records[sym].used_global = true;
		}
	}
	
	
	

	if (error_code == 9)
	{
		// rule 11
		cout << setfill('0') << setw(3) << id << ": " << 9999;
		printf(" Error: Illegal opcode; treated as 9999\n");
	}
	else if (error_code == 3)
	{
		// rule 3		
		cout << setfill('0') << setw(3) << id << ": " << optor * 1000;
		printf(" Error: %s is not defined; zero used\n", sym.c_str());
	}
	else if (error_code == 6)
	{
		// rule 6		
		cout << setfill('0') << setw(3) << id << ": " << entry.instruction;
		printf(" Error: External address exceeds length of uselist; treated as immediate\n");
	}
	else 
	{
		cout << setfill('0') << setw(3) << id << ": " << optor * 1000 + sym_records[sym].abs_addr << std::endl;
	}
}


std::map<std::string,Symbol> map_sym_records(std::vector<Module> & list_of_modules, const int & module_size)
{
	std::map<std::string,Symbol> sym_records;

	for (int i = 0; i < list_of_modules.size(); i++) 
	{
		for (int j = 0; j < list_of_modules[i].defin_list.size(); j++)
		{
			auto sym = list_of_modules[i].defin_list[j].sym;			
			auto abs_addr = list_of_modules[i].defin_list[j].abs_addr;
			auto rel_addr = list_of_modules[i].defin_list[j].rel_addr;
			int module_offset = list_of_modules[i].module_offset;
			if ( rel_addr > module_size - 1)
			{
				// rule 5
				printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n", i+1, sym.c_str(), rel_addr, module_size-1);
				list_of_modules[i].defin_list[j].rel_addr = 0;
				list_of_modules[i].defin_list[j].abs_addr = module_offset + 0;
				abs_addr = module_offset;
			}

			if (sym_records.find(sym) == sym_records.end())
			{
				Symbol new_symbol;
				new_symbol.symbol = sym;
				new_symbol.abs_addr = abs_addr;
				new_symbol.defined_module = i;
				new_symbol.used_global = false;				
				new_symbol.defined_mul_times = false;

				sym_records[sym] = new_symbol;
			}
			else{
				// rule 2
				sym_records[sym].defined_mul_times = true;
			}
		}
	}
	return sym_records;
}
void convert_address(std::vector<Module> & list_of_modules, const int & module_size)
{
	std::vector<std::string> warning_msgs;
	
	// build symbol map to address
	auto sym_records = map_sym_records(list_of_modules, module_size);
	// print symbol table
	cout << "Symbol Table\n";
	for (auto i : sym_records) 
	{
        cout << i.first << "=" << i.second.abs_addr;
		if (i.second.defined_mul_times)
		{
			// rule 2, defined multiple times
			printf(" Error: This variable is multiple times defined; first value used");
		}
        cout << endl;
	}
	cout << endl;
	

	// print memory map an go over each module
	cout << "Memory Map" << endl; 
	int line_count = 0;
	for (int i = 0; i < list_of_modules.size(); i++) {
		Module & this_module = list_of_modules[i];	
		const ProgramList & prog_list = list_of_modules[i].prog_list;

		// cout << "+" << this_module.module_offset << std::endl;
		for (int j = 0; j < prog_list.size(); j++)
		{
			int abs_addr = 0;
			int id = j + this_module.module_offset;

			if (prog_list[j].type == "R")
			{ 
				R_types(prog_list[j], this_module.module_offset, id, module_size);		

			}
			else if (prog_list[j].type == "A")
			{
				A_types(prog_list[j], id);
				
			}
			else if (prog_list[j].type == "I")
			{
				I_types(prog_list[j], id);				
			}
			else if (prog_list[j].type == "E")
			{
				E_types(prog_list[j], sym_records, this_module, id);				
			}
		}

		// If a symbol appears in a use list but is not actually used in the module
		UseList & use_list = this_module.use_list;
		for (int j = 0; j < use_list.size(); j++)
		{
			if (!use_list[j].used_in_module)
			{
				printf("Warning: Module %d: %s appeared in the uselist but was not actually used\n", i + 1, use_list[j].symbol.c_str());
			}
		}		
		
	}

	cout << endl;
	for (auto i : sym_records) 
	{
        if (!i.second.used_global)
		{
			printf("Warning: Module %d: %s was defined but never used\n", i.second.defined_module + 1, i.second.symbol.c_str());
		}
	}

}



int main(int argc, char * argv[])
{
	// The first pass
    ifstream in_f(argv[1]);	
    auto list_of_tokens = getToken(in_f);		
	std::vector<Module> list_of_modules;
    int module_size = parse_tokens(list_of_tokens, list_of_modules);

	// The second pass
	convert_address(list_of_modules, module_size);
}