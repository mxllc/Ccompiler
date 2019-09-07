#include "utils.h"

Quadruples::Quadruples() {}
Quadruples::Quadruples(int n, const string &o, const string &a1, const string &a2, const string &r)
{
	num = n;
	op = o;
	arg1 = a1;
	arg2 = a2;
	result = r;
}
void Quadruples::SetContent(int n, const string &o, const string &a1, const string &a2, const string &r)
{
	num = n;
	op = o;
	arg1 = a1;
	arg2 = a2;
	result = r;
	return;
}

Instructions::Instructions() {}

Instructions::Instructions(const string &op, const string & arg1, const string & arg2, const string & arg3)
{
	this->op = op;
	this->arg1 = arg1;
	this->arg2 = arg2;
	this->arg3 = arg3;
}
void Instructions::SetContent(const string &op, const string & arg1, const string & arg2, const string & arg3)
{
	this->op = op;
	this->arg1 = arg1;
	this->arg2 = arg2;
	this->arg3 = arg3;
}
