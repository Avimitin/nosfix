void *__fastcall sub_1802C5350(__int64 a1, __int128 *a2, void *a3)
{
  __int128 v6; // xmm0
  _OWORD *v7; // rdx
  void *result; // rax
  __int128 v9; // [rsp+20h] [rbp-28h] BYREF
  __int128 v10; // [rsp+30h] [rbp-18h] BYREF
  LARGE_INTEGER PerformanceCount; // [rsp+50h] [rbp+8h] BYREF

  PerformanceCount.QuadPart = 0; /*0x1802c5362*/
  QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1802c5376*/
  unknown_libname_4(a1: &v10); /*0x1802c5381*/
  v6 = *a2; /*0x1802c538f*/
  v7 = *(_OWORD **)(a1 + 24); /*0x1802c5392*/
  *(LARGE_INTEGER *)&v9 = PerformanceCount; /*0x1802c5399*/
  result = &unk_1803D290F; /*0x1802c539e*/
  if ( a3 != nullptr ) /*0x1802c53a5*/
    result = a3; /*0x1802c53a5*/
  *((_QWORD *)&v9 + 1) = result; /*0x1802c53a9*/
  v10 = v6; /*0x1802c53ae*/
  if ( *(_OWORD **)(a1 + 32) == v7 ) /*0x1802c53b7*/
    return (void *)sub_1800C7E20(a1: a1 + 16, a2: v7, a3: &v9); /*0x1802c53e4*/
  *v7 = v9; /*0x1802c53be*/
  v7[1] = v10; /*0x1802c53c6*/
  *(_QWORD *)(a1 + 24) += 32LL; /*0x1802c53ca*/
  return result; /*0x1802c53cf*/
}
