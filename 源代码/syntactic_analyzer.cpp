#include "syntactic_analyzer.h"

vector<string> split_string(const string &str, const string &pattern)
{
	vector<string> res;
	if (str == "")
		return res;
	//在字符串末尾也加入分隔符，方便截取最后一段
	string strs = str + pattern;
	size_t pos = strs.find(pattern);

	while (pos != strs.npos)
	{
		string temp = strs.substr(0, pos);
		res.push_back(temp);
		//去掉已分割的字符串,在剩下的字符串中进行分割
		strs = strs.substr(pos + 1, strs.size());
		pos = strs.find(pattern);
	}

	//debug: grammar中sp检测
	//for (auto iter = res.begin(); iter != res.end(); ++iter)
	//	if ((*iter) == "")
	//		cout << "***************************";
	return res;
}

set<string> SyntacticAnalyzer::GetProductionFirstSet(const vector<string> & symbol_string)
{
	set<string> first;
	for (auto iter1 = symbol_string.begin(); iter1 != symbol_string.end(); ++iter1)
	{
		//不可能为$,会提前做判断
		//if ((*iter1) == "$")
		//	break;

		if (!IsNonTerminalSymbol(*iter1))//循环到终结符
		{
			first.insert(*iter1);
			break;//终结符需要跳出循环
		}

		for (auto iter2 = first_map_[*iter1].begin(); iter2 != first_map_[*iter1].end(); ++iter2)//遍历产生式右部符号对应的First集合
		{
			if ((*iter2) != "$" && first.find(*iter2) == first.end())//非空也不重复
				first.insert(*iter2);
		}


		//当前符号first集合中没有空$，退出这条产生式的循环。
		if (first_map_[*iter1].find("$") == first_map_[*iter1].end())
			break;

		//每一个符号的first都包含空，且已经循环到最后一个符号；这个时候整个串的first集合也应该包含空
		if (iter1 + 1 == symbol_string.end() && first.find("$") == first.end())
			first.insert("$");
	}
	return first;
}

bool SyntacticAnalyzer::GenProduction()
{
	string grammar_filename = "./raw_data/grammar.txt";
	ifstream grammar_reader;

	grammar_reader.open(grammar_filename);

	if (!grammar_reader.is_open()) {
		cerr << "语法分析过程中，grammar文件打开失败！" << endl;
		return false;
	}

	string grammar_str;
	while (!grammar_reader.eof())
	{
		getline(grammar_reader, grammar_str);
		if (grammar_str.length() == 0)//空串
			continue;

		string left;
		vector<string> right;

		size_t left_pos = grammar_str.find_first_of("->");
		if (left_pos < 0)
		{
			cerr << "grammar出现错误，某个产生式没有\" -> \" 符号，请规范！" << endl;
			grammar_reader.close();
			return false;
		}
		left = grammar_str.substr(0, left_pos);

		//跳过"->"
		grammar_str = grammar_str.substr(left_pos + 2);


		while (true) //处理产生式右部
		{
			size_t right_pos = grammar_str.find_first_of("|");
			if (right_pos == string::npos)//最后一个候选式
			{
				right.push_back(grammar_str);
				//productions_.push_back({ left, split_string(grammar_str, " ") });//将产生式分割


				vector<string> split_production = split_string(grammar_str, " ");//将产生式分割
				productions_.push_back({ left,  split_production });
				//添加终结符的grammar_symbol_集合
				for (auto iter = split_production.begin(); iter != split_production.end(); ++iter)
					if (!IsNonTerminalSymbol(*iter) && (*iter)!= "$")
						grammar_symbol_.insert(*iter);
				
				break;
			}
			//不是最后一个候选式
			//string debug_s = grammar_str.substr(left_pos, right_pos - left_pos);
			right.push_back(grammar_str.substr(0, right_pos));

			vector<string> split_production = split_string(grammar_str.substr(0, right_pos), " ");//将产生式分割
			productions_.push_back({ left,  split_production });
			//添加终结符的grammar_symbol_集合
			for (auto iter = split_production.begin(); iter != split_production.end(); ++iter)
				if (!IsNonTerminalSymbol(*iter) && (*iter) != "$")
					grammar_symbol_.insert(*iter);

			grammar_str = grammar_str.substr(right_pos + 1);
		}

		grammars_.push_back({ left, right });
	}
	
	grammar_reader.close();
	return true;

}

void SyntacticAnalyzer::GenAugmentedGrammar()
{
	//vector<Production> productions_
	/*string left = productions_[0].left + "'";*/
	string left = "Start";
	vector <string> right;
	right.push_back(productions_[0].left);
	productions_.insert(productions_.begin(), { left , right });
	return;
}

void SyntacticAnalyzer::GenFirstSet()
{
	for (auto iter = productions_.begin(); iter != productions_.end(); ++iter)//遍历每一个产生式
	{
		//vector<string> debug = iter->right;
		first_map_.insert({ iter->left, set<string>{} });//创建FirstMap
		follow_map_.insert({ iter->left, set<string>{} });//创建FollowMap
		if (iter->right[0] == "$")
		{
			if (first_map_[iter->left].find("$") == first_map_[iter->left].end())//往first集合中添加 空$
				first_map_[iter->left].insert("$");
		}
	}

	while (true)
	{
		bool is_change = false;
		for (auto iter1 = productions_.begin(); iter1 != productions_.end(); ++iter1)//遍历每一个产生式
		{
			for (auto iter2 = iter1->right.begin(); iter2 != iter1->right.end(); ++iter2)//遍历产生式右部
			{
				if ((*iter2) == "$")
					break;

				if (!IsNonTerminalSymbol(*iter2))//循环到终结符
				{
					if (first_map_[iter1->left].find(*iter2) == first_map_[iter1->left].end())
					{
						first_map_[iter1->left].insert(*iter2);
						is_change = true;
					}
					break;//终结符需要跳出循环

				}
				//map 使用[]之后，如果原来没有对应的key，会自动创建。
				//string gebug_str = *iter2;
				//auto debug_iter1 = first_map_[*iter2].begin();
				//auto debug_iter2 = first_map_[*iter2].end();


				for (auto iter3 = first_map_[*iter2].begin(); iter3 != first_map_[*iter2].end(); ++iter3)//遍历产生式右部符号对应的First集合
				{
					if ((*iter3) != "$" && first_map_[iter1->left].find(*iter3) == first_map_[iter1->left].end())//非空也不重复
					{
						first_map_[iter1->left].insert(*iter3);
						is_change = true;
					}
				}

				//当前符号first集合中没有空$，退出这条产生式的循环。
				if (first_map_[*iter2].find("$") == first_map_[*iter2].end())
					break;
				
				//每一个符号的first都包含空，且已经循环到最后一个符号；这个时候产生式左部的first也包含空
				if (iter2 + 1 == iter1->right.end() && first_map_[iter1->left].find("$") == first_map_[iter1->left].end())
				{
					first_map_[iter1->left].insert("$");
					is_change = true;
				}

			}
		}


		if (!is_change)//未发生改变，生成First完成
			break;
	}
	return;
}

void SyntacticAnalyzer::GenFollowSet()
{
	//GetProductionFirstSet()
	follow_map_[productions_[0].left].insert("#");//文法开始符号，添加#

	while (true)
	{
		bool is_change = false;
		for (auto iter1 = productions_.begin(); iter1 != productions_.end(); ++iter1)//遍历每个产生式
		{
			for (auto iter2 = iter1->right.begin(); iter2 != iter1->right.end(); ++iter2)//遍历产生式右部每个符号
			{
				if (!IsNonTerminalSymbol(*iter2))//当前符号为终结符，跳过
					continue;

				//非终结符的情况
				//======================================注意===检查=====
				set<string> first = GetProductionFirstSet(vector<string>(iter2 + 1, iter1->right.end()));
				for (auto iter = first.begin(); iter != first.end(); ++iter)
				{
					if ((*iter) != "$" && follow_map_[*iter2].find(*iter) == follow_map_[*iter2].end())//非空且还没有加入
					{
						follow_map_[*iter2].insert(*iter);
						is_change = true;
					}				
				}
				if (first.empty() || first.find("$") != first.end())//把产生式左部的follow集合加到当前非终结符的follow集合中
				{
					for (auto iter = follow_map_[iter1->left].begin(); iter != follow_map_[iter1->left].end(); ++iter)
					{
						if(follow_map_[*iter2].find(*iter) == follow_map_[*iter2].end())//还没有加入过
						{
							follow_map_[*iter2].insert(*iter);
							is_change = true;
						}
					}
				}
				
			}
			
		}

		if (!is_change)//未发生改变，生成Follow完成
			break;
	}


	return;
}

void SyntacticAnalyzer::GenGrammarSymbolSet()
{
	//把非终结符添加入文法符号集合中
	for (auto iter = first_map_.begin(); iter != first_map_.end(); ++iter)
		grammar_symbol_.insert(iter->first);
	
	grammar_symbol_.insert("#");//终结符#一起添加进文法符号集合中
	return;
}

void SyntacticAnalyzer::GenLrItems()
{
	for (auto iter = productions_.begin(); iter != productions_.end(); ++iter)
	{
		if (iter->right[0] == "$")//产生式右部是空串，LR0项目 只有一个 pos用-1来特殊表示
		{
			lr_items_.insert(LrItem{ iter - productions_.begin(), -1 });//插入产生式编号与 -1（表示空）A->epsilon 只生成A->·
			continue;
		}
		int production_lenth = iter->right.size();
		for (int count = 0; count <= production_lenth; ++count)
		{
			//auto debug = iter - productions_.begin();
			//auto debug2 = LrItem{ iter - productions_.begin(), count };
			//debug2 这里需要添加 LrItem的<重载能能正常运行
			lr_items_.insert(LrItem{ iter - productions_.begin(), count });//插入产生式编号与小圆点位置	
			//PrintLrItems();
		}
	}
	return;
}

set<LrItem> SyntacticAnalyzer::GenItemClosureSet(const LrItem &input_item)
{
	vector<LrItem> item_stack;//用来暂存item
	item_stack.push_back(input_item);
	set<LrItem> item_set;//最终返回的Item

	while (!item_stack.empty())//暂存的栈不空，就需要一直循环
	{
		//从栈中取出Item
		LrItem item = item_stack[item_stack.size() - 1];
		item_stack.pop_back();
		//插入到闭包中
		item_set.insert(item);

		//A->·
		if (-1 == item.point_pos)
			continue;

		//小圆点在最后面，表示规约； 这里先判断，防止下面对vector的越界访问
		if (item.point_pos == productions_[item.production_number].right.size())
			continue;
		//终结符
		if (!IsNonTerminalSymbol(productions_[item.production_number].right[item.point_pos]))
			continue;

		string current_symbol = productions_[item.production_number].right[item.point_pos];
		//非终结符
		for (auto iter = lr_items_.begin(); iter != lr_items_.end(); ++iter)
		{
			//以当前非终结符为左部，并且小圆点的位置在第0位 或者 -1位（A->·）
			if (productions_[iter->production_number].left == current_symbol && (iter->point_pos == 0 || iter->point_pos == -1))
			{
				if(item_set.find(*iter) == item_set.end())//保证之前不在set中，否则会陷入死循环
					item_stack.push_back(*iter);
			}
		}
	}

	return item_set;
}

set<LrItem> SyntacticAnalyzer::GenItemsClosureSet(const set<LrItem> &items)
{
	vector<LrItem> item_stack;//用来暂存item
	for(auto iter = items.begin();iter!=items.end();++iter)
		item_stack.push_back(*iter);
	//item_stack.push_back(input_item);
	set<LrItem> item_set;//最终返回的Item

	while (!item_stack.empty())//暂存的栈不空，就需要一直循环
	{
		//从栈中取出Item
		LrItem item = item_stack[item_stack.size() - 1];
		item_stack.pop_back();
		//插入到闭包中
		item_set.insert(item);

		//A->·
		if (-1 == item.point_pos)
			continue;

		//小圆点在最后面，表示规约； 这里先判断，防止下面对vector的越界访问
		if (item.point_pos == productions_[item.production_number].right.size())
			continue;
		//终结符
		if (!IsNonTerminalSymbol(productions_[item.production_number].right[item.point_pos]))
			continue;

		string current_symbol = productions_[item.production_number].right[item.point_pos];
		//非终结符
		for (auto iter = lr_items_.begin(); iter != lr_items_.end(); ++iter)
		{
			//以当前非终结符为左部，并且小圆点的位置在第0位 或者 -1位（A->·）
			if (productions_[iter->production_number].left == current_symbol && (iter->point_pos == 0 || iter->point_pos == -1))
			{
				if (item_set.find(*iter) == item_set.end())//保证之前不在set中，否则会陷入死循环
					item_stack.push_back(*iter);
			}
		}
	}

	return item_set;
}

bool SyntacticAnalyzer::GenNormalFamilySet()
{
	//把起始字符的Closure作为第0个规范族
	//LrItem debug = *lr_items_.begin();

	vector<set<LrItem>> item_stack;//用来临时存放NormalFamiliySet
	item_stack.push_back(GenItemClosureSet(*lr_items_.begin()));
	normal_family_.push_back(item_stack[0]);

	while (!item_stack.empty())
	{
		//从栈中取出NormaliFamilySet
		set<LrItem> item = item_stack[item_stack.size() - 1];
		item_stack.pop_back();
		
		
		//找到当前规范族的序号
		int current_state = find(normal_family_.begin(), normal_family_.end(), item) - normal_family_.begin();

		for (auto iter1 = item.begin(); iter1 != item.end(); ++iter1)//对于规范族中的每个LR0项目
		{
			//小圆点在第-1位置或者最后面，表示是需要规约的项目
			if (-1 == iter1->point_pos  || (productions_[iter1->production_number].right.size() == iter1->point_pos))
			{
				//先求follow集合
				string symbol = productions_[iter1->production_number].left;//产生式左部
				//对follow集合进行遍历，加入action_goto表中，同时进行错误的判断
				for (auto iter2 = follow_map_[symbol].begin(); iter2 != follow_map_[symbol].end(); ++iter2)
				{
					//action_goto_table中没有，这个时候添加进去
					if (action_goto_tables_.find({ current_state, *iter2 }) == action_goto_tables_.end())
					{
						//通过当前的 状态+输入字符（这里是左部的follow集合）,对应为 规约，使用第iter1->production_number产生式
						action_goto_tables_.insert({ { current_state, *iter2},  {CONCLUDE,iter1->production_number } });
					}
					else//表示已经存在映射规则
					{
						//存在映射但是却不相等，说明不是SLR文法
						if (!(action_goto_tables_[{ current_state, *iter2 }] == SlrOperation{ CONCLUDE,iter1->production_number }))
						{
							PrintSlrError(item);
							return false;
						}
					}
				}
			}
			else
			{
				//产生式右部的当前符号
				string current_right_symbol = productions_[iter1->production_number].right[iter1->point_pos];
				//起到GO函数的作用，求下一个规范族
				//set<LrItem> next_normal_family = GenItemClosureSet({ iter1->production_number, iter1->point_pos + 1 });
				//======================修改================================================================================
				set<LrItem> items;//存储 接受相同字符的item
				for (auto iter2 = item.begin(); iter2 != item.end(); ++iter2)
				{
					//防止越界
					if (iter2->point_pos == -1 || (iter2->point_pos == productions_[iter2->production_number].right.size()))
						continue;
					//auto debug = productions_[iter1->production_number].right[iter1->point_pos];
					//auto debug2 = productions_[iter2->production_number].right[iter2->point_pos];
					if(productions_[iter1->production_number].right[iter1->point_pos] == 
						productions_[iter2->production_number].right[iter2->point_pos])
						items.insert({ iter2->production_number, iter2->point_pos + 1 });
					/*if (iter1->point_pos + 1 == iter2->point_pos + 1)*/
				}
				set<LrItem> next_normal_family = GenItemsClosureSet(items);


				//auto debug = find(normal_family_.begin(), normal_family_.end(), next_normal_family);
				//auto debug2 = normal_family_.begin();
				//auto debug3 = next_normal_family;
				//判断当前规范族是否已经存在
				if (find(normal_family_.begin(), normal_family_.end(), next_normal_family) == normal_family_.end())//不存在
				{
					normal_family_.push_back(next_normal_family);//插入规范族集合中
					item_stack.push_back(next_normal_family);//插入待处理堆栈中
				}
				int next_state = find(normal_family_.begin(), normal_family_.end(), next_normal_family) - normal_family_.begin();

				//action_goto_table中没有，这个时候添加进去
				if (action_goto_tables_.find({ current_state, current_right_symbol }) == action_goto_tables_.end())
				{
					//通过当前的 状态+输入字符（这里是左部的follow集合）,对应为 移进，跳转到第next_state个状态
					action_goto_tables_.insert({ { current_state, current_right_symbol},  {MOVE, next_state } });
				}
				else//表示已经存在映射规则
				{
					//存在映射但是却不相等，说明不是SLR文法
					if (!(action_goto_tables_[{ current_state, current_right_symbol }] == SlrOperation{ MOVE, next_state }))
					{
						PrintSlrError(item);
						return false;
					}
				}


			}

			


		}
	}
	int current_state2 = -1;
	//插入acc状态
	for (auto iter1 = normal_family_.begin(); iter1 != normal_family_.end(); ++iter1)
	{
		for(auto iter2 = iter1->begin();iter2!=iter1->end();++iter2)
			if (LrItem(0, 1) == *iter2)
			{
				current_state2 = iter1 - normal_family_.begin();
				break;
			}
		if (current_state2 >= 0)
			break;
	}
	set<LrItem> item = { LrItem(0,1) };
	//auto debug = find(normal_family_.begin(), normal_family_.end(), item) == normal_family_.end();
	action_goto_tables_[{current_state2, "#"}] = { ACCEPT, current_state2 };

	//map<pair<int, string>, SlrOperation> action_goto_tables_;//action 与 goto 表的融合
	//enum SLR_OPERATIONS { CONCLUDE, MOVE, ACCEPT, NONTERMINAL, ERROR };//规约、移进、接受、压入非终结符、出错
	//struct SlrOperation//ACTION或者GOTO操作
	//{
	//	SLR_OPERATIONS op;//规约、移进、接受
	//	int state;//item中的序号，从0开始
	//};

	//一边构造规范族，一边构造action_goto表

	return true;
}

bool SyntacticAnalyzer::BuildGrammar()
{
	GenProduction();
	GenAugmentedGrammar();
	GenFirstSet();
	GenFollowSet();
	GenGrammarSymbolSet();
	GenLrItems();
	return GenNormalFamilySet();
}

void SyntacticAnalyzer::PrintGrammars()
{
	for (auto iter1 = grammars_.begin(); iter1 != grammars_.end(); iter1++)
	{
		cout << (*iter1).left << "->";
		for (auto iter2 = (iter1->right).begin(); iter2 != (iter1->right).end(); iter2++)
			cout << (*iter2) << "|";
		cout << "\b " << endl;
	}
	return;
}

void SyntacticAnalyzer::PrintProdcutions(const string filename)
{
	ofstream printer;
	printer.open(filename);

	if (!printer.is_open()) {
		cerr << "语法分析过程中，" << filename << "文件打开失败！" << endl;
		return ;
	}
	//for (auto iter1 = grammars_.begin(); iter1 != grammars_.end(); iter1++)
	//{
	//	printer << (*iter1).left << "->";
	//	for (auto iter2 = (iter1->right).begin(); iter2 != (iter1->right).end(); iter2++)
	//	{
	//		printer << (*iter2);
	//		if((iter1->right).end() - iter2 != 1)
	//			printer << "|";
	//	}
	//	printer << endl;
	//}

	for (auto iter1 = productions_.begin(); iter1 != productions_.end(); iter1++)
	{
		printer << (*iter1).left << "->";
		for (auto iter2 = (iter1->right).begin(); iter2 != (iter1->right).end(); iter2++)
		{
			printer << (*iter2) << ' ';
		}
		printer << endl;
	}

	//productions_
	return;
}

void SyntacticAnalyzer::PrintFirst(const string filename)
{
	ofstream printer;
	printer.open(filename);

	if (!printer.is_open()) {
		cerr << "语法分析过程中，" << filename << "文件打开失败！" << endl;
		return;
	}
	printer << "First集合：" << endl;
	for (auto iter1 = first_map_.begin(); iter1 != first_map_.end(); ++iter1)
	{
		//for (auto iter2 = iter1->begin(); )
		printer << (*iter1).first << ": ";
		int count = 0;
		int size_ = (*iter1).second.size();
		for (auto iter2 = (*iter1).second.begin(); iter2 != (*iter1).second.end(); ++iter2)
		{
			printer << (*iter2);
			if (size_ > count + 1)
				printer << " ";
		}
		printer << endl;
	}
	return;
}

void SyntacticAnalyzer::PrintFollow(const string filename)
{
	ofstream printer;
	printer.open(filename);

	if (!printer.is_open()) {
		cerr << "语法分析过程中，" << filename << "文件打开失败！" << endl;
		return;
	}
	printer << "Follow集合：" << endl;
	for (auto iter1 = follow_map_.begin(); iter1 != follow_map_.end(); ++iter1)
	{
		//for (auto iter2 = iter1->begin(); )
		printer << (*iter1).first << ": ";
		int count = 0;
		int size_ = (*iter1).second.size();
		for (auto iter2 = (*iter1).second.begin(); iter2 != (*iter1).second.end(); ++iter2)
		{
			printer << (*iter2);
			if (size_ > count + 1)
				printer << " ";
		}
		printer << endl;
	}
	return;
}

void SyntacticAnalyzer::PrintGrammarSymbolSet(const string filename)
{	
	ofstream printer;
	printer.open(filename);
	if (!printer.is_open()) {
		cerr << "语法分析过程中，" << filename << "文件打开失败！" << endl;
		return;
	}

	printer << "文法符号（终结符与非终结符）：" << endl;
	for (auto iter = grammar_symbol_.begin(); iter != grammar_symbol_.end(); ++iter)
	{
		printer << *iter << endl;
	}
	return;
}

void SyntacticAnalyzer::PrintLrItems(const string filename)
{
	ofstream printer;
	printer.open(filename);
	if (!printer.is_open()) {
		cerr << "语法分析过程中，" << filename << "文件打开失败！" << endl;
		return;
	}
	printer << "LR0项目：" << endl;
	for (auto iter1 = lr_items_.begin(); iter1 != lr_items_.end(); ++iter1)
	{
		//if (productions_[iter1->production_number].right[0] == "$")//跳过空串
		//	continue;
		printer << productions_[iter1->production_number].left << "->";

		if (-1 == iter1->point_pos)//空串，只有一个点
		{
			printer << "·" << endl;
			continue;
		}

		int production_length = productions_[iter1->production_number].right.size();

		for (auto iter2 = productions_[iter1->production_number].right.begin();
			iter2 != productions_[iter1->production_number].right.end(); iter2++)
		{
			if (iter1->point_pos == (iter2 - productions_[iter1->production_number].right.begin()))
				printer << "· ";
			printer << *iter2 << " ";

			//最后一个字符的情况
			if (iter1->point_pos == production_length && 
				(productions_[iter1->production_number].right.end() - iter2 == 1))
				printer << "·";
		}

		printer << endl;
	}
	return;
}

void SyntacticAnalyzer::PrintClosure()
{
	cout << "Closure项目：" << endl;
	for (auto iter = lr_items_.begin(); iter != lr_items_.end(); ++iter)
	{
		set<LrItem> lr = GenItemClosureSet(*iter);

		for (auto iter1 = lr.begin(); iter1 != lr.end(); ++iter1)
		{
			cout << productions_[iter1->production_number].left << "->";

			if (-1 == iter1->point_pos)//空串，只有一个点
			{
				cout << "·" << endl;
				continue;
			}

			int production_length = productions_[iter1->production_number].right.size();

			for (auto iter2 = productions_[iter1->production_number].right.begin();
				iter2 != productions_[iter1->production_number].right.end(); iter2++)
			{
				if (iter1->point_pos == (iter2 - productions_[iter1->production_number].right.begin()))
					cout << "· ";
				cout << *iter2 << " ";

				//最后一个字符的情况
				if (iter1->point_pos == production_length &&
					(productions_[iter1->production_number].right.end() - iter2 == 1))
					cout << "·";
			}
			cout << endl;
		}
		cout << endl;
	}
	
}

void SyntacticAnalyzer::PrintSlrError(const set<LrItem> & normal_family)
{
	cerr << "不是SLR文法！！！ 冲突项目的规范族：" << endl;
	set<LrItem> lr = normal_family;

	for (auto iter1 = lr.begin(); iter1 != lr.end(); ++iter1)
	{
		cerr << productions_[iter1->production_number].left << "->";

		if (-1 == iter1->point_pos)//空串，只有一个点
		{
			cerr << "·" << endl;
			continue;
		}

		int production_length = productions_[iter1->production_number].right.size();

		for (auto iter2 = productions_[iter1->production_number].right.begin();
			iter2 != productions_[iter1->production_number].right.end(); iter2++)
		{
			if (iter1->point_pos == (iter2 - productions_[iter1->production_number].right.begin()))
				cerr << "· ";
			cerr << *iter2 << " ";

			//最后一个字符的情况
			if (iter1->point_pos == production_length &&
				(productions_[iter1->production_number].right.end() - iter2 == 1))
				cout << "·";
		}
		cerr << endl;
	}
	cerr << endl;
}

void  SyntacticAnalyzer::PrintNormalFamiliySet(const string filename)
{
	ofstream printer;
	printer.open(filename);
	if (!printer.is_open()) {
		cerr << "语法分析过程中，" << filename << "文件打开失败！" << endl;
		return;
	}
	printer << "项目集规范族：" << endl;
	for (auto iter1 = normal_family_.begin(); iter1 != normal_family_.end(); ++iter1)
	{
		printer << "规范族 " << iter1 - normal_family_.begin() << " : " << endl;
		for (auto iter2 = iter1->begin(); iter2 != iter1->end(); ++iter2)
		{

			printer << productions_[iter2->production_number].left << "->";

			if (-1 == iter2->point_pos)//空串，只有一个点
			{
				printer << "·" << endl;
				continue;
			}

			int production_length = productions_[iter2->production_number].right.size();

			for (auto iter3 = productions_[iter2->production_number].right.begin();
				iter3 != productions_[iter2->production_number].right.end(); iter3++)
			{
				if (iter2->point_pos == (iter3 - productions_[iter2->production_number].right.begin()))
					printer << "· ";
				printer << *iter3 << " ";

				//最后一个字符的情况
				if (iter2->point_pos == production_length &&
					(productions_[iter2->production_number].right.end() - iter3 == 1))
					printer << "·";
			}
			printer << endl;
		}
		printer << endl;
	}
}

void SyntacticAnalyzer::PrintActionGotoTable(const string filename)
{
	ofstream printer;
	printer.open(filename);
	if (!printer.is_open()) {
		cerr << "语法分析过程中，" << filename << "文件打开失败！" << endl;
		return;
	}
	//输出标头
	printer << "  ";
	for (auto iter = grammar_symbol_.begin(); iter != grammar_symbol_.end(); ++iter)
	{
		if (IsNonTerminalSymbol(*iter))
			continue;
		if("," == (*iter))//处理由于在CSV中输出 ","导致的位移问题。
			printer << "," << "，";
		else
			printer << "," << (*iter) ;
	}
	for (auto iter = grammar_symbol_.begin(); iter != grammar_symbol_.end(); ++iter)
	{
		if (!IsNonTerminalSymbol(*iter))
			continue;
		printer << "," << (*iter) ;
	}
	printer << endl;

	for (unsigned int state = 0; state < normal_family_.size(); ++state)
	{
		printer << "state " << state;
		for (auto iter = grammar_symbol_.begin(); iter != grammar_symbol_.end(); ++iter)
		{
			if (IsNonTerminalSymbol(*iter))
				continue;

			//map 不存在的情况
			if (action_goto_tables_.find({ state, *iter }) == action_goto_tables_.end())
				printer << ",error";
			else {
				int next_state = action_goto_tables_[{state, *iter}].state;
				int op = action_goto_tables_[{state, (*iter)}].op;
				/*cout << "  ";*/
				if (op == MOVE)
					printer << ",s" << next_state;
				else if (op == CONCLUDE)
				{
					printer << ",r" << next_state;
				}
				else if (op == ACCEPT)
					printer << ",acc";
				else
					printer << ",???";
				//cout << "  ";
			}
		}
		for (auto iter = grammar_symbol_.begin(); iter != grammar_symbol_.end(); ++iter)
		{
			if (!IsNonTerminalSymbol(*iter))
				continue;
			/*cout << "  " << iter->left << "  ";*/
			//map 不存在的情况
			if (action_goto_tables_.find({ state, *iter }) == action_goto_tables_.end())
				printer << ",error";
			else {
				int next_state = action_goto_tables_[{state, *iter}].state;
				int op = action_goto_tables_[{state, *iter}].op;
				//cout << "  ";
				if (op == MOVE)
					printer << ",s" << next_state;
				else if (op == CONCLUDE)
				{
					printer << ",r" << next_state;
				}
				else if (op == ACCEPT)
					printer << ",acc";
				else
					printer << ",???";
				//cout << "  ";
			}
		}
		
		printer << endl;
	}
	return;
}

void SyntacticAnalyzer::PrintBuildGrammarDetails()
{
	if (print_detail_)
	{
		PrintProdcutions();
		PrintFirst();
		PrintFollow();
		PrintGrammarSymbolSet();
		PrintLrItems();
		PrintNormalFamiliySet();
		PrintActionGotoTable();
	}
	return;
}

void SyntacticAnalyzer::PrintAnalysisProcess(int step, const SlrOperation &sl_op)
{
	if (!print_detail_)
		return;
	syntactic_analyzer_printer_ << step << ',';
	//输出状态栈
	for (auto iter = state_sequence_stack_.begin(); iter != state_sequence_stack_.end(); ++iter)
	{
		syntactic_analyzer_printer_ << *iter << ' ';
	}
	syntactic_analyzer_printer_ << ',';

	//输出字符串栈
	for (auto iter = move_conclude_string_stack_.begin(); iter != move_conclude_string_stack_.end(); ++iter)
	{
		if ("," == *iter)
			syntactic_analyzer_printer_ << "，";
		else
			syntactic_analyzer_printer_ << *iter << ' ';
	}
	syntactic_analyzer_printer_ << ',';


	//输出操作
	//---------------------------------------------------------
	if(sl_op.op ==MOVE)
		syntactic_analyzer_printer_ << "移进";
	else if(sl_op.op == ACCEPT)
		syntactic_analyzer_printer_ << "接受";
	else if (sl_op.op == CONCLUDE)
	{
		syntactic_analyzer_printer_ << "规约： " << productions_[sl_op.state].left << "->";
		for (auto iter = productions_[sl_op.state].right.begin(); iter != productions_[sl_op.state].right.end(); ++iter)
		{
			if ("," == *iter)
				syntactic_analyzer_printer_ << "，";
			else
				syntactic_analyzer_printer_ << *iter << ' ';
		}
	}

	syntactic_analyzer_printer_ << endl;
}

bool SyntacticAnalyzer::IsNonTerminalSymbol(const string &symbol)
{
	if (symbol.length() == 0)
		return false;
	if (symbol[0] >= 'A' && symbol[0] <= 'Z')
		return true;
	return false;
}


SyntacticAnalyzer::SyntacticAnalyzer(bool show_detail)
{
	print_detail_ = show_detail;//选择是否在分析过程中打印语法分析的详细信息
	if (show_detail)
	{
		syntactic_analyzer_printer_.open("./gen_data/syntactic_analyzer/syntactic_analyser_process.csv");
		if (!syntactic_analyzer_printer_.is_open()) {
			cerr << "语法分析过程中，显示语法分析过程文件打开失败！" << endl;
		}
		else
			syntactic_analyzer_printer_ << "步骤, 状态栈, 符号栈, 动作说明" << endl;
	}
	BuildGrammar();//根据grammar.txt建立SLR分析表 ACTION 和  GOTO
	PrintBuildGrammarDetails();//打印构建SLR分析表的详细过程以及结果。（会自动根据print_detail_来判断是否打印）

	//调用词法分析器
	//LexicalAnalyzer lexical_analyzer;
	//lexical_analyzer.IsReadyToAnalyze(true);
}

SyntacticAnalyzer::~SyntacticAnalyzer()
{
	if (syntactic_analyzer_printer_.is_open())
		syntactic_analyzer_printer_.close();
}


bool LrItem::operator==(const LrItem & item) const 
{
	return (this->production_number == item.production_number) && (this->point_pos == item.point_pos);
}

bool LrItem::operator<(const LrItem & item) const
{
	return this->production_number < item.production_number || this->production_number == item.production_number&&this->point_pos < item.point_pos;
}

bool SlrOperation::operator==(const SlrOperation & operation) const
{
	return (this->op == operation.op) && (this->state == operation.state);
}


//==================================================================================================================
//构造SLR分析器
bool SyntacticAnalyzer::StartAnalize(const string code_filename)
{
	//栈的初始化
	state_sequence_stack_.push_back(0);//初始状态0压入栈中
	move_conclude_string_stack_.push_back("#");//初始的#字符串压入栈中


	//struct GrammarSymbolInfo//语义分析时候用到的文法符号属性
	//{
	//	string symbol_name;//对应与grammar中的名称
	//	string txt_value;//对应的文本属性值
	//	int num_value;//对应的数值属性值
	//};
	grammar_symbol_info_stack_.push_back({"Program"});//初始的文法符号属性

	//初始化词法分析器
	LexicalAnalyzer lexcial_analyzer;
	if (!lexcial_analyzer.IsReadyToAnalyze(true, code_filename))
		return false;

	int sytactic_step = 0;
	while (true)
	{
		//enum LEXICAL_TYPE {
		//	LCINT, LKEYWORD, LIDENTIFIER, LTYPE, LBORDER, LUNKNOWN, LEOF, LOPERATOR,

		//LEXICAL_TYPE type;//词类型
		//string value;//常量的值，或者关键字本身
		
		WordInfo get_word = lexcial_analyzer.GetWord();
		string word_string = get_word.word_string;
		if (get_word.type == LUNKNOWN)//错误处理
		{
			cerr << "词法分析器过程中，发生unknown错误！" << endl;
			cerr << get_word.value << endl;
		}



		//enum SLR_OPERATIONS { CONCLUDE, MOVE, ACCEPT, NONTERMINAL, ERROR };//规约、移进、接受、压入非终结符、出错
		//struct SlrOperation//ACTION或者GOTO操作
		//{
		//	SLR_OPERATIONS op;//规约、移进、接受
		//	int state;//item中的序号，从0开始
		//	bool operator==(const SlrOperation &operation) const;//判断两个操作是否相同
		//};

		//map<pair<int, string>, SlrOperation> action_goto_tables_;//action 与 goto 表的融合

		//vector<string> move_conclude_string_stack_;//存放 移进规约串 的栈
		//vector<int> state_sequence_stack_;//状态序列栈

		while (true)
		{
			//选择状态序列栈栈顶
			int current_state = state_sequence_stack_[state_sequence_stack_.size() - 1];
			//action_goto表中不存在对应的操作，语法分析过程中出现错误，报告错误并返回
			if (action_goto_tables_.find({ current_state, word_string }) == action_goto_tables_.end())
			{
				cerr << "语法分析器过程中，发生错误！" << endl;
				cerr << get_word.value << endl;
				cerr << "state：" << current_state << " 与 " << word_string << " 在action_goto_table 中不含对应操作!" << endl;
				return false;
			}

			if (MOVE == action_goto_tables_[{ current_state, word_string }].op)//移进操作
			{
				state_sequence_stack_.push_back(action_goto_tables_[{ current_state, word_string }].state);
				move_conclude_string_stack_.push_back(word_string);
				PrintAnalysisProcess(sytactic_step, action_goto_tables_[{ current_state, word_string }]);
				sytactic_step++;
				grammar_symbol_info_stack_.push_back({ get_word.word_string, get_word.value });//文法符号信息压入

				break;
			}
			else if (CONCLUDE == action_goto_tables_[{ current_state, word_string }].op)//规约操作
			{
				//获取规约产生式的序号
				int conclude_production_number = action_goto_tables_[{ current_state, word_string }].state;
				int production_length; //产生式右部符号数目
				if (productions_[conclude_production_number].right[0] == "$")
					production_length = 0;
				else
					production_length = productions_[conclude_production_number].right.size();


				for (int i = 0; i < production_length; ++i)//将两个栈都弹出production_length个元素
				{
					//state_sequence_stack_.pop();
					//move_conclude_string_stack_.pop();
					state_sequence_stack_.erase(state_sequence_stack_.end() - 1);
					move_conclude_string_stack_.erase(move_conclude_string_stack_.end() - 1);
				}

				move_conclude_string_stack_.push_back(productions_[conclude_production_number].left);//用于规约的产生式的左部 压入栈中
				if (action_goto_tables_.find({ state_sequence_stack_[state_sequence_stack_.size() - 1], productions_[conclude_production_number].left }) == action_goto_tables_.end())//不存在goto
				{
					cerr << "语法分析器算法发生致命错误CONCLUDE中" << endl;
					cerr << get_word.value << endl;
					return false;
				}
				state_sequence_stack_.push_back(action_goto_tables_[{ state_sequence_stack_[state_sequence_stack_.size() - 1] , productions_[conclude_production_number].left }].state);//goto对应的状态压入
				
				//语义分析错误，则退出
				if (!sementic_analyzer_.ExecuteSemanticCheck(grammar_symbol_info_stack_, productions_[conclude_production_number]))
					return false;
				

			}
			else if (ACCEPT == action_goto_tables_[{ current_state, word_string }].op)//宣布接受
			{
				PrintAnalysisProcess(sytactic_step, action_goto_tables_[{ current_state, word_string }]);
				sytactic_step++;
				cerr << "语法分析正确完成！" << endl;
				cerr << "语义分析正确完成！" << endl;
				return true;
			}
			else//语法分析器算法错误
			{
				cerr << "致命错误！" << endl;
				cerr << "语法分析器算法存在错误，请检查！" << endl;
				return false;
			}
			PrintAnalysisProcess(sytactic_step, action_goto_tables_[{ current_state, word_string }]);
			sytactic_step++;
		}
		
	}

	return true;
}