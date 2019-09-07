#pragma once
#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include<iostream>
#include<fstream>
#include<string>
#include<set>
#include "utils.h"

using namespace std;

class LexicalAnalyzer {
private:
	//所有运算符
	set<string> OPERATORS = { "+", "-", "*", "/", "=", "==", ">", ">=", "<", "<=", "!="};

	//关键字
	set<string> KEYWORDS = { "if", "else", "return", "while"};

	//变量类型
	set<string> TYPES = { "int", "void"};

	//前置运算符
	set<string> PRE_OPERATORS = { "+", "-"};

	//界符
	set<char> BORDERS = { '(', ')', '{', '}', ',', ';'};

	ifstream code_reader_;
	ofstream lexical_analyser_printer_;

	unsigned int line_counter_;//用于词法分析发生错误时的行数定位
	bool print_detail_;//选择是否将词法分析结果输出到txt中
	unsigned int step_counter_;//词法分析计步器

	bool IsLetter(const unsigned char ch);//是否字母
	bool IsDigital(const unsigned char ch);//是否数字
	bool IsSingleCharOperator(const unsigned char ch);//是否单符号运算符
	bool IsDoubleCharOperatorPre(const unsigned char ch);//是否双符号运算符
	bool IsBlank(const unsigned char ch);//是否是空白符
	unsigned char GetNextChar();//获取字符流中的下一个字符，同时计算行数。
	void PrintDetail(WordInfo word);//打印词法分析信息
	WordInfo GetBasicWord();//获取文法符号,但是无法进行打印

public:
	LexicalAnalyzer();
	~LexicalAnalyzer();
	bool IsReadyToAnalyze(bool show_detail = true, const string code_filename="./raw_data/test.cpp");//文件是否读到
	WordInfo GetWord();//获取文法符号，可进行打印信息。

};


#endif