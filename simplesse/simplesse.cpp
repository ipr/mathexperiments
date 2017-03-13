// simplesse.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
//#include "simplesse.h"

#include <intrin.h> // __cpuid
//#include <pmmintrin.h>

#include <xmmintrin.h>


enum CpuExFlag
{
	CpuExFlagSSE3 = 0x1, // SSE3
	CpuExFlagSSE41 = 0x80000 // SSE4.1
};

// See http://msdn.microsoft.com/en-us/library/hskdteyh.aspx
bool checksupport(CpuExFlag flag)
{
    int CPUInfo[4] = {-1};

	// __cpuid && __cpuidex: function id & subfunction id = leaf & subleaf

	// first call: get highest available non-extended function id
    __cpuid( CPUInfo, 0 );
    if ( CPUInfo[0] < 1  )
        return false;

	// second call
    __cpuid(CPUInfo, 1 );

	// SSE 4.1
    return ( (CPUInfo[2] & flag) == flag );
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	if (checksupport(CpuExFlagSSE41) == false)
	{
		return -1;
	}

	// force 16 byte align
	__declspec(align(16)) struct vec_t { float m_v[4]; }; // 16 byte -> align

	vec_t v1 = {1.0, 1.0, 1.0, 1.0};
	vec_t v2 = {2.2, 2.2, 2.2, 2.2};
	vec_t vec_res = {0.0, 0.0, 0.0, 0.0};
	__asm 
	{  
		movaps xmm0, [v1] ; v1.[w,z,y,x]
		addps xmm0, [v2]  ; [].w+w, z+z, y+y, x+x
		movaps [vec_res], xmm0 ; back to c++ data
	}
	return 0;
}

