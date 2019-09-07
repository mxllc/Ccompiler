void main(void)
{
	int a;
	int b;
	int c;
	int d;
	a = 1;
	b = 1;
	c = 2;
	d = 20;
	
	while(a<10)
	{
		b = 1;
		while(b < 10)
		{
			if (b == 5)
			{
				c = c + a * b;
				d = d + a / b;
			}
			else
			{
				if (b > 7)
				{
					c = c + a + b;
					d = d + a / b;
				}
				
				if (b < 3)
				{
					c = c + a - b;
					d = d + a * b;
				}
			}
			
			b = b + 1;
		}
		
		a = a +1;
	}
	
	if(c>100)
	{
		a = 100;
	}
	else
	{
		b = 100;
	}
	
	
	return;
}
