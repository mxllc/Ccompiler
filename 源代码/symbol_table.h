#pragma once
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include "utils.h"
#include <vector>

//符号表中的符号
struct Symbol
{
	CATEGORY mode;//MODE { FUNCTION, VARIABLE, CONST, TEMP }; //符号表中符号的类型：函数、变量、常量、临时变量
	string name;
	VARIABLE_TYPE type;//如果是变量，就是变量的类型；如果是函数，就是函数的返回类型
	string value;//存储常量值


	int parameter_num = 0;//如果是函数，此变量表示参数个数
	int entry_address;//函数入口地址
	int func_symbol_pos;//函数表在符号表中的位置,即这个函数表是第几张表
	int function_table_num;//函数形式参数 在函数符号表中的序号

	int offset = -1;//TEMP:以$gp为启始的偏移地址 这里的1表示4个bytes;  -1表示无效  VARIABLE:在栈帧中的位置
	int reg = -1;//MIPS的10个寄存器 t0-t9； 有效 0-9  使用-1表示无
	//offset与reg搭配使用
	//o-1, r-1		表示还没装载到reg，也没有保存到内存
	//o-1, r>-1		表示寄存器上的值是有效的
	//o>-1, r-1		表示reg中的值是无效的，但是内存中有保存
};



enum TABLE_TYPE { CONST_TABLE, TEMP_TABLE, GLOBAL_TABLE, FUNCTION_TABLE, WHILE_TABLE, IF_TABLE };//常量、临时变量、全局、函数、循环体、选择分支

class SymbolTable
{
private:
	vector<Symbol> table_;//用vector来存放各个符号，形成一张符号表
	TABLE_TYPE table_type_;//符号表的类型
	string name_;//符号表的名称


public:
	SymbolTable(TABLE_TYPE, string = "");
	~SymbolTable();
	TABLE_TYPE GetTableType();//返回整个符号表的类型
	int AddSymbol(const Symbol &);//向符号表中添加符号并返回符号在符号表中的序号。若存在符号，则返回-1
	int AddSymbol(const string &);//向临时符号表中添加符号并返回符号在符号表中的序号
	int AddSymbol();//向临时符号表中添加符号并返回符号在符号表中的序号

	int FindSymbol(const string &) const;//在符号表中寻找是否存在指定名称的符号，没有则返回-1，有则返回序号
	int FindConst(const string &) const;//在符号表中寻找是否存在指定的常数，没有则返回-1，有则返回序号
	bool SetValue(int pos, string &);//根据pos来设置value
	string GetSymbolName(int pos) const;//根据pos获取符号名称
	CATEGORY GetSymbolMode(int pos)const;//根据pos获取符号模式
	VARIABLE_TYPE GetSymbolType(int pos)const;//根据pos获取符号类型
	string GetTableName()const;//获取当前符号表的名称
	Symbol &GetSymbol(int pos);//根据pos获取符号
	vector<Symbol> &GetTable();//返回整个符号表

};

#endif