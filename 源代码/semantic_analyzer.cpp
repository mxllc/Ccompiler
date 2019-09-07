#include"semantic_analyzer.h"

SemanticAnalyzer::SemanticAnalyzer()
{
	//创建全局符号表，并添加到当前符号表栈中
	symbol_tables_.push_back(SymbolTable(GLOBAL_TABLE, "global_table"));
	current_symbol_table_stack_.push_back(0);

	//创建临时变量符号表
	symbol_tables_.push_back(SymbolTable(TEMP_TABLE, "temp_table"));
	

	intermediate_code_temp_printer_.open("./gen_data/target_file/intermediate_code_temp.txt");
	if (!intermediate_code_temp_printer_.is_open()) {
		cerr << "用来保存输出中间代码的临时文件打开失败！" << endl;
	}

	next_state_num_ = 1;//从1开始，0留给跳转到main的指令
	backpatching_level_ = 0;//从0开始
	main_line_ = -1;
	target_code_generator_.TargetCodeGeneratorInit(symbol_tables_);//将参数表传到目标代码生成器中，实现共享

	while_cnt_ = 0;
	if_cnt_ = 0;
	w_j_label_num_ = 0;
}
SemanticAnalyzer::~SemanticAnalyzer()
{
	if (intermediate_code_temp_printer_.is_open())
		intermediate_code_temp_printer_.close();

	if (intermediate_code_printer_.is_open())
		intermediate_code_printer_.close();

	if (intermediate_code_temp_reader_.is_open())
		intermediate_code_temp_reader_.close();	

	remove("./gen_data/target_file/intermediate_code_temp.txt");
}

int SemanticAnalyzer::GetNextStateNum()
{
	return next_state_num_++;
}
int SemanticAnalyzer::PeekNextStateNum()
{
	return next_state_num_;
}

string SemanticAnalyzer::GetArgName(SymbolPos &sp, bool is_return)
{
	string arg_name = symbol_tables_[sp.table_pos].GetSymbolName(sp.symbol_pos);
	if (is_return == false)
	{
		if (VARIABLE == symbol_tables_[sp.table_pos].GetSymbolMode(sp.symbol_pos))//变量
		{
			//arg_name = arg_name + "_" + std::to_string(sp.table_pos);
			arg_name = arg_name + "-" + symbol_tables_[sp.table_pos].GetTableName()+"Var";
		}
	}
	else
	{
		int pos = symbol_tables_[0].FindSymbol(symbol_tables_[sp.table_pos].GetTableName());
		Symbol &symbol = symbol_tables_[0].GetSymbol(pos);
		int symbol_pos_minus = sp.symbol_pos - 1;
		arg_name = arg_name + "-" + symbol.name + "_paramerter " + std::to_string(sp.symbol_pos);
		//if (VARIABLE == symbol_tables_[sp.table_pos].GetSymbolMode(sp.symbol_pos))//变量
		//{
		//	int pos = symbol_tables_[0].FindSymbol(symbol_tables_[sp.table_pos].GetTableName());
		//	if (pos != -1 && symbol_tables_[0].GetSymbol(pos).mode == FUNCTION)
		//	{
		//		Symbol &symbol = symbol_tables_[0].GetSymbol(pos);
		//		int symbol_pos_minus = sp.symbol_pos - 1;
		//		if (symbol_pos_minus <= symbol.parameter_num)
		//			arg_name = arg_name + "-" + symbol.name + "_paramerter " + std::to_string(sp.symbol_pos);
		//		else
		//			arg_name = arg_name + "_" + std::to_string(sp.table_pos);
		//	}
		//	else
		//		arg_name = arg_name + "_" + std::to_string(sp.table_pos);
		//}
	}
	return arg_name;
}

bool SemanticAnalyzer::PrintQuadruples(const Quadruples &quadruples)
{
	//intermediate_code_temp_printer_ << quadruples.num << "(" << quadruples.op << ","
	//	<< quadruples.arg1 << "," << quadruples.arg2 << "," << quadruples.result << ")" << endl;
	if(backpatching_level_ == 0)//不需要回填
		intermediate_code_temp_printer_ << quadruples.num << " (" << quadruples.op << ", "
		<< quadruples.arg1 << ", " << quadruples.arg2 << ", " << quadruples.result << ")" << endl;
	else//需要回填	
		quadruples_stack_.push_back(quadruples);
	
	return true;
}

bool SemanticAnalyzer::PrintQuadruples()
{
	intermediate_code_printer_.open("./gen_data/target_file/intermediate_code.txt");
	if (!intermediate_code_printer_.is_open()) {
		cerr << "用来保存输出中间代码的文件打开失败！" << endl;
		return false;
	}

	intermediate_code_temp_reader_.open("./gen_data/target_file/intermediate_code_temp.txt");
	if (!intermediate_code_temp_reader_.is_open()) {
		cerr << "读取用来保存输出中间代码的临时文件打开失败！" << endl;
		return false;
	}

	//关闭临时文件。
	if (intermediate_code_temp_printer_.is_open())
		intermediate_code_temp_printer_.close();
	////临时文件回到头部
	//intermediate_code_temp_printer_.clear();
	//intermediate_code_temp_printer_.seekg(0);
	
	//添加第0调指令，直接跳转到Main函数所在的位置
	intermediate_code_printer_ << 0 << " (j, " << "-, " << "-, " << main_line_ << ")" << endl;

	while (!intermediate_code_temp_reader_.eof())
	{
		string str;
		std::getline(intermediate_code_temp_reader_, str);
		if (str == "")
			continue;
		intermediate_code_printer_ << str << endl;
	}

	if (intermediate_code_temp_reader_.is_open())
		intermediate_code_temp_reader_.close();

	//删除临时文件
	if (remove("./gen_data/target_file/intermediate_code_temp.txt") != 0)
		cerr << "中间代码临时文件删除失败！" << endl;

	return true;
}

bool SemanticAnalyzer::CreateSymbolTable(TABLE_TYPE table_type, const string &table_name)
{
	symbol_tables_.push_back(SymbolTable(table_type, table_name));
	current_symbol_table_stack_.push_back(symbol_tables_.size() - 1);
	return true;
}

bool SemanticAnalyzer::ExecuteSemanticCheck(vector<GrammarSymbolInfo>&symbol_info_stack, const Production &production)
{
	if ("Const_value" == production.left)//Const_value->num
	{	
		GrammarSymbolInfo symbol_info = symbol_info_stack.back();
		
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = symbol_info.txt_value;
		

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Factor" == production.left && production.right[0] == "Const_value")//Factor->Const_value
	{
		//需要把Const_value加入临时变量表，得到临时变量的名称已经在表中的位置。
		//txt_value 表示名称
		//pos 表示位置
		//需要 中间代码操作---------------------------------------------------------------------------------------------

		//符号属性
		GrammarSymbolInfo symbol_info = symbol_info_stack.back();

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;

		int symbol_pos = symbol_tables_[1].AddSymbol(symbol_info.txt_value);
		string symbol_name = symbol_tables_[1].GetSymbolName(symbol_pos);

		conclude_syble_info.txt_value = symbol_name;
		conclude_syble_info.pos = { 1, symbol_pos };//1表示临时变量表

		
		PrintQuadruples(Quadruples(GetNextStateNum(), ":=", symbol_info.txt_value, "-", symbol_name));

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成
		target_code_generator_.LoadImmToReg(symbol_info.txt_value, conclude_syble_info.pos);
	}
	
	else if("Factor" == production.left && production.right[0] == "(")//Factor->(Expression)
	{
		//符号属性
		GrammarSymbolInfo symbol_expression = symbol_info_stack[symbol_info_stack.size() - 2];

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = symbol_tables_[symbol_expression.pos.table_pos].GetSymbolName(symbol_expression.pos.symbol_pos);
		conclude_syble_info.pos = symbol_expression.pos;

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Factor" == production.left && production.right[0] == "identifier")//Factor->identifier FTYPE
	{
		//符号属性
		GrammarSymbolInfo &symbol_identifier = symbol_info_stack[symbol_info_stack.size() - 2];
		GrammarSymbolInfo symbol_ftype = symbol_info_stack.back();

		
		//在符号表栈中一级级逆序寻找 identifier是否被定义过

		int identifier_pos = -1;
		int identifier_layer = current_symbol_table_stack_.size() - 1;
		for (; identifier_layer >= 0; --identifier_layer)
		{
			SymbolTable search_table = symbol_tables_[current_symbol_table_stack_[identifier_layer]];
			identifier_pos = search_table.FindSymbol(symbol_identifier.txt_value);
			if (identifier_pos != -1)//表示在某一级中找到了
				break;
		}


		if (-1 == identifier_pos)//没找到，表示不存在，说明没定义就使用，语义错误
		{
			cerr << "语义错误！！！" << symbol_identifier.txt_value << "没有定义！" << endl;
			return false;
		}

		//给Indentifier赋值pos
		symbol_identifier.pos.table_pos = current_symbol_table_stack_[identifier_layer];
		symbol_identifier.pos.symbol_pos = identifier_pos;

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		if (symbol_ftype.txt_value == "")//表示FTYPE是从FTYPE->$获取，即这里Factor是从标识符获取，而非函数。
		{
			//Factor->identifier，而不是函数调用
			//如果不是临时变量的话，需要申请一个临时变量用来存放。
			if (TEMP == symbol_tables_[symbol_identifier.pos.table_pos].GetSymbolMode(symbol_identifier.pos.symbol_pos))
			{
				//原本就是临时变量，无需再申请，可直接传递使用
				conclude_syble_info.pos = { symbol_identifier.pos.table_pos, symbol_identifier.pos.symbol_pos };
				conclude_syble_info.txt_value = symbol_identifier.txt_value;
			}
			else
			{
				//申请临时变量
				int symbol_pos = symbol_tables_[1].AddSymbol(symbol_identifier.txt_value);
				string symbol_name = symbol_tables_[1].GetSymbolName(symbol_pos);


				//用临时变量来存储当前变量的值
				PrintQuadruples(Quadruples(GetNextStateNum(), ":=", GetArgName(symbol_identifier.pos), "-", symbol_name));

				conclude_syble_info.pos = { 1, symbol_pos };//1表示临时变量表

				//目标代码生成
				string tar_arg1_name = target_code_generator_.SetArgBeReady(conclude_syble_info.pos);//新申请的临时变量
				SymbolPos sb_tmp = { symbol_identifier.pos.table_pos, symbol_identifier.pos.symbol_pos };
				string tar_arg2_name = target_code_generator_.SetArgBeReady(sb_tmp);

				target_code_generator_.PrintQuadruples(Instructions("move", tar_arg1_name, tar_arg2_name, ""));
			}

			//conclude_syble_info.txt_value = symbol_tables_[current_symbol_table_stack_[identifier_layer]].GetSymbolName(identifier_pos);		
			//conclude_syble_info.pos = { current_symbol_table_stack_[identifier_layer], identifier_pos };

		}
		else {
			//Factor->identifier FTYPE  是从函数调用中获取的
			//向上传递pos即可
			int table_pos, symbol_pos = 0;
			table_pos = symbol_tables_[symbol_ftype.pos.table_pos].GetSymbol(symbol_ftype.pos.symbol_pos).func_symbol_pos;
			//conclude_syble_info.pos = { table_pos, symbol_pos };
			conclude_syble_info.txt_value = "";

			//申请临时变量
			int symbol_pos_t = symbol_tables_[1].AddSymbol(symbol_identifier.txt_value);
			string symbol_name = symbol_tables_[1].GetSymbolName(symbol_pos_t);

			//用临时变量来存储当前变量的值
			int func_table_pos_t = symbol_tables_[0].GetSymbol(symbol_identifier.pos.symbol_pos).func_symbol_pos;
			SymbolPos return_symbol_pos = { func_table_pos_t , 0 };
			PrintQuadruples(Quadruples(GetNextStateNum(), ":=", GetArgName(return_symbol_pos), "-", symbol_name));
			conclude_syble_info.pos = { 1, symbol_pos_t };//1表示临时变量表


			//目标代码生成
			target_code_generator_.ClearRegs();//调用函数前先清空寄存器
			string target_func_name = symbol_tables_[0].GetSymbolName(identifier_pos);
			target_code_generator_.PrintQuadruples("jal " + target_func_name);//生成函数调用指令
			//清理栈帧
			target_code_generator_.PrintQuadruples(Instructions("move", "$sp", "$fp", ""));//sp指到fp
			target_code_generator_.PrintQuadruples(Instructions("lw", "$fp", "($fp)", ""));//fp指回老fp
			int par_num = symbol_tables_[0].GetSymbol(symbol_identifier.pos.symbol_pos).parameter_num;//函数参数个数
			int sp_back_num = -1 - par_num; //sp指针回退到函数调用前的栈顶。 1返回地址，以及实际参数的个数
			target_code_generator_.PushStack_sp(sp_back_num);//sp回退

			//返回值修改 v0 ->tmp
			string tar_arg1_name = target_code_generator_.SetArgBeReady(conclude_syble_info.pos);//新申请的临时变量
			//SymbolPos sb_tmp = { symbol_identifier.pos.table_pos, symbol_identifier.pos.symbol_pos };
			string tar_arg2_name = target_code_generator_.SetArgBeReady(conclude_syble_info.pos);
			target_code_generator_.PrintQuadruples(Instructions("move", tar_arg1_name, "$v0", ""));
		}

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Factor_loop" == production.left && production.right[0] == "$")//Factor_loop->$
	{
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";//空，这里必须为空
		//symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Factor_loop" == production.left && production.right[0] == "Factor_loop")
		//Factor_loop->Factor_loop Factor *|Factor_loop Factor /
		//这里需要创建临时变量
	{
		//符号属性
		GrammarSymbolInfo symbol_factor = symbol_info_stack[symbol_info_stack.size() - 2];
		GrammarSymbolInfo symbol_factor_loop = symbol_info_stack[symbol_info_stack.size() - 3];


		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;

		if (symbol_factor_loop.txt_value == "")//右部的Factor_loop是从$规约而来
		{
			//申请临时变量，用来存放结果，从左往右传递

			conclude_syble_info.txt_value = symbol_info_stack.back().symbol_name; //  * 或者 /

			if (TEMP == symbol_tables_[symbol_factor.pos.table_pos].GetSymbolMode(symbol_factor.pos.symbol_pos))
			{
				//原本就是临时变量，无需再申请，可直接传递使用
				conclude_syble_info.pos = symbol_factor.pos;
			}
			else
			{
				//申请临时变量
				int symbol_pos = symbol_tables_[1].AddSymbol(symbol_factor.txt_value);
				string symbol_name = symbol_tables_[1].GetSymbolName(symbol_pos);

				//用临时变量来存储当前变量的值
				PrintQuadruples(Quadruples(GetNextStateNum(), ":=", GetArgName(symbol_factor.pos), "-", symbol_name));
				conclude_syble_info.pos = { 1, symbol_pos };//1表示临时变量表

				//目标代码生成
				string tar_arg1_name = target_code_generator_.SetArgBeReady(conclude_syble_info.pos);//新申请的临时变量
				string tar_arg2_name = target_code_generator_.SetArgBeReady(symbol_factor.pos);

				target_code_generator_.PrintQuadruples(Instructions("move", tar_arg1_name, tar_arg2_name, ""));
			}
						
		}
		else//右部的Factor_loop有东西。需要计算，同时将临时变量向上传递
		{
			//这里需要通过中间代码生成
			//把临时变量 op Factor的值赋给临时变量
			
			//生成中间代码：临时变量和factor 运算后返回给临时变量
			//获取两个arg的名称。 
			//不是临时变量的需要加上表所在的序号
			//arg2 类似a_2
			string arg2_name = GetArgName(symbol_factor.pos);
			string arg1_name = GetArgName(symbol_factor_loop.pos);

			string tar_arg2_name = target_code_generator_.SetArgBeReady(symbol_factor.pos);
			string tar_arg1_name = target_code_generator_.SetArgBeReady(symbol_factor_loop.pos);

			conclude_syble_info.txt_value = symbol_factor_loop.txt_value; //  * 或者 /
			conclude_syble_info.pos = symbol_factor_loop.pos;
			
			//目标代码与中间代码共同生成
			if ("*" == symbol_factor_loop.txt_value)
			{
				PrintQuadruples(Quadruples(GetNextStateNum(), "*", arg1_name, arg2_name, arg1_name));
				target_code_generator_.PrintQuadruples(Instructions("mult", tar_arg1_name, tar_arg2_name, ""));
				target_code_generator_.PrintQuadruples(Instructions("mflo", tar_arg1_name, "", ""));//取最低的32位。高32舍去
				
			}
			else// '/' 除法
			{
				PrintQuadruples(Quadruples(GetNextStateNum(), "/", arg1_name, arg2_name, arg1_name));
				target_code_generator_.PrintQuadruples(Instructions("div", tar_arg1_name, tar_arg2_name, ""));
				target_code_generator_.PrintQuadruples(Instructions("mflo", tar_arg1_name, "", ""));//取最低的32位。高32舍去
			}
		}

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Item" == production.left)//Item->Factor_loop Factor
	{
		//符号属性
		GrammarSymbolInfo  symbol_factor_loop = symbol_info_stack[symbol_info_stack.size() - 2];
		GrammarSymbolInfo symbol_factor = symbol_info_stack.back();

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		//conclude_syble_info.txt_value = symbol_factor.txt_value + symbol_factor_loop.txt_value;


		//Factor_loop 为空
		if (symbol_factor_loop.txt_value == "")
		{
			conclude_syble_info.txt_value = "";
			conclude_syble_info.pos = symbol_factor.pos;


		}
		else//不空
		{
			//Factor_loop 和 Factor 操作一下，然后返回Factor_loop 

			string arg2_name = GetArgName(symbol_factor.pos);
			string arg1_name = GetArgName(symbol_factor_loop.pos);

			string tar_arg2_name = target_code_generator_.SetArgBeReady(symbol_factor.pos);
			string tar_arg1_name = target_code_generator_.SetArgBeReady(symbol_factor_loop.pos);


			if ("*" == symbol_factor_loop.txt_value)
			{
				PrintQuadruples(Quadruples(GetNextStateNum(), "*", arg1_name, arg2_name, arg1_name));
				target_code_generator_.PrintQuadruples(Instructions("mult", tar_arg1_name, tar_arg2_name, ""));
				target_code_generator_.PrintQuadruples(Instructions("mflo", tar_arg1_name, "", ""));//取最低的32位。高32舍去
			}
			else// '/' 除法
			{
				PrintQuadruples(Quadruples(GetNextStateNum(), "/", arg1_name, arg2_name, arg1_name));
				target_code_generator_.PrintQuadruples(Instructions("div", tar_arg1_name, tar_arg2_name, ""));
				target_code_generator_.PrintQuadruples(Instructions("mflo", tar_arg1_name, "", ""));//取最低的32位。高32舍去
			}

			//conclude_syble_info.txt_value  == * 或者 /
			conclude_syble_info.txt_value = symbol_factor_loop.txt_value;
			conclude_syble_info.pos = symbol_factor_loop.pos;
		}


		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Item_loop" == production.left && production.right[0] == "$")//Item_loop->$
	{
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";//空，这里必须为空
		//symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Item_loop" == production.left && production.right[0] == "Item_loop")
	//Item_loop->Item_loop Item +|Item_loop Item -
	//这里需要创建临时变量
	{
		//符号属性
		GrammarSymbolInfo symbol_item = symbol_info_stack[symbol_info_stack.size() - 2];
		GrammarSymbolInfo symbol_item_loop = symbol_info_stack[symbol_info_stack.size() - 3];


		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;

		if (symbol_item_loop.txt_value == "")//右部的Item_loop是从$规约而来
		{
			//这里没有必要申请临时变量，直接把值从Item传递到Item_loop
			conclude_syble_info.txt_value = symbol_info_stack.back().symbol_name; //  + 或者 -
			conclude_syble_info.pos = symbol_item.pos;

			////申请临时变量，用来存放结果，从左往右传递
			//conclude_syble_info.txt_value = symbol_info_stack.back().symbol_name; //  + 或者 -

			////申请临时变量
			//int symbol_pos = symbol_tables_[1].AddSymbol(symbol_item.txt_value);
			//string symbol_name = symbol_tables_[1].GetSymbolName(symbol_pos);
			//
			////这里产生中间代码--------
			////用临时变量来存储当前变量的值
			//PrintQuadruples(Quadruples(GetNextStateNum(), ":=", GetArgName(symbol_item.pos), "-", symbol_name));

			//conclude_syble_info.pos = { 1, symbol_pos };//1表示临时变量表
		}
		else//右部的Item_loop有东西。需要计算，同时产生临时变量
		{
			//这里需要通过中间代码生成
			//把临时变量 op Item的值赋给临时变量

			//生成中间代码：临时变量和Item 运算后返回给临时变量
			//获取两个arg的名称。 
			//不是临时变量的需要加上表所在的序号
			//arg2 类似a_2
			string arg2_name = GetArgName(symbol_item.pos);
			string arg1_name = GetArgName(symbol_item_loop.pos);

			string tar_arg2_name = target_code_generator_.SetArgBeReady(symbol_item.pos);
			string tar_arg1_name = target_code_generator_.SetArgBeReady(symbol_item_loop.pos);

			//目标代码与中间代码共同生成
			if ("+" == symbol_item_loop.txt_value)
			{
				PrintQuadruples(Quadruples(GetNextStateNum(), "+", arg1_name, arg2_name, arg1_name));
				target_code_generator_.PrintQuadruples(Instructions("add", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			}
			else// '-' 减法
			{
				PrintQuadruples(Quadruples(GetNextStateNum(), "-", arg1_name, arg2_name, arg1_name));
				target_code_generator_.PrintQuadruples(Instructions("sub", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			}

			conclude_syble_info.txt_value = symbol_info_stack.back().symbol_name; //  + 或者 -
			conclude_syble_info.pos = symbol_item_loop.pos;

		}

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}

	else if ("Add_expression" == production.left)//Add_expression->Item_loop Item
	{
		//符号属性
		GrammarSymbolInfo symbol_item_loop = symbol_info_stack[symbol_info_stack.size() - 2];
		GrammarSymbolInfo symbol_item = symbol_info_stack.back();

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;


		//Item_loop 为空
		if (symbol_item_loop.txt_value == "")
		{
			conclude_syble_info.txt_value = "";
			conclude_syble_info.pos = symbol_item.pos;
		}
		else//不空
		{
			//Item_loop 和 Item 操作一下，然后返回Item_loop 
			string arg2_name = GetArgName(symbol_item.pos);
			string arg1_name = GetArgName(symbol_item_loop.pos);

			string tar_arg2_name = target_code_generator_.SetArgBeReady(symbol_item.pos);
			string tar_arg1_name = target_code_generator_.SetArgBeReady(symbol_item_loop.pos);

			//目标代码与中间代码共同生成
			if ("+" == symbol_item_loop.txt_value)
			{
				PrintQuadruples(Quadruples(GetNextStateNum(), "+", arg1_name, arg2_name, arg1_name));
				target_code_generator_.PrintQuadruples(Instructions("add", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			}
			else// '-' 减法
			{
				PrintQuadruples(Quadruples(GetNextStateNum(), "-", arg1_name, arg2_name, arg1_name));
				target_code_generator_.PrintQuadruples(Instructions("sub", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			}

			//conclude_syble_info.txt_value  == + 或者 -
			conclude_syble_info.txt_value = symbol_item_loop.txt_value;
			conclude_syble_info.pos = symbol_item_loop.pos;
		}


		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Add_expression_loop" == production.left && production.right[0] == "$")//Add_expression_loop->$
	{
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";//空，这里必须为空
		//symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Add_expression_loop" == production.left && production.right[0] ==  "Add_expression_loop")
	//Add_expression_loop->Add_expression_loop Add_expression Relop
	//这里需要创建临时变量
	{
		//符号属性
		GrammarSymbolInfo symbol_add_expression = symbol_info_stack[symbol_info_stack.size() - 2];
		GrammarSymbolInfo symbol_add_expression_loop = symbol_info_stack[symbol_info_stack.size() - 3];


		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;

		if (symbol_add_expression_loop.txt_value == "")//右部的Factor_loop是从$规约而来
		{
			//这里没有必要申请临时变量，直接把值从Add_expression传递到Add_expression_loop
			conclude_syble_info.txt_value = symbol_info_stack.back().txt_value; //  <|<=|>|>=|==|!=
			conclude_syble_info.pos = symbol_add_expression.pos;
		}
		else//右部的Add_expression_loop有东西。需要计算，同时将临时变量向上传递
		{
			string arg2_name = GetArgName(symbol_add_expression.pos);
			string arg1_name = GetArgName(symbol_add_expression_loop.pos);
			string op_name = "j" + symbol_add_expression_loop.txt_value; //  <|<=|>|>=|==|!=;

			int next_state_num = GetNextStateNum();	
			PrintQuadruples(Quadruples(next_state_num, op_name, arg1_name, arg2_name, std::to_string(next_state_num + 3)));
			PrintQuadruples(Quadruples(GetNextStateNum(), ":=", "0", "-", arg1_name));
			PrintQuadruples(Quadruples(GetNextStateNum(), "j", "-", "-", std::to_string(next_state_num + 4)));
			PrintQuadruples(Quadruples(GetNextStateNum(), ":=", "1", "-", arg1_name));

			string tar_arg2_name = target_code_generator_.SetArgBeReady(symbol_add_expression.pos);
			string tar_arg1_name = target_code_generator_.SetArgBeReady(symbol_add_expression_loop.pos);
			string op = symbol_add_expression_loop.txt_value;
			if ("<" == op)
				target_code_generator_.PrintQuadruples(Instructions("slt", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else if ("<=" == op)
				target_code_generator_.PrintQuadruples(Instructions("sle", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else if (">" == op)
				target_code_generator_.PrintQuadruples(Instructions("sgt", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else if (">=" == op)
				target_code_generator_.PrintQuadruples(Instructions("sge", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else if ("==" == op)
				target_code_generator_.PrintQuadruples(Instructions("seq", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else if ("!=" == op)
				target_code_generator_.PrintQuadruples(Instructions("snq", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else
			{
				cerr << "语义分析错误！" << "Add_expression_loop" << endl;
				return false;
			}
			//目标代码生成
			//  <  | <=  |  >  | >= | ==  | !=
			// slt | sle | sgt | sge| seq | snq

			conclude_syble_info.txt_value = symbol_add_expression_loop.txt_value; //  <|<=|>|>=|==|!=
			conclude_syble_info.pos = symbol_add_expression_loop.pos;

		}

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	

	else if ("Relop" == production.left)//Relop-><|<=|>|>= |= = |!=
	{
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = symbol_info_stack.back().txt_value;//<|<=|>|>= |= = |!=

		// 获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}

	else if ("Expression" == production.left)//Expression->Add_expression_loop Add_expression
	{
		//符号属性
		GrammarSymbolInfo symbol_add_expression_loop = symbol_info_stack[symbol_info_stack.size() - 2];
		GrammarSymbolInfo symbol_add_expression = symbol_info_stack.back();

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;


		//Add_expression_loop 为空
		if (symbol_add_expression_loop.txt_value == "")
		{
			conclude_syble_info.txt_value = symbol_add_expression.txt_value;
			conclude_syble_info.pos = symbol_add_expression.pos;
		}
		else//不空
		{
			//Add_expression_loop 和 Add_expression 操作一下，然后返回Add_expression_loop -----------------------中间代码

			string arg2_name = GetArgName(symbol_add_expression.pos);
			string arg1_name = GetArgName(symbol_add_expression_loop.pos);
			string op_name = "j" + symbol_add_expression_loop.txt_value; //  <|<=|>|>=|==|!=;

			int next_state_num = GetNextStateNum();
			PrintQuadruples(Quadruples(next_state_num, op_name, arg1_name, arg2_name, std::to_string(next_state_num + 3)));
			PrintQuadruples(Quadruples(GetNextStateNum(), ":=", "0", "-", arg1_name));
			PrintQuadruples(Quadruples(GetNextStateNum(), "j", "-", "-", std::to_string(next_state_num + 4)));
			PrintQuadruples(Quadruples(GetNextStateNum(), ":=", "1", "-", arg1_name));


			string tar_arg2_name = target_code_generator_.SetArgBeReady(symbol_add_expression.pos);
			string tar_arg1_name = target_code_generator_.SetArgBeReady(symbol_add_expression_loop.pos);
			string op = symbol_add_expression_loop.txt_value;
			if ("<" == op)
				target_code_generator_.PrintQuadruples(Instructions("slt", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else if ("<=" == op)
				target_code_generator_.PrintQuadruples(Instructions("sle", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else if (">" == op)
				target_code_generator_.PrintQuadruples(Instructions("sgt", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else if (">=" == op)
				target_code_generator_.PrintQuadruples(Instructions("sge", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else if ("==" == op)
				target_code_generator_.PrintQuadruples(Instructions("seq", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else if ("!=" == op)
				target_code_generator_.PrintQuadruples(Instructions("snq", tar_arg1_name, tar_arg1_name, tar_arg2_name));
			else
			{
				cerr << "语义分析错误！" << "Add_expression_loop" << endl;
				return false;
			}

			//conclude_syble_info.txt_value  ==  <|<=|>|>=|==|!=
			conclude_syble_info.txt_value = symbol_add_expression_loop.txt_value;
			conclude_syble_info.pos = symbol_add_expression_loop.pos;
		}


		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	

	
	else if ("Internal_variable_stmt" == production.left )//Internal_variable_stmt->int identifier
	{
		GrammarSymbolInfo symbol_identifier = symbol_info_stack.back();
		//这里注意是引用
		SymbolTable &current_table = symbol_tables_[current_symbol_table_stack_.back()];

		//先在当前符号表中寻找 identifier是否被定义过
		if (-1 != current_table.FindSymbol(symbol_identifier.txt_value))//找到，表示已存在，说明重复定义，语义错误
		{
			cerr << "语义错误！！！" << symbol_identifier.txt_value << "重复定义！" << endl;
			return false;
		}

		//先在全局符号表中查看这个是与函数定义冲突
		int search_symbol_pos = symbol_tables_[0].FindSymbol(symbol_identifier.txt_value);
		if (-1 != search_symbol_pos && symbol_tables_[0].GetSymbolMode(search_symbol_pos) == FUNCTION)
		{

			cerr << "语义错误！！！" << symbol_identifier.txt_value << "已经被定义为函数！" << endl;
			return false;
		}
		
		//没有被定义过，加入符号表中
		Symbol variable_symbol;
		variable_symbol.mode = VARIABLE;//类型
		variable_symbol.name = symbol_identifier.txt_value;//名称
		variable_symbol.type = INT;
		variable_symbol.value = "";//字面值还没有被赋值，此处为空
		
		int symbol_pos = current_table.AddSymbol(variable_symbol);//加入符号表中

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();
		
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;

		//下面这两句好像没有什么用处
		conclude_syble_info.txt_value = symbol_identifier.txt_value; //变量的名称
		conclude_syble_info.pos = { current_symbol_table_stack_.back(), symbol_pos };//把在符号表的顺序，在符号表中的位置存下来

		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成：定义一个局部变量就push进栈，$sp=$sp+1*4
		target_code_generator_.PushStack_sp(1);
	}
	
	else if ("Assign_sentence" == production.left)//Assign_sentence->identifier = Expression ;
	{
		//获取identifier与Expression的文法符号属性
		GrammarSymbolInfo symbol_expression = symbol_info_stack[symbol_info_stack.size() - 2];
		GrammarSymbolInfo symbol_identifier = symbol_info_stack[symbol_info_stack.size() - 4];
		//SymbolTable current_table = symbol_tables_[current_symbol_table_stack_.back()];

		//在符号表栈中一级级逆序寻找 identifier是否被定义过

		int identifier_pos = -1;
		int identifier_layer = current_symbol_table_stack_.size() - 1;
		for (;identifier_layer >= 0; --identifier_layer)
		{
			SymbolTable search_table = symbol_tables_[current_symbol_table_stack_[identifier_layer]];
			identifier_pos = search_table.FindSymbol(symbol_identifier.txt_value);
			if (identifier_pos != -1)//表示在某一级中找到了
				break;
		}


		if (-1 == identifier_pos)//没找到，表示不存在，说明没定义就使用，语义错误
		{
			cerr << "语义错误！！！" << symbol_identifier.txt_value << "没有定义！" << endl;
			return false;
		}

		//输出赋值的中间代码
		SymbolPos sp;
		sp.table_pos = current_symbol_table_stack_[identifier_layer];
		sp.symbol_pos = identifier_pos;
		string result_name = GetArgName(sp);
		string arg1_name = GetArgName(symbol_expression.pos);
		PrintQuadruples(Quadruples(GetNextStateNum(), ":=", arg1_name, "-", result_name));

		//目标代码生成
		string tar_arg2_name = target_code_generator_.SetArgBeReady(symbol_expression.pos);
		string tar_arg1_name = target_code_generator_.SetArgBeReady(sp);
		target_code_generator_.PrintQuadruples(Instructions("move", tar_arg1_name, tar_arg2_name, ""));

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();

		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";//这个信息没什么用，就是随便加进去，可能以后会用到
		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Create_Function_table" == production.left)//Create_Function_table->$
	{
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";
		
		
		//Stmt->int identifier Stmt_type | void identifier Create_Function_table Function_stmt
		//此时symbol_info_stack的最后一个为identifier，即函数的名称。
		GrammarSymbolInfo symbol_identifier = symbol_info_stack.back();
		
		//先在全局符号表中查看这个函数是否被定义过了
		if (-1 != symbol_tables_[0].FindSymbol(symbol_identifier.txt_value))
		{
			cerr << "语义错误！！！" << symbol_identifier.txt_value << "重复定义！" << production.left << endl;
			return false;
		}

		//CreateSymbolTable(FUNCTION_TABLE, symbol_identifier.txt_value);//创建新的函数符号表，并进入
		SymbolTable new_table(FUNCTION_TABLE, symbol_identifier.txt_value);
		symbol_tables_.push_back(new_table);
		//把函数表看成global_table中的一个符号，并加入
		Symbol new_function_symbol;
		new_function_symbol.mode = FUNCTION;
		new_function_symbol.name = symbol_identifier.txt_value;
		new_function_symbol.type = VOID;
		new_function_symbol.func_symbol_pos = symbol_tables_.size() - 1;
		symbol_tables_[0].AddSymbol(new_function_symbol);

		
		//进入新的符号表中
		current_symbol_table_stack_.push_back(symbol_tables_.size() - 1);

		//把函数返回值作为第0项添加到函数的符号表中
		GrammarSymbolInfo symbol_return_type = symbol_info_stack[symbol_info_stack.size() - 2]; //int 或者 void

		//定义返回值为变量，加入函数符号表的第0项---
		Symbol variable_symbol;
		variable_symbol.mode = RETURN_VAR;//类型
		variable_symbol.name = symbol_tables_[current_symbol_table_stack_.back()].GetTableName() + "-return_value";//名称
		if (symbol_return_type.txt_value=="int")
			variable_symbol.type = INT;
		else if(symbol_return_type.txt_value == "void")
			variable_symbol.type = VOID;
		else {
			cerr << "函数返回类型只能有int 和 void！语义分析错误! " << production.left << endl;
			return false;
		}
		variable_symbol.value = "";//字面值还没有被赋值，此处为空
		
		//查看是否为Main,是的话做好标记等待跳转
		if (symbol_identifier.txt_value == "main")
			main_line_ = PeekNextStateNum();
		//输出函数名称
		PrintQuadruples(Quadruples(GetNextStateNum(), symbol_identifier.txt_value, "-", "-", "-"));

		//将返回值加入符号表 pos =(table_pos, 0)
		symbol_tables_[current_symbol_table_stack_.back()].AddSymbol(variable_symbol);

		//右部为$，所以不用pop
		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成
		//生成函数名的标签
		target_code_generator_.PrintQuadruples(symbol_identifier.txt_value + " :");
		//函数调用，进入新函数后先进行栈帧建设
		target_code_generator_.CreateStackFrame();

		//循环标签的存储
		w_j_label_nums.push_back(w_j_label_num_);
		w_j_label_num_ = 0;
	}

	else if ("Exit_Function_table" == production.left)//->$
	{
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";
		//Stmt->int identifier Stmt_type | void identifier Create_Function_table Function_stmt
		//Function_stmt->(Formal_parameter)Sentence_block Exit_Function_table
		GrammarSymbolInfo symbol_identifier = symbol_info_stack[symbol_info_stack.size() - 6];

		//从当前符号表栈中弹出
		current_symbol_table_stack_.pop_back();

		//右部为$，所以不用pop
		symbol_info_stack.push_back(conclude_syble_info);

		//循环标签的恢复
		w_j_label_num_ = w_j_label_nums.back();
		w_j_label_nums.pop_back();

		target_code_generator_.ClearRegs();//退出前先清空寄存器


		if (symbol_identifier.txt_value != "main")
			target_code_generator_.PrintQuadruples(Instructions("jr", "$ra", "", ""));//输出跳回指令
		else//main函数，需要特殊处理
			target_code_generator_.PrintSyscall();//main的特殊结束
	}

	
	else if ("Variavle_stmt" == production.left)//Variavle_stmt->;
	{
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = ";";
		
		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Stmt_type" == production.left && production.right[0] == "Variavle_stmt")//Stmt_type->Variavle_stmt
	{
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = ";";

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}


	else if ("Stmt" == production.left && production.right[0] == "int")//Stmt->int identifier Stmt_type
	{
		//符号属性
		GrammarSymbolInfo symbol_identifier = symbol_info_stack[symbol_info_stack.size() - 2];
		GrammarSymbolInfo symbol_Stmt_type = symbol_info_stack.back();

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		if (symbol_Stmt_type.txt_value == ";")//表示定义变量，而不是函数
		{
			//在符号表栈中一级级逆序寻找 identifier是否被定义过
			int identifier_pos = -1;
			int identifier_layer = current_symbol_table_stack_.size() - 1;
			for (; identifier_layer >= 0; --identifier_layer)
			{
				SymbolTable search_table = symbol_tables_[current_symbol_table_stack_[identifier_layer]];
				identifier_pos = search_table.FindSymbol(symbol_identifier.txt_value);
				if (identifier_pos != -1)//表示在某一级中找到了
					break;
			}


			if (-1 != identifier_pos)//找到了，表示存在，说明重复定义，语义错误
			{
				cerr << "语义错误！！！" << symbol_identifier.txt_value << "重复定义！" << production.left << endl;
				return false;
			}

			//没有被定义过，加入符号表中
			Symbol variable_symbol;
			variable_symbol.mode = VARIABLE;//类型
			variable_symbol.name = symbol_identifier.txt_value;//名称
			variable_symbol.type = INT;
			variable_symbol.value = "";//字面值还没有被赋值，此处为空

			int symbol_pos = symbol_tables_[current_symbol_table_stack_.back()].AddSymbol(variable_symbol);//加入符号表中
			target_code_generator_.PushStack_sp(1);//定义一个局部变量就push进栈，$sp=$sp+1*4

			conclude_syble_info.txt_value = symbol_identifier.txt_value;

		}
		else {
			;//定义函数
			//无操作。 创建函数表已经存储返回值的操作在Create_Function_table的规约中完成。
			//Stmt_type->Variavle_stmt | Create_Function_table Function_stmt
			//Create_Function_table->$
		}

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	

	
	else if ("Return_expression" == production.left && production.right[0] == "$")//Return_expression->$
	{
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";//空，这里必须为空

		//在这种情况下，返回值为void。 这需要判断当前符号表第0个项目的类型。
		//判断返回类型是否正确
		if (VOID != symbol_tables_[current_symbol_table_stack_.back()].GetSymbolType(0))
		{
			//返回类型存在问题。
			cerr << "语义错误！！！" << symbol_tables_[current_symbol_table_stack_.back()].GetTableName() 
				<< "函数定义的返回值为int类型。 但是return语句返回void类型！" << endl;
			return false;
		}
		
		//symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}

	else if ("Return_expression" == production.left && production.right[0] == "Expression")//Return_expression->Expression
	{

		//在这种情况下，返回值为int。 这需要判断当前符号表第0个项目的类型。
		//判断返回类型是否正确
		if (INT != symbol_tables_[current_symbol_table_stack_.back()].GetSymbolType(0))
		{
			//返回类型存在问题。
			cerr << "语义错误！！！" << symbol_tables_[current_symbol_table_stack_.back()].GetTableName() 
				<< "函数定义的返回值为void类型。 但是return语句返回int类型！" << endl;
			return false;
		}
		
		GrammarSymbolInfo symbol_expression = symbol_info_stack.back();
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "int";//表示有值返回，区别与""
		conclude_syble_info.pos = symbol_expression.pos;

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成
		//string tar_arg2_name = target_code_generator_.SetArgBeReady(symbol_expression.pos);
		string tar_arg_name = target_code_generator_.SetArgBeReady(symbol_expression.pos);
		target_code_generator_.PrintQuadruples(Instructions("move", "$v0", tar_arg_name, ""));//把返回值赋给v0
		target_code_generator_.PrintQuadruples(Instructions("lw", "$ra", "-4($fp)", ""));//把返回地址赋给ra
	}

	else if ("Return_sentence" == production.left )//Return_sentence->return Return_expression ;
	{
		GrammarSymbolInfo symbol_return_expression = symbol_info_stack[symbol_info_stack.size() -2];

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;

		SymbolTable &current_function_table = symbol_tables_[current_symbol_table_stack_.back()];//获取当前函数表
		int function_pos = symbol_tables_[0].FindSymbol(current_function_table.GetTableName());//函数在全局符号表的位置
		Symbol &current_function_symbol = symbol_tables_[0].GetSymbol(function_pos);//函数在全局符号表中作为符号

		if ("" != symbol_return_expression.txt_value)
		{
			SymbolPos result_pos;
			result_pos.table_pos = current_symbol_table_stack_.back();
			result_pos.symbol_pos = 0;//返回值的位置在 第0项
			string result_name = GetArgName(result_pos);
			string arg1_name = GetArgName(symbol_return_expression.pos);

			//把返回值写到函数表的第0项
			PrintQuadruples(Quadruples(GetNextStateNum(), ":=", arg1_name, "-", result_name));


			conclude_syble_info.txt_value = "int";
			conclude_syble_info.pos = symbol_return_expression.pos;
		}
		else
			conclude_syble_info.txt_value = "";
		//输出返回指令
		PrintQuadruples(Quadruples(GetNextStateNum(), "return", "-", "-", current_function_table.GetTableName()));


		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}

	else if ("Parameter" == production.left )//Parameter->int identifier
	{
		GrammarSymbolInfo symbol_identifier = symbol_info_stack.back();
		//这里注意是引用
		SymbolTable &current_table = symbol_tables_[current_symbol_table_stack_.back()];

		int table_pos = symbol_tables_[0].FindSymbol(current_table.GetTableName());


		//先在当前符号表中寻找 identifier是否被定义过
		if (-1 != current_table.FindSymbol(symbol_identifier.txt_value))//找到，表示已存在，说明重复定义，语义错误
		{
			cerr << "语义错误！！！" << symbol_identifier.txt_value << "函数参数 重复定义！" << endl;
			return false;
		}

		//先在全局符号表中查看这个是与函数定义冲突
		int search_symbol_pos = symbol_tables_[0].FindSymbol(symbol_identifier.txt_value);
		if (-1 != search_symbol_pos && symbol_tables_[0].GetSymbolMode(search_symbol_pos) == FUNCTION)
		{
			cerr << "语义错误！！！" << symbol_identifier.txt_value << "已经被定义为函数！" << production.left << endl;
			return false;
		}
		
		//没有被定义过，加入符号表中
		Symbol variable_symbol;
		variable_symbol.mode = VARIABLE;//类型
		variable_symbol.name = symbol_identifier.txt_value;//名称
		variable_symbol.type = INT;
		variable_symbol.value = "";//字面值还没有被赋值，此处为空
		
		int symbol_pos = current_table.AddSymbol(variable_symbol);//加入符号表中

		//全局符号表中函数的参数个数需要加一   GetSymbol函数返回的是引用
		symbol_tables_[0].GetSymbol(table_pos).parameter_num++;

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();
		
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;

		//下面这两句好像没有什么用处
		conclude_syble_info.txt_value = symbol_identifier.txt_value; //变量的名称
		conclude_syble_info.pos = { current_symbol_table_stack_.back(), symbol_pos };//把在符号表的顺序，在符号表中的位置存下来

		symbol_info_stack.push_back(conclude_syble_info);
		
	}
	
	else if ("Sentence_block" == production.left)//Sentence_block->Sentence_block_m { Internal_stmt Sentence_string }
	{	
		//返回下一个state的序号。给回填使用
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = std::to_string(PeekNextStateNum());

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);
	}

	//else if ("Sentence_block_m" == production.left)//Sentence_block_m->$
	//{
	//	//主要用来清空寄存器
	//	//由规约后的产生式左部构造一个文法符号属性
	//	GrammarSymbolInfo conclude_syble_info;
	//	conclude_syble_info.symbol_name = production.left;
	//	conclude_syble_info.txt_value = "";
	//
	//
	//	//获取产生式右部的长度
	//	int length = 0;
	//	if ("$" != production.right[0])
	//		length = production.right.size();
	//
	//	for (int i = 0; i < length; ++i)
	//		symbol_info_stack.pop_back();
	//
	//	symbol_info_stack.push_back(conclude_syble_info);
	//
	//	target_code_generator_.ClearRegs();//进入循环前先清空寄存器
	//}
	//

	

	else if ("While_sentence_m2" == production.left)//While_sentence_m2->$
	{	
	//While_sentence->while While_sentence_m1 ( Expression ) While_sentence_m2 Sentence_block
			
		GrammarSymbolInfo symbol_expression = symbol_info_stack[symbol_info_stack.size() -2 ];
		string symbol_name = symbol_tables_[symbol_expression.pos.table_pos].GetSymbolName(symbol_expression.pos.symbol_pos);

		//两个等待回填的真假出口
		PrintQuadruples(Quadruples(GetNextStateNum(), "j=", symbol_name, "0", "---j="));
		backpatching_point_pos_.push_back(quadruples_stack_.size() - 1);

		PrintQuadruples(Quadruples(GetNextStateNum(), "j", "-", "-", "---j"));
		backpatching_point_pos_.push_back(quadruples_stack_.size() - 1);

		//int next_state_num = PeekNextStateNum();
		//backpatching_value.push_back(next_state_num);
		
		//返回下一个state的序号。给回填使用
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = std::to_string(PeekNextStateNum());

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成
		string tar_arg1_name = target_code_generator_.SetArgBeReady(symbol_expression.pos);
		string j_label_end = j_label_.back();
		j_label_.pop_back();
		//条件等于0则跳出循环
		target_code_generator_.PrintQuadruples(Instructions("beq", tar_arg1_name, "$zero", j_label_end));
	
	}


	else if ("While_sentence_m1" == production.left)//While_sentence_m1->$
	{	
		//返回下一个state的序号。给回填使用
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = std::to_string(PeekNextStateNum());

		//同时，回填层级+1
		backpatching_level_++;

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);

		//将两个标签生成后压入栈中
		string table_name = symbol_tables_[current_symbol_table_stack_.back()].GetTableName();
		int cnt = w_j_label_num_++;
		string label_1 = "Label_" + table_name + "_while_begin_" + std::to_string(cnt);
		string label_2 = "Label_" + table_name + "_while_end_" + std::to_string(cnt);
		//先写begin
		w_label_.push_back(label_2);
		w_label_.push_back(label_1);
		//先跳end
		j_label_.push_back(label_1);
		j_label_.push_back(label_2);
		//目标代码生成
		string w_label_begin = w_label_.back();
		w_label_.pop_back();

		target_code_generator_.PrintQuadruples("\n#while循环，先清空寄存器");//空一行
		target_code_generator_.ClearRegs();//进入循环前先清空寄存器
		//生成while的begin标签
		target_code_generator_.PrintQuadruples(w_label_begin+" :");

	}
	
	else if ("While_sentence" == production.left)
	//While_sentence->while While_sentence_m1 ( Expression ) While_sentence_m2 Sentence_block
	{		
		//跳转到while的判断条件，这个四元式加入到回填栈中
		GrammarSymbolInfo symbol_while_sentence_m1 = symbol_info_stack[symbol_info_stack.size() - 6];
		GrammarSymbolInfo symbol_while_sentence_m2 = symbol_info_stack[symbol_info_stack.size() - 2];
		GrammarSymbolInfo symbol_sentence_block = symbol_info_stack.back();
		PrintQuadruples(Quadruples(GetNextStateNum(), "j", "-", "-", symbol_while_sentence_m1.txt_value));

		//开始回填 两个地方
		//真出口
		int batch_pos = backpatching_point_pos_.back();
		backpatching_point_pos_.pop_back();
		quadruples_stack_[batch_pos].SetContent(quadruples_stack_[batch_pos].num, quadruples_stack_[batch_pos].op,
			quadruples_stack_[batch_pos].arg1, quadruples_stack_[batch_pos].arg2, symbol_while_sentence_m2.txt_value);
		//假出口
		batch_pos = backpatching_point_pos_.back();
		backpatching_point_pos_.pop_back();
		quadruples_stack_[batch_pos].SetContent(quadruples_stack_[batch_pos].num, quadruples_stack_[batch_pos].op,
			quadruples_stack_[batch_pos].arg1, quadruples_stack_[batch_pos].arg2, std::to_string(PeekNextStateNum()));
		

		--backpatching_level_;//回填深度减1
		//判断是否需要输出
		if (backpatching_level_ == 0)
		{
			for (auto iter = quadruples_stack_.begin(); iter != quadruples_stack_.end(); ++iter)
				PrintQuadruples(*iter);
			int length = quadruples_stack_.size();
			for (int i = 0; i < length; ++i)//回填栈全部输出，可以清空了
				quadruples_stack_.pop_back();
		}

		/*int next_state_num = PeekNextStateNum();*/

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成
		target_code_generator_.ClearRegs();//退出循环前先清空寄存器
		string j_label_begin = j_label_.back();
		j_label_.pop_back();
		target_code_generator_.PrintQuadruples("j " + j_label_begin);
		string w_label_end = w_label_.back();
		w_label_.pop_back();

		target_code_generator_.PrintQuadruples(w_label_end + " :");
		//清空寄存器 --保证出if是干净的
		target_code_generator_.ClearRegs();
		target_code_generator_.PrintQuadruples("");//空一行
		

	}

	else if ("If_sentence_m0" == production.left)//If_sentence_m0->$
	{	
		//主要用来加深回填深度
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = std::to_string(PeekNextStateNum());

		//同时，回填层级+1
		backpatching_level_++;

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);

		target_code_generator_.PrintQuadruples("\n#if判断");//空一行
		//target_code_generator_.PrintQuadruples("\n#if判断，先清空寄存器");//空一行
		//target_code_generator_.ClearRegs();//进入循环前先清空寄存器
	}

	else if ("If_sentence_n" == production.left)//If_sentence_n->$ 
	{	
		//输出跳转到if-else结束的地方,避免执行else
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		
		//等待回填的跳出出口
		PrintQuadruples(Quadruples(GetNextStateNum(), "j", "-", "-", "---j-if-n"));
		backpatching_point_pos_.push_back(quadruples_stack_.size() - 1);

		conclude_syble_info.txt_value = std::to_string(PeekNextStateNum());//把if-else的假出口地址进行传递

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成
		//只输出j else_end  以及 label if_end
		string w_label_if_end = w_label_.back();
		w_label_.pop_back();
		string j_label_else_end = j_label_.back();
		j_label_.pop_back();

		//清空寄存器 --保证出if是干净的
		target_code_generator_.ClearRegs();
		//注意这里的顺序：先跳转，后输出标签
		//if结束后要跳转到else_end，从而避免执行else
		target_code_generator_.PrintQuadruples(Instructions("j", j_label_else_end, "", ""));
		//生成if_end标签
		target_code_generator_.PrintQuadruples(w_label_if_end + " :");
	}

	else if ("If_sentence_m1" == production.left)//If_sentence_m1->$
	{	
	//If_sentence->if If_sentence_m0 ( Expression ) If_sentence_m1 Sentence_block If_expression
			
		GrammarSymbolInfo symbol_expression = symbol_info_stack[symbol_info_stack.size() -2 ];
		string symbol_name = symbol_tables_[symbol_expression.pos.table_pos].GetSymbolName(symbol_expression.pos.symbol_pos);

		//两个等待回填的真假出口
		PrintQuadruples(Quadruples(GetNextStateNum(), "j=", symbol_name, "0", "---j="));
		backpatching_point_pos_.push_back(quadruples_stack_.size() - 1);

		PrintQuadruples(Quadruples(GetNextStateNum(), "j", "-", "-", "---j"));
		backpatching_point_pos_.push_back(quadruples_stack_.size() - 1);

		
		//返回下一个state的序号。给回填使用
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = std::to_string(PeekNextStateNum());

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);


		//将两个标签生成后压入栈中
		string table_name = symbol_tables_[current_symbol_table_stack_.back()].GetTableName();
		int cnt = w_j_label_num_++;
		string label_1 = "Label_" + table_name + "_if_end_" + std::to_string(cnt);
		string label_2 = "Label_" + table_name + "_else_end_" + std::to_string(cnt);
		//if_end先写
		w_label_.push_back(label_2);
		w_label_.push_back(label_1);
		//if_end先跳
		j_label_.push_back(label_2);
		j_label_.push_back(label_1);
		//目标代码生成
		string j_label_if_end = j_label_.back();
		j_label_.pop_back();

		////清空寄存器 --保证进入if或者else都是干净的
		//target_code_generator_.ResetRegs();
		//目标代码生成
		string tar_arg1_name = target_code_generator_.SetArgBeReady(symbol_expression.pos);

		target_code_generator_.ClearRegs();//进入循环前先清空寄存器
		//条件等于0则跳过if
		target_code_generator_.PrintQuadruples(Instructions("beq", tar_arg1_name, "$zero", j_label_if_end));

	}

	else if ("If_expression" == production.left && production.right[0] == "$")//If_expression->$
	{	
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";//这里为空


		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成
		//只输出一个if_end的label，把多余的w_label与w_label从栈中弹出。
		string w_label_if_end = w_label_.back();
		w_label_.pop_back();
		w_label_.pop_back();//弹出else_end，因为这是if不含else的语句
		j_label_.pop_back();//弹出else_end，同上

		//清空寄存器 --出if是干净的
		target_code_generator_.ClearRegs();
		//生成if_end标签
		target_code_generator_.PrintQuadruples(w_label_if_end + " :");
	}

	else if ("If_expression" == production.left && production.right[0] == "If_sentence_n")
	//If_expression->If_sentence_n else Sentence_block
	{	
		GrammarSymbolInfo symbol_if_sentence_n = symbol_info_stack[symbol_info_stack.size() - 3];
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = symbol_if_sentence_n.txt_value; //区别一下 空

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成
		//只输出一个else_end的label
		string w_label_else_end = w_label_.back();
		w_label_.pop_back();

		//清空寄存器 --保证出else是干净的
		target_code_generator_.ClearRegs();
		//生成if_end标签
		target_code_generator_.PrintQuadruples(w_label_else_end + " :");
	}

	else if ("If_sentence" == production.left)
	//If_sentence->if If_sentence_m0(Expression) If_sentence_m1 Sentence_block If_expression
	{		
		GrammarSymbolInfo symbol_if_sentence_m1 = symbol_info_stack[symbol_info_stack.size() - 3];
		GrammarSymbolInfo symbol_if_expression = symbol_info_stack.back();

		if ("" == symbol_if_expression.txt_value)//只有if，没有else
		{
			//开始回填 两个地方
			//真出口
			int batch_pos = backpatching_point_pos_.back();
			backpatching_point_pos_.pop_back();
			quadruples_stack_[batch_pos].SetContent(quadruples_stack_[batch_pos].num, quadruples_stack_[batch_pos].op,
				quadruples_stack_[batch_pos].arg1, quadruples_stack_[batch_pos].arg2, symbol_if_sentence_m1.txt_value);
			//假出口
			batch_pos = backpatching_point_pos_.back();
			backpatching_point_pos_.pop_back();
			quadruples_stack_[batch_pos].SetContent(quadruples_stack_[batch_pos].num, quadruples_stack_[batch_pos].op,
				quadruples_stack_[batch_pos].arg1, quadruples_stack_[batch_pos].arg2, std::to_string(PeekNextStateNum()));
		}
		else//if-else语句
		{
			//开始回填 三个地方
			//完成if，跳出if-else
			int batch_pos = backpatching_point_pos_.back();
			backpatching_point_pos_.pop_back();
			quadruples_stack_[batch_pos].SetContent(quadruples_stack_[batch_pos].num, quadruples_stack_[batch_pos].op,
				quadruples_stack_[batch_pos].arg1, quadruples_stack_[batch_pos].arg2, std::to_string(PeekNextStateNum()));
			
			//if真出口
			batch_pos = backpatching_point_pos_.back();
			backpatching_point_pos_.pop_back();
			quadruples_stack_[batch_pos].SetContent(quadruples_stack_[batch_pos].num, quadruples_stack_[batch_pos].op,
				quadruples_stack_[batch_pos].arg1, quadruples_stack_[batch_pos].arg2, symbol_if_sentence_m1.txt_value);

			//if假出口
			batch_pos = backpatching_point_pos_.back();
			backpatching_point_pos_.pop_back();
			quadruples_stack_[batch_pos].SetContent(quadruples_stack_[batch_pos].num, quadruples_stack_[batch_pos].op,
				quadruples_stack_[batch_pos].arg1, quadruples_stack_[batch_pos].arg2, symbol_if_expression.txt_value);
		}

		--backpatching_level_;//回填深度减1
		//判断是否需要输出
		if (backpatching_level_ == 0)
		{
			for (auto iter = quadruples_stack_.begin(); iter != quadruples_stack_.end(); ++iter)
				PrintQuadruples(*iter);
			int length = quadruples_stack_.size();
			for (int i = 0; i < length; ++i)//回填栈全部输出，可以清空了
				quadruples_stack_.pop_back();
		}

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成
		//target_code_generator_.ClearRegs();//退出if前先清空寄存器
	}

	
	else if ("Call_func_check" == production.left)//Call_func_check->$
	{
		//做语义检查，是否为合法的函数。
		//Factor->identifier ( Call_func_check Actual_parameter_list)
		//符号属性
		GrammarSymbolInfo symbol_identifier = symbol_info_stack[symbol_info_stack.size() - 2];


		//在符号表栈中一级级逆序寻找 identifier是否被定义过
		int identifier_pos = -1;
		int identifier_layer = current_symbol_table_stack_.size() - 1;
		for (; identifier_layer >= 0; --identifier_layer)
		{
			SymbolTable search_table = symbol_tables_[current_symbol_table_stack_[identifier_layer]];
			identifier_pos = search_table.FindSymbol(symbol_identifier.txt_value);
			if (identifier_pos != -1)//表示在某一级中找到了
				break;
		}
		if (-1 == identifier_pos )//没找到，表示不存在，说明没定义就使用，语义错误
		{
			cerr << "语义错误！！！" << symbol_identifier.txt_value << "没有定义！" << endl;
			return false;
		}

		if (FUNCTION != symbol_tables_[current_symbol_table_stack_[identifier_layer]].GetSymbolMode(identifier_pos))
		//找到了，但是这个标识符不是函数，语义错误
		{
			cerr << "语义错误！！！" << symbol_identifier.txt_value << "不是函数，无法被调用！" << endl;
			return false;
		}

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";
		conclude_syble_info.pos = { current_symbol_table_stack_[identifier_layer], identifier_pos };//存储被调用函数的位置

		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);
	}
	

	else if ("Expression_loop" == production.left && production.right[0] == "$")//Expression_loop->$
	{	
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "0";//这里为0，表示已经传递了0个参数


		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);
	}
	

	else if ("Expression_loop" == production.left && production.right[0] == "Expression_loop")
	//Expression_loop->Expression_loop Expression ,
	{	
	//Factor->identifier ( Call_func_check Expression_loop Expression ,
		//符号属性
		GrammarSymbolInfo symbol_call_func_check = symbol_info_stack[symbol_info_stack.size() - 4];
		GrammarSymbolInfo symbol_expression_loop = symbol_info_stack[symbol_info_stack.size() - 3];
		GrammarSymbolInfo symbol_expression = symbol_info_stack[symbol_info_stack.size() - 2];
		Symbol &called_function = symbol_tables_[symbol_call_func_check.pos.table_pos].GetSymbol(symbol_call_func_check.pos.symbol_pos);
		int parameters_num = called_function.parameter_num;
		int passed_parameters_num = std::stoi(symbol_expression_loop.txt_value);
		if (passed_parameters_num >= parameters_num)
		{//参数传递过多！
			cerr << "语义错误！！！" << called_function.name << "传递的参数过多！" << endl;
			return false;
		}

		int table_pos = called_function.func_symbol_pos;
		
		SymbolPos result_pos;
		result_pos.table_pos = table_pos;
		result_pos.symbol_pos = passed_parameters_num + 1;//第0个是返回值

		string result_name = GetArgName(result_pos, true);

		string arg1_name = GetArgName(symbol_expression.pos);
		
		PrintQuadruples(Quadruples(GetNextStateNum(), ":=", arg1_name, "-", result_name));

		passed_parameters_num++;
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = std::to_string(passed_parameters_num);


		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成
		//string tar_arg2_name = target_code_generator_.SetArgBeReady(symbol_expression.pos);
		string tar_arg1_name = target_code_generator_.SetArgBeReady(symbol_expression.pos);//获取实参的内存地址名称
		target_code_generator_.PrintQuadruples(Instructions("sw", tar_arg1_name, "($sp)", ""));
		target_code_generator_.PushStack_sp(1);//由于在sp上添加了实参,因此sp需要移动1*4

	}
	

	else if ("Actual_parameter_list" == production.left && production.right[0] == "Expression_loop")
	//Actual_parameter_list->Expression_loop Expression
	{	
	//Factor->identifier ( Call_func_check Expression_loop Expression
		//符号属性
		GrammarSymbolInfo symbol_call_func_check = symbol_info_stack[symbol_info_stack.size() - 3];
		GrammarSymbolInfo symbol_expression_loop = symbol_info_stack[symbol_info_stack.size() - 2];
		GrammarSymbolInfo symbol_expression = symbol_info_stack.back();
		Symbol &called_function = symbol_tables_[symbol_call_func_check.pos.table_pos].GetSymbol(symbol_call_func_check.pos.symbol_pos);
		int parameters_num = called_function.parameter_num;
		int passed_parameters_num = std::stoi(symbol_expression_loop.txt_value);
		if (passed_parameters_num >= parameters_num)
		{//参数传递过多！
			cerr << "语义错误！！！" << called_function.name << "传递的参数过多！" << endl;
			return false;
		}

		int table_pos = called_function.func_symbol_pos;
		
		SymbolPos result_pos;
		result_pos.table_pos = table_pos;
		result_pos.symbol_pos = passed_parameters_num + 1;//第0个是返回值

		string result_name = GetArgName(result_pos, true);

		string arg1_name = GetArgName(symbol_expression.pos);
		
		PrintQuadruples(Quadruples(GetNextStateNum(), ":=", arg1_name, "-", result_name));

		passed_parameters_num++;


		//判断是否参数过少！
		if (passed_parameters_num < parameters_num)
		{//参数传递过多！
			cerr << "语义错误！！！" << called_function.name << "传递的参数过少！" << endl;
			return false;
		}

		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = std::to_string(passed_parameters_num);


		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);

		//目标代码生成
		string tar_arg1_name = target_code_generator_.SetArgBeReady(symbol_expression.pos);//获取实参的内存地址名称
		target_code_generator_.PrintQuadruples(Instructions("sw", tar_arg1_name, "($sp)", ""));
		target_code_generator_.PushStack_sp(1);//由于在sp上添加了实参,因此sp需要移动1*4
	}
	
	else if ("Actual_parameter_list" == production.left && production.right[0] == "$")//Actual_parameter_list->$
	{
		//Call_func->( Call_func_check 
		//符号属性
		GrammarSymbolInfo symbol_call_func_check = symbol_info_stack.back();
		Symbol &called_function = symbol_tables_[symbol_call_func_check.pos.table_pos].GetSymbol(symbol_call_func_check.pos.symbol_pos);
		int parameters_num = called_function.parameter_num;
		/*int passed_parameters_num = */
		if (0 != parameters_num)
		{//参数传递过多！
			cerr << "语义错误！！！" << called_function.name << "函数的调用需要参数，但是没有传入参数！" << endl;
			return false;
		}
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "0";//这里为0，表示已经传递了0个参数


		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);
	}

	else if ("Call_func" == production.left)//Call_func->(Call_func_check Actual_parameter_list)
	{
	//输出call语句
		//Factor->identifier ( Call_func_check Actual_parameter_list)
		//符号属性
		GrammarSymbolInfo symbol_identifier = symbol_info_stack[symbol_info_stack.size() - 5];
		GrammarSymbolInfo symbol_call_func_check = symbol_info_stack[symbol_info_stack.size() - 3];
		PrintQuadruples(Quadruples(GetNextStateNum(), "call", "-", "-", symbol_identifier.txt_value));
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";
		conclude_syble_info.pos = symbol_call_func_check.pos;


		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for(int i = 0; i < length;++i)
			symbol_info_stack.pop_back();

		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("FTYPE" == production.left && production.right[0] == "Call_func")//FTYPE->Call_func
	{
		GrammarSymbolInfo symbol_call_func = symbol_info_stack.back();
		//将调用函数返回pos一级级向上传递
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "call_func";//这里只要不是""就可以
		conclude_syble_info.pos = symbol_call_func.pos;
		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	
	else if ("Program" == production.left)//Program->Stmt_string
	{
		//到这里基本语义分析正确。
		//判断是否存在Main函数。
		if (main_line_ == -1)
		{
			cerr << "语义错误！！！" << "main函数没有定义！" << endl;
			return false;
		}
		PrintQuadruples();
		cerr << "中间代码生成完毕！" << endl;
		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";
		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}

	else
	{
		
		//Stmt_string->Stmt_loop Stmt
		//Stmt_loop->Stmt_loop Stmt | $
		//Internal_stmt->$|Internal_variable_stmt ; Internal_stmt
		//Sentence_string->Sentence_loop Sentence
		//Sentence_loop->Sentence_loop Sentence | $
		//Sentence->If_sentence | While_sentence | Return_sentence | Assign_sentence
		//Stmt->void identifier Create_Function_table Function_stmt
		//Formal_parameter->Formal_parameter_list | void | $
		//Formal_parameter_list->Parameter_loop Parameter
		//Parameter_loop->Parameter_loop Parameter, | $
		//Function_stmt->( Formal_parameter ) Sentence_block Exit_Function_table


		//由规约后的产生式左部构造一个文法符号属性
		GrammarSymbolInfo conclude_syble_info;
		conclude_syble_info.symbol_name = production.left;
		conclude_syble_info.txt_value = "";
		//获取产生式右部的长度
		int length = 0;
		if ("$" != production.right[0])
			length = production.right.size();

		for (int i = 0; i < length; ++i)
			symbol_info_stack.pop_back();
		symbol_info_stack.push_back(conclude_syble_info);
	}
	

	//其它情况下的规约不产生动作
	return true;
}