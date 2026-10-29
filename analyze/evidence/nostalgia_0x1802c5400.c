__int64 __fastcall sub_1802C5400(_QWORD *a1)
{
  _QWORD *v2; // rbx
  _OWORD *v3; // rdx
  __int64 result; // rax
  __int64 v5; // rcx
  __int64 v6; // rcx
  __int128 v7; // [rsp+20h] [rbp-28h] BYREF
  __int128 v8; // [rsp+30h] [rbp-18h] BYREF
  LARGE_INTEGER PerformanceCount; // [rsp+50h] [rbp+8h] BYREF

  PerformanceCount.QuadPart = 0; /*0x1802c540d*/
  QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1802c541b*/
  unknown_libname_4(a1: &v8); /*0x1802c5426*/
  v2 = a1 + 2; /*0x1802c5430*/
  v3 = (_OWORD *)a1[3]; /*0x1802c543b*/
  *(LARGE_INTEGER *)&v7 = PerformanceCount; /*0x1802c543f*/
  *((_QWORD *)&v7 + 1) = &unk_1803D291D; /*0x1802c544b*/
  v8 = xmmword_1813655B0; /*0x1802c5450*/
  if ( (_OWORD *)a1[4] == v3 ) /*0x1802c5459*/
  {
    sub_1800C7E20(a1: a1 + 2, a2: v3, a3: &v7); /*0x1802c547b*/
  }
  else
  {
    *v3 = v7; /*0x1802c5460*/
    v3[1] = v8; /*0x1802c5468*/
    a1[3] += 32LL; /*0x1802c546c*/
  }
  result = a1[5]; /*0x1802c5480*/
  a1[6] = result; /*0x1802c5488*/
  if ( a1 + 5 != v2 ) /*0x1802c548f*/
  {
    a1[5] = *v2; /*0x1802c5497*/
    *v2 = result; /*0x1802c549a*/
    v5 = a1[6]; /*0x1802c54a1*/
    a1[6] = a1[3]; /*0x1802c54a5*/
    a1[3] = v5; /*0x1802c54a9*/
    result = a1[4]; /*0x1802c54ad*/
    v6 = a1[7]; /*0x1802c54b1*/
    a1[7] = result; /*0x1802c54b5*/
    a1[4] = v6; /*0x1802c54b9*/
  }
  return result; /*0x1802c54bd*/
}
