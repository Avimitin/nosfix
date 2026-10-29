__m128 __fastcall sub_180004040(__int64 a1, __int16 a2)
{
  __int128 v4; // xmm7
  __int128 v5; // xmm6
  float *v6; // rbx
  __int128 v7; // xmm0
  float v9; // xmm1_4
  LARGE_INTEGER PerformanceCount; // [rsp+20h] [rbp-68h] BYREF

  v4 = 0xBF800000; /*0x180004093*/
  v5 = 0xBF800000; /*0x180004097*/
  avs2_core_244(a1: a1 + 50312, a2: 0, a3: 8); /*0x18000409b*/
  QueryPerformanceFrequency(lpFrequency: (LARGE_INTEGER *)(a1 + 50312)); /*0x1800040a8*/
  v6 = (float *)&unk_180038838; /*0x1800040ae*/
  do /*0x18000413d*/
  {
    if ( *((_WORD *)v6 + 1) == a2 && *(_WORD *)v6 == *(_WORD *)a1 && *((_QWORD *)v6 - 1) != 0 ) /*0x1800040d8*/
    {
      *(double *)&v7 = sub_180001260(a1: &unk_188F5FEC0); /*0x1800040e1*/
      v4 = v7; /*0x1800040f1*/
      *(float *)&v4 = *(float *)&v7 - v6[9]; /*0x1800040f4*/
      avs2_core_244(a1: &PerformanceCount, a2: 0, a3: 8); /*0x1800040f9*/
      QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x180004104*/
      v5 = 0; /*0x18000410a*/
      *(float *)&v5 = (float)((float)((float)(int)PerformanceCount.LowPart - (float)(int)*((_QWORD *)v6 + 5)) * 1000.0) /*0x180004132*/
                    / (float)(int)*(_QWORD *)(a1 + 50312);
    }
    v6 += 14; /*0x180004136*/
  }
  while ( (__int64)v6 < (__int64)&unk_180046838 ); /*0x18000413d*/
  if ( *(float *)&v4 == -1.0 ) /*0x180004145*/
  {
    return (__m128)0xBF800000; /*0x180004147*/
  }
  else
  {
    v9 = fabs((float)(*(float *)&v4 - *(float *)&v5)); /*0x180004163*/
    if ( v9 <= 100.0 ) /*0x18000416e*/
      return (__m128)v5; /*0x180004175*/
    else
      return (__m128)v4; /*0x180004170*/
  }
}
