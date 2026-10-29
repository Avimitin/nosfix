__int64 sub_1802C5030()
{
  int v0; // ecx
  double v1; // xmm1_8
  int v2; // ecx
  double v3; // xmm1_8
  LARGE_INTEGER PerformanceCount; // [rsp+30h] [rbp+8h] BYREF

  PerformanceCount.QuadPart = 0; /*0x1802c5039*/
  QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1802c5042*/
  v0 = PerformanceCount.LowPart - qword_1813E6D98; /*0x1802c5059*/
  qword_1813E6D98 = PerformanceCount.QuadPart; /*0x1802c5062*/
  v1 = (double)v0; /*0x1802c506e*/
  v2 = dword_1813E6C28; /*0x1802c507e*/
  v3 = v1 / (double)(int)Frequency.LowPart; /*0x1802c5085*/
  dbl_1812CF790[dword_1813E6C28] = v3; /*0x1802c5089*/
  ++v2; /*0x1802c508e*/
  qword_1813E7FE8 = *(_QWORD *)&v3; /*0x1802c5092*/
  dword_1813E6C28 = v2 % 30; /*0x1802c50ab*/
  return (unsigned int)(30 * (v2 / 30)); /*0x1802c50b1*/
}
