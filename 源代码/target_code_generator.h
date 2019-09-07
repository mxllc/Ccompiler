#pragma once
#ifndef TARGET_CODE_GENERATOR
#define TARGET_CODE_GENERATOR
#include "utils.h"
#include "symbol_table.h"
#include <iostream>
#include <string>
#include <fstream>
using namespace std;

#define GF_INI_OFFSET 1024
class TargetCodeGenerator {
private:
	ofstream target_code_printer_;//目标代码打印机
	vector<SymbolTable> *symbol_tables_pt_;//符号表容器指针，每一个符号表对应一个过程   ----与语义分析器共享同一个。 使用的时候解指针
	
	vector<RegisterInfo> registers_ = { { "$t0","" },{ "$t1","" },{ "$t2","" },{ "$t3","" },{ "$t4","" },
										{ "$t5","" },{ "$t6","" },{ "$t7","" },{ "$t8","" },{ "$t9","" } };//临时寄存器的初始化

	int gf_offset;//gf的偏移, 这个1个单位代表内存4个单位。前1024个单元（1024*4）留给全局变量
	void SetMissTime(int reg);//除了reg变成0，其余都+1
public:
	TargetCodeGenerator();
	~TargetCodeGenerator();
	void TargetCodeGeneratorInit(vector<SymbolTable> &);
	bool PrintQuadruples(const Instructions &);//根据指令进行打印目标代码临时文件
	bool PrintQuadruples(const string&);//生成对应与string的跳转label
	bool PushStack_sp(int num);//sp+4*num的指令，在声明变量、函数调用时候执行
	bool CreateStackFrame();//发生函数调用后，进入被调用函数时，首先进行栈帧的构建； 项目：返回地址、旧fp
	bool InitialSpFpGp();//初始化sp,fp,gp的值
	int GetTempReg();//使用LRU算法获取接下来应该被使用的寄存器。返回寄存器在vector中的序号
	bool LoadTempReg(int reg, SymbolPos &pos);//把符号表中指定的符号写到指定reg
	bool IsSymbolLoaded(SymbolPos &)const;//判断符号是否已经被加载到寄存器上
	string GetMemAddr(const SymbolPos &pos)const;//根据Pos返回内存地址 4($gp), -4($fp) 等等
	bool PrintSyscall();//main的结束
	int LoadImmToReg(const string&, const SymbolPos &pos);//把立即数加载到临时寄存器中，并返回寄存器的序号
	string SetArgBeReady(SymbolPos pos);//让参与运算的数进入临时寄存器，返回寄存器名称
	bool ClearRegs();//将寄存器上的局部变量全部写回，并情况寄存器
	bool ResetRegs();//将10个寄存器设置成no_possessed
};


#endif
