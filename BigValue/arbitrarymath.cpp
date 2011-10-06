// arbitrarymath.cpp : Defines the entry point for the console application.
//

#include "BigValue.h"

int main(int argc, char* argv[])
{
	CBigValue ip(1234LL);
	uint64_t absip = (uint64_t)ip;
	CBigValue im(-1234LL);
	uint64_t absim = (uint64_t)im;

	CBigValue inb(-1100110011001100110LL);
	uint64_t absinb = (uint64_t)inb;
	CBigValue ipb(1100110011001100110LL);
	uint64_t absipb = (uint64_t)ipb;

	CBigValue ui(1234ULL);
	uint64_t absui = (uint64_t)ui;
	CBigValue uib(1100110011001100110ULL);
	uint64_t absuib = (uint64_t)uib;

	/*
	CBigValue fp(42.1f);
	CBigValue fm(-42.1f);

	CBigValue dp(42.10);
	CBigValue dm(-42.10);
	*/

	return 0;
}

