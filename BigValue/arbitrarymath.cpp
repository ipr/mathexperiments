// arbitrarymath.cpp : Defines the entry point for the console application.
//

#include "BigValue.h"

int main(int argc, char* argv[])
{
	CBigValue ip(1234LL);
	CBigValue im(-1234LL);
	CBigValue j(1234ULL);

	CBigValue fp(42.1f);
	CBigValue fm(-42.1f);

	//CBigValue dp(42.1d);
	//CBigValue dm(-42.1d);

	return 0;
}

