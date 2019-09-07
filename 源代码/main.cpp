#include <iostream>
#include <string>
#include "lexical_analyzer.h"
#include "syntactic_analyzer.h"

#include "utils.h"

using namespace std;

int main()
{

	cout << "请手动创建目录 gen_data/lexcial_analyzer" << endl;
	cout << "请手动创建目录 gen_data/syntactic_analyzer" << endl;
	cout << "请手动创建目录 gen_data/target_file" << endl << endl;
	cout << "词法分析的结果存放在lexcial_analyzer中" << endl;
	cout << "语法分析的结果存放在syntactic_analyzer中" << endl;
	cout << "中间代码与目标代码的结果存放在target_file中" << endl << endl;
	string file_name = "./raw_data/";
	string str;
	cout << "请指定在gen_data/目录下需要编译的cpp源文件：" << endl;
	cin >> str;
	file_name  = file_name + str;
	SyntacticAnalyzer sa;
	sa.StartAnalize(file_name);


	return 0;
}