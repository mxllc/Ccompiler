#include "target_code_generator.h"

TargetCodeGenerator::TargetCodeGenerator()
{
	target_code_printer_.open("./gen_data/target_file/target_code.txt");
	if (!target_code_printer_.is_open()) {
		cerr << "用来保存输出目标代码的文件打开失败！" << endl;
	}
	InitialSpFpGp();//初始化设置，没与Mars内存对齐会出现错误
	gf_offset = GF_INI_OFFSET;
}
void TargetCodeGenerator::TargetCodeGeneratorInit(vector<SymbolTable> &symbol_tables_)
{
	this->symbol_tables_pt_ = &symbol_tables_;
}
TargetCodeGenerator::~TargetCodeGenerator()
{
	if (target_code_printer_.is_open())
		target_code_printer_.close();
}

bool TargetCodeGenerator::InitialSpFpGp()
{
	//lui $sp, 0x1001
	//move $fp, $sp  初始的fp直接给0？ 待修改----------------------------------------------------------------
	PrintQuadruples(Instructions("lui", "$sp", "0x1001", ""));
	PrintQuadruples(Instructions("lui", "$gp", "0x1000", ""));
	/*PrintQuadruples(Instructions("move", "$fp", "$sp", ""));*/
	PrintQuadruples(Instructions("j", "main", "", ""));
	return true;
}

bool TargetCodeGenerator::PrintQuadruples(const Instructions &instr)
{
	target_code_printer_ << instr.op << " " << instr.arg1 << " "
		<< instr.arg2 << " " << instr.arg3 << endl;
	
	return true;
}

bool TargetCodeGenerator::PrintSyscall()
{
	target_code_printer_ << "syscall" << endl;
	return true;
}

bool TargetCodeGenerator::PrintQuadruples(const string&label)
{
	target_code_printer_ << label << endl;
	return true;
}

bool TargetCodeGenerator::PushStack_sp(int num)
{
	int size = num * 4;
	PrintQuadruples(Instructions("addi", "$sp", "$sp", std::to_string(size)));
	//类似 addi $sp $sp 12
	return true;
}

bool TargetCodeGenerator::CreateStackFrame()
{
	target_code_printer_ << "#创建栈帧" << endl;
	//保存返回地址 sw $ra ($sp)
	PrintQuadruples(Instructions("sw", "$ra", "($sp)", ""));
	//保存老fp sw $ra ($sp)
	PrintQuadruples(Instructions("sw", "$fp", "4($sp)", ""));
	//更新fp addi $fp $sp 4
	PrintQuadruples(Instructions("addi", "$fp", "$sp", "4"));
	//sp + 2*4
	PushStack_sp(2);
	target_code_printer_ << endl << endl;
	return true;
}

void TargetCodeGenerator::SetMissTime(int reg)
{
	//除了reg变成0，其余都+1
	int length = registers_.size();
	for (int i = 0; i < length; ++i)
	{
		if (reg == i)
			registers_[i].miss_time = 0;
		else
			registers_[i].miss_time += 1;
	}
	return;
}

int TargetCodeGenerator::GetTempReg()
{
	//表示寄存器没被全部占用
	int reg = -1;
	int miss_t = -1;//没被使用的次数

	//寄存器全部被占用了
	int reg_p = -1;
	int miss_t_p = -1;

	for (auto iter = registers_.begin(); iter != registers_.end(); ++iter)
	{
		//寄存器存在可用的
		if (iter->is_possessed == false)//还没被占用
		{
			if (iter->miss_time > miss_t)//没被使用的次数
			{
				reg = iter - registers_.begin();
				miss_t = iter->miss_time;
			}
		}
		//寄存器全部被占用
		if (iter->miss_time > miss_t_p)//没被使用的次数
		{
			reg_p = iter - registers_.begin();
			miss_t_p = iter->miss_time;
		}
	}

	if (reg != -1)//表示存在没被占用的寄存器，找出最近第一个最少使用的寄存器就直接返回reg
		return reg;
	else//表示所有寄存器都被占用了
		return reg_p;
}

bool TargetCodeGenerator::IsSymbolLoaded(SymbolPos &pos)const
{
	vector<SymbolTable> &symbol_tables = *symbol_tables_pt_;
	Symbol &sb = symbol_tables[pos.table_pos].GetSymbol(pos.symbol_pos);
	if (sb.reg == -1)
		return false;
	else
		return true;
}

bool TargetCodeGenerator::LoadTempReg(int reg, SymbolPos &pos)
{
	vector<SymbolTable> &symbol_tables = *symbol_tables_pt_;
	////先检查pos 对应的符号是否已经在寄存器-------------------------这里先不做，调用这个函数的时候就得先检查
	Symbol &sb = symbol_tables[pos.table_pos].GetSymbol(pos.symbol_pos);
	
	
	if (true == registers_[reg].is_possessed)//被占用了，需要先写回到内存。
	{
		string sw_mem_addr = GetMemAddr(registers_[reg].content_info);
		PrintQuadruples(Instructions("sw", registers_[reg].name, sw_mem_addr, ""));
		//表示不在寄存器上了
		SymbolPos reg_pos = registers_[reg].content_info;
		symbol_tables[reg_pos.table_pos].GetSymbol(reg_pos.symbol_pos).reg = -1;
	}
	
	string mem_addr = GetMemAddr(pos);
	PrintQuadruples(Instructions("lw", registers_[reg].name, mem_addr, ""));

	//设置寄存器信息
	registers_[reg].content_info = pos;
	registers_[reg].is_possessed = true;
	SetMissTime(reg);
	
	//设置symbol信息
	sb.reg = reg;

	return true;
	//if (false == registers_[reg].is_possessed)//没被占用，可用直接写到reg
	//{
	//	string mem_addr = GetMemAddr(pos);
	//	PrintQuadruples(Instructions("lw", registers_[reg].name, mem_addr, ""));
	//	return true;
	//}
	//else//被占用了，需要先写到内存。
	//{
	//	string sw_mem_addr = GetMemAddr(registers_[reg].content_info);
	//	PrintQuadruples(Instructions("sw", registers_[reg].name, sw_mem_addr, ""));
	//	string mem_addr = GetMemAddr(pos);
	//	PrintQuadruples(Instructions("lw", registers_[reg].name, mem_addr, ""));
	//}
}

string TargetCodeGenerator::GetMemAddr(const SymbolPos &pos) const//根据Pos返回内存地址 4($gp), -4($fp) 等等
{
	string addr = "";
	//需要考虑的情况：临时变量、全局变量、局部变量、形参。    返回值？---------------------待考虑-------------------
	vector<SymbolTable> &symbol_tables = *symbol_tables_pt_;
	if (pos.table_pos == 1)//临时变量
	{
		addr = "($gp)";
		int offset_gp = GF_INI_OFFSET + pos.symbol_pos;
		addr = std::to_string(offset_gp * 4) + addr;
	}
	else if (pos.table_pos == 0)//全局变量
	{
		addr = "($gp)";
		
		int offset_gp = -1;
		vector<Symbol> &global_table = symbol_tables[0].GetTable();
		for (auto iter = global_table.begin(); iter != global_table.end(); ++iter)
		{
			if (iter->mode == FUNCTION)
				continue;
			offset_gp++;
			if (iter - global_table.begin() == pos.symbol_pos)
				break;
		}
		addr = std::to_string(offset_gp * 4) + addr;

		//cerr << "TargetCodeGenerator::GetMemAddr  全局变量，等待修改！" << endl;
	}
	else//局部变量，形参，返回值
	{
		int func_pos_in_global = symbol_tables[0].FindSymbol(symbol_tables[pos.table_pos].GetTableName());
		Symbol &func = symbol_tables[0].GetSymbol(func_pos_in_global);//获取变量所在的符号表

		if (pos.symbol_pos == 0)//返回值
		{
			Symbol return_value = symbol_tables[pos.table_pos].GetSymbol(0);
			if (return_value.type == INT)
				addr = "$v0";
			else//type == VOID
				addr = "";

			//cerr << "TargetCodeGenerator::GetMemAddr  返回值，等待修改！" << endl;
		}
		else
		{
			
			int par_num = func.parameter_num;//获取形参个数
			if (pos.symbol_pos <= par_num)//位置在形参个数之内，表示形参
			{
				addr = "($fp)";
				int offset_fp = -2 - par_num + pos.symbol_pos;//($fp)老fp  -4($fp)存返回地址
				addr = std::to_string(offset_fp * 4) + addr;
			}
			else//局部变量
			{
				addr = "($fp)";
				int offset_fp = pos.symbol_pos - par_num;//($fp)老fp  4($fp)存第0个局部变量
				addr = std::to_string(offset_fp * 4) + addr;
			}
		}
	}
	return addr;
}

int TargetCodeGenerator::LoadImmToReg(const string&imm, const SymbolPos &pos)
{
	vector<SymbolTable> &symbol_tables = *symbol_tables_pt_;
	int reg = GetTempReg();//获取下一个可用的

	if (true == registers_[reg].is_possessed)//被占用了，需要先写回到内存。
	{
		string sw_mem_addr = GetMemAddr(registers_[reg].content_info);
		PrintQuadruples(Instructions("sw", registers_[reg].name, sw_mem_addr, ""));
		//表示不在寄存器上了
		SymbolPos reg_pos = registers_[reg].content_info;
		symbol_tables[reg_pos.table_pos].GetSymbol(reg_pos.symbol_pos).reg = -1;
	}

	PrintQuadruples(Instructions("addi", registers_[reg].name, "$zero", imm));//加载立即数

	Symbol &sb = symbol_tables[pos.table_pos].GetSymbol(pos.symbol_pos);
	//设置寄存器信息
	registers_[reg].content_info = pos;
	registers_[reg].is_possessed = true;
	SetMissTime(reg);

	//设置symbol信息
	sb.reg = reg;

	return true;
}

string TargetCodeGenerator::SetArgBeReady(SymbolPos pos)
{
	string name = "$t";
	vector<SymbolTable> &symbol_tables = *symbol_tables_pt_;
	Symbol &sb = symbol_tables[pos.table_pos].GetSymbol(pos.symbol_pos);//获取该符号

	if (sb.reg == -1)//需要装载
	{
		int reg = GetTempReg();
		LoadTempReg(reg, pos);	
	}
	name += std::to_string(sb.reg);

	return name;
}

bool TargetCodeGenerator::ClearRegs()
{
	vector<SymbolTable> &symbol_tables = *symbol_tables_pt_;
	for (auto iter = registers_.begin(); iter != registers_.end(); ++iter)
	{
		if (iter->is_possessed == false)
			continue;
		SymbolPos pos = iter->content_info;
		Symbol &sb = symbol_tables[pos.table_pos].GetSymbol(pos.symbol_pos);//获取该符号
		sb.reg = -1;
		iter->is_possessed = false;
		iter->miss_time = 0;
		iter->content_info = { -1, -1 };
		if (sb.mode == TEMP)
			continue;
		//不是临时变量，需要在mips中写回
		int reg = iter - registers_.begin();
		string mem_addr = GetMemAddr(pos);
		PrintQuadruples(Instructions("sw", registers_[reg].name, mem_addr, ""));
	}
	return true;
}

bool TargetCodeGenerator::ResetRegs()
{
	vector<SymbolTable> &symbol_tables = *symbol_tables_pt_;
	for (auto iter = registers_.begin(); iter != registers_.end(); ++iter)
	{
		if (iter->is_possessed == false)
			continue;
		SymbolPos pos = iter->content_info;
		Symbol &sb = symbol_tables[pos.table_pos].GetSymbol(pos.symbol_pos);//获取该符号
		sb.reg = -1;
		iter->is_possessed = false;
		iter->miss_time = 0;
		iter->content_info = { -1, -1 };
	}
	return true;
}