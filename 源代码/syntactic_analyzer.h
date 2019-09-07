#pragma once
#ifndef SYNTACTIC_ANALYZER
#define SYNTACTIC_ANALYZER

#include "lexical_analyzer.h"
#include "semantic_analyzer.h"
#include <map>


enum SLR_OPERATIONS { CONCLUDE, MOVE, ACCEPT, NONTERMINAL, ERROR };//规约、移进、接受、压入非终结符、出错
struct SlrOperation//ACTION或者GOTO操作
{
	SLR_OPERATIONS op;//规约、移进、接受
	int state;//item中的序号，从0开始
	bool operator==(const SlrOperation &operation) const;//判断两个操作是否相同
};

//LR项目
struct LrItem {
	int production_number;//项目所对应的产生式在 productions_ 的序号
	int point_pos;//项目中小圆点的位置，从0开始，最大值等于产生式右部符号数目
	LrItem(int item_num, int point_num) { this->production_number = item_num, this->point_pos = point_num; };
	bool operator==(const LrItem &item) const;//判断两个item是否相同
	bool operator<(const LrItem &item) const;//没有重载<操作的时候，无法插入
};



class SyntacticAnalyzer {
private:
	bool print_detail_;
	vector<Grammar> grammars_;
	vector<Production> productions_;//将gammars_中的产生式拆开成多个产生式，方便编号。
	set<string> grammar_symbol_;//语法的所有符号，包括终结符与非终结符。
	map<string, set<string>> first_map_, follow_map_;//First集合、Follow集合
	vector<set<LrItem>> normal_family_;//LR0项目集规范族
	set<LrItem> lr_items_;//LR0所有项目
	map<pair<int, string>, SlrOperation> action_goto_tables_;//action 与 goto 表的融合
	ofstream syntactic_analyzer_printer_;//语法分析过程打印
	vector<string> move_conclude_string_stack_;//存放 移进规约串的栈
	vector<int> state_sequence_stack_;//状态序列栈
	SemanticAnalyzer sementic_analyzer_;//语义分析器，在规约的时候被使用，进行语义检查
	vector<GrammarSymbolInfo> grammar_symbol_info_stack_;//语义分析的时候用到的文法符号属性栈

	bool IsNonTerminalSymbol(const string &symbol);//判断是否为非终结符

	bool GenProduction();//从grammar.txt中生成产生式
	void GenAugmentedGrammar();//生成拓广文法
	void GenFirstSet();//生成First集合
	void GenFollowSet();//生成Follow集合
	void GenGrammarSymbolSet();//生成文法符号集合
	bool GenNormalFamilySet();//生成项目集规范族、Action与Goto表，返回false则说明文法不是SLR，带有冲突
	void GenLrItems();//生成LR0所有项目
	bool BuildGrammar();//对grammar.txt文件进行分析，生成以上所有集合。
	set<LrItem> GenItemClosureSet(const LrItem &);//生成某个项目的闭包集合
	set<LrItem> GenItemsClosureSet(const set<LrItem> &);//生成某个项目集的闭包集合
	set<string> GetProductionFirstSet(const vector<string> &);//获取某个串的First集合


	void PrintBuildGrammarDetails();//对grammar.txt文件进行分析，生成以上所有集合。
	void PrintProdcutions(const string filename = "./gen_data/syntactic_analyzer/productions.txt");//打印产生式
	void PrintGrammars();//打印文法
	void PrintFirst(const string filename = "./gen_data/syntactic_analyzer/fisrts.txt");//打印First集合
	void PrintFollow(const string filename = "./gen_data/syntactic_analyzer/follows.txt");//打印Follow集合
	void PrintGrammarSymbolSet(const string filename = "./gen_data/syntactic_analyzer/symbols.txt");//打印GrammarSymbol集合
	void PrintLrItems(const string filename = "./gen_data/syntactic_analyzer/items.txt");//打印LrItems集合
	void PrintClosure();//打印Closure集合，debug时候使用
	void PrintSlrError(const set<LrItem> &);//打印SLR分析中存在冲突的规范族，出现错误的时候直接显示在CMD窗口上
	void PrintNormalFamiliySet(const string filename = "./gen_data/syntactic_analyzer/normal_families.txt");//打印所有规范族
	void PrintActionGotoTable(const string filename = "./gen_data/syntactic_analyzer/action_goto_tables.csv");//打印Action和Goto表

	void PrintAnalysisProcess(int step, const SlrOperation &sl_op);//打印语法分析过程

public:
	SyntacticAnalyzer(bool show_detail = true);
	~SyntacticAnalyzer();
	bool StartAnalize(const string code_filename = "./raw_data/test.cpp");//开始进行语法分析
	

};


#endif