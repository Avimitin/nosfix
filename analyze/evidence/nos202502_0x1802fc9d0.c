__int64 __fastcall sub_1802FC9D0(__int64 a1)
{
  int v1; // r11d
  __int64 v2; // r8
  int v3; // edx
  __int64 v4; // r9
  int v5; // r10d
  float v6; // xmm3_4
  int v7; // eax
  bool v8; // zf
  float v9; // xmm2_4
  __int64 result; // rax

  v1 = *(_DWORD *)a1; /*0x1802fc9d0*/
  v2 = *(int *)(a1 + 316); /*0x1802fc9d3*/
  v3 = *(_DWORD *)a1 + 1; /*0x1802fc9da*/
  *(_DWORD *)a1 = v3; /*0x1802fc9de*/
  if ( (int)v2 < *(_DWORD *)(a1 + 312) ) /*0x1802fc9e7*/
  {
    v4 = *(_QWORD *)(a1 + 328); /*0x1802fc9e9*/
    v5 = *(_DWORD *)(v4 + 8 * v2 + 12); /*0x1802fc9f0*/
    if ( v3 == v5 ) /*0x1802fc9f8*/
    {
      *(_DWORD *)(a1 + 276) = *(_DWORD *)(v4 + 8 * v2 + 8); /*0x1802fc9ff*/
      *(_DWORD *)(a1 + 300) = *(_DWORD *)(a1 + 292); /*0x1802fca0b*/
      *(_DWORD *)(a1 + 316) = v2 + 1; /*0x1802fca15*/
      *(_DWORD *)(a1 + 308) = v5; /*0x1802fca1b*/
    }
  }
  v6 = *(float *)(a1 + 292); /*0x1802fca22*/
  v7 = v3 - *(_DWORD *)(a1 + 308); /*0x1802fca2c*/
  v8 = *(_BYTE *)(a1 + 288) == 0; /*0x1802fca32*/
  *(float *)(a1 + 296) = v6; /*0x1802fca39*/
  v9 = (float)((float)v7 /*0x1802fca60*/
             / (float)((float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300) / *(float *)(a1 + 276)))
     + *(float *)(a1 + 300);
  *(float *)(a1 + 292) = v9; /*0x1802fca68*/
  if ( v8 ) /*0x1802fca70*/
  {
    if ( v3 >= 0 && ((float)(v6 - (float)(int)v6) > (float)(v9 - (float)(int)v9) || v3 == 0) ) /*0x1802fcaa7*/
      *(_BYTE *)(a1 + 288) = 1; /*0x1802fcaa9*/
  }
  else
  {
    *(_BYTE *)(a1 + 288) = 0; /*0x1802fca72*/
  }
  result = *(unsigned int *)(a1 + 280); /*0x1802fcab0*/
  *(_DWORD *)(a1 + 284) = result; /*0x1802fcab6*/
  if ( v1 >= 0 ) /*0x1802fcabf*/
  {
    *(float *)(a1 + 280) = 1.0 - (float)(v9 - (float)(int)v9); /*0x1802fcaed*/
    return (unsigned int)(int)v9; /*0x1802fcada*/
  }
  else
  {
    *(_DWORD *)(a1 + 280) = 1065353216; /*0x1802fcac9*/
  }
  return result; /*0x1802fcad1*/
}
