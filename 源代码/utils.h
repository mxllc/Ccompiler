#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>

using namespace std;

enum CATEGORY { FUNCTION, VARIABLE, CONST, TEMP, RETURN_VAR }; //符号表中符号的种类：函数、变量、常量、临时变量、返回变量
enum VARIABLE_TYPE { INT, VOID };//

enum LEXICAL_TYPE {
	LCINT, LKEYWORD, LIDENTIFIER, LTYPE, LBORDER, LUNKNOWN, LEOF, LOPERATOR, LBEGIN,
	LCFLOAT, LCDOUBLE, LCCHAR, LCSTRING, LCERROR, LOERROR
};//C开头的代表常数类型,LCERROR表示常数错误,LOERROR表示运算符错误，LWERROR表示单词错误,LNO表示空


struct WordInfo//综合属性
{
	LEXICAL_TYPE type;//词类型
	string value;//常量的值，或者关键字本身
	string word_string;//进行语义分析时的符号

	//SymbolLocation location;// 在语义分析时代表常量符号表中相应常量的序号
	//vector<string> modifiers;//修饰符，仅对需要多个修饰符的语法有用，例如Type-(+)->const signed int ***,('+'表示多步推导)，产生式右部多次规约到Type，Type的value应包含const、signed、int、*、*、*等多个值

	//仅在函数调用的产生式用到，表示函数在全局表中的序号
	int functionIndex;
};


struct SymbolPos {
	int table_pos;//变量、常量、临时变量 所在table在所有table中的序号
	int symbol_pos;//变量、常量、临时变量 所在table中的位置
	SymbolPos(int tp, int sp) { table_pos = tp; symbol_pos = sp; }
	SymbolPos() {};
};
struct GrammarSymbolInfo//语义分析时候用到的文法符号属性
{
	string symbol_name;//对应与grammar中的名称
	string txt_value;//对应的文本属性值
	SymbolPos pos;//table,symbol的位置
	string op;//表示 +, -, *, /
};

struct Grammar//右部包含多个
{
	string left;
	vector<string> right;
};

struct Production//产生式
{
	string left;
	vector<string> right;
};

struct Quadruples//四元式
{
	int num;//行号
	string op;
	string arg1;
	string arg2;
	string result;

	Quadruples();
	Quadruples(int n, const string &o, const string &a1, const string &a2, const string &r);
	void SetContent(int n, const string &o, const string &a1, const string &a2, const string &r);
};

struct Instructions//目标代码指令
{
	string op;
	string arg1;
	string arg2;
	string arg3;
	Instructions();
	Instructions(const string &op, const string & arg1, const string & arg2, const string & arg3);
	void SetContent(const string &op, const string & arg1, const string & arg2, const string & arg3);
};

//struct SymbolOffset
//{
//	int table_pos;
//	int offset;
//	CATEGORY mode;
//	SymbolOffset();
//	SymbolOffset(int tableIndex, int offset, CATEGORY mode);
//};

struct RegisterInfo//临时寄存器的信息 方便寄存器值和内存之间传递
{
	const string name;//"$t0"之类的
	string content;//具体的数值
	SymbolPos content_info;//content的信息,通过Pos来定位symbol的位置。从而获取symbol的信息
	int miss_time = 0;//未使用次数，没有空闲寄存器时，腾出最久未被使用的寄存器
	bool is_possessed = false;//表示是否被占用。 在从函数return之后，需要释放局部定义的变量所占用的寄存器
};

#endif