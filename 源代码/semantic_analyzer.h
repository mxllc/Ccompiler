#pragma once
#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include <vector>
#include "utils.h"
#include "symbol_table.h"
#include <fstream>
#include <iostream>
#include "target_code_generator.h"


using namespace std;

class SemanticAnalyzer
{
private:
	vector<SymbolTable> symbol_tables_;//符号表容器，每一个符号表对应一个过程
	vector<int> current_symbol_table_stack_;//当前所处定义域的所有相关层次的符号表的序号栈
	bool CreateSymbolTable(TABLE_TYPE, const string &);//创建一个新的符号表，并修改当前符号表

	int next_state_num_;//下一个四元式的序号。这里从1开始，0留给跳转到程序起始地址的四元式
	ofstream intermediate_code_temp_printer_;//中间代码临时文件打印机
	fstream intermediate_code_temp_reader_;//中间代码临时文件读取
	ofstream intermediate_code_printer_;//中间代码打印机

	int backpatching_level_;//回填的层次，从0开始。 这是由于回填会嵌套
	vector<Quadruples> quadruples_stack_;//存放回填四元式，回填完毕后统一输出。
	vector<int> backpatching_value;//回填的值
	vector<int> backpatching_point_pos_;//回填点在回填栈中的地址
	int main_line_;//main函数的序号
	bool PrintQuadruples(const Quadruples &);//根据四元式进行打印中间代码临时文件
	bool PrintQuadruples();//根据临时文件打印中间代码

	vector<string> w_label_;//用来记录while，if语句Label。 这里是记录写出的Label
	vector<string> j_label_;//记录要跳转的Label
	vector<int> w_j_label_nums;//记录每层的while,if数目
	int w_j_label_num_;//初始为0，每进入一个函数就设0，出去设

	TargetCodeGenerator target_code_generator_;//目标代码生成器

	//生成目标代码中跳转用的标签号，防止重复
	int while_cnt_;
	int if_cnt_;

	
public:
	SemanticAnalyzer();
	~SemanticAnalyzer();

	bool ExecuteSemanticCheck(vector<GrammarSymbolInfo>&, const Production &);//执行语义分析检查,并根据产生式修改文法符号属性栈
	int GetNextStateNum();//返回下一个四元式的序号，并且序号加1
	int PeekNextStateNum();//查看下一个四元式的序号
	
	string GetArgName(SymbolPos &, bool is_return=false);//在中间代码生成的时候，获取变量的名称。 对于不是临时变量的变量，需要加上表的序号，例如： a_2
	
};


#endif // !SEMANTIC_ANALYZER_H

