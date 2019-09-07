int fac(int x)   //递归函数
{
	int f;
	if(x <= 1)
	{
		f=1;
	}
	else
	{
		f=fac(x-1)*x;
	}
	return f;
}

void main(void)
{
	int a;
	int b;
	int c;
	a=3;
	a=fac(fac(a));
	return;
}