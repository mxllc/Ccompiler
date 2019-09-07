#include "symbol_table.h"

SymbolTable::SymbolTable(TABLE_TYPE t_type, string iname)
{
	table_type_ = t_type;
	name_ = iname;
}

SymbolTable::~SymbolTable()
{
}

vector<Symbol> &SymbolTable::GetTable()
{
	return table_;
}

TABLE_TYPE SymbolTable::GetTableType()
{
	return table_type_;
}

Symbol &SymbolTable::GetSymbol(int pos)
{
	return table_[pos];
}

int SymbolTable::AddSymbol(const Symbol & sb)
{
	//双保险，在这里再进行一次判断，再加入符号表之前查看是否已经存在。
	//如果存在就返回-1，表示插入失败
	if (-1 != FindSymbol(sb.name))
		return -1;

	//原先不存在的情况，这个时候向符号表中添加符号
	table_.push_back(sb);
	return table_.size() - 1;
}

int SymbolTable::AddSymbol(const string &str)
{
	string tmp_name = "T" + std::to_string(table_.size());
	Symbol sb;
	sb.name = tmp_name;
	sb.mode = TEMP;
	sb.value = str;
	table_.push_back(sb);
	return table_.size() - 1;
}

int SymbolTable::AddSymbol()
{
	string tmp_name = "T" + std::to_string(table_.size());
	Symbol sb;
	sb.name = tmp_name;
	sb.mode = TEMP;
	table_.push_back(sb);
	return table_.size() - 1;
}

string SymbolTable::GetTableName()const
{
	return name_;
}

string SymbolTable::GetSymbolName(int pos) const
{
	return table_[pos].name;
}

int SymbolTable::FindSymbol(const string &name) const
{
	for (auto iter = table_.begin(); iter != table_.end(); ++iter)
	{
		if (name == iter->name)//符号的名称相同，表示找到
			return iter - table_.begin();
	}

	return -1;//没有找到
}

int SymbolTable::FindConst(const string &const_value) const
{
	for (auto iter = table_.begin(); iter != table_.end(); ++iter)
	{
		if (iter->mode != CONST)
			continue;
		if (const_value == iter->value)//常数的字面值相等
			return iter - table_.begin();
	}

	return -1;//没有找到
}

bool SymbolTable::SetValue(int pos, string &value)
{
	table_[pos].value = value;
	return true;
}
CATEGORY SymbolTable::GetSymbolMode(int pos)const
{
	return table_[pos].mode;
}
VARIABLE_TYPE SymbolTable::GetSymbolType(int pos)const
{
	return table_[pos].type;
}