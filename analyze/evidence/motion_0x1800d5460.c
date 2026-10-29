__int64 __fastcall sub_1800D5460(
        __int64 a1,
        int a2,
        int a3,
        int a4,
        int a5,
        int a6,
        int a7,
        int a8,
        int a9,
        __int64 a10,
        __int64 a11,
        __int64 a12,
        __int64 a13)
{
  float v14; // xmm9_4
  int v15; // eax
  int v16; // eax
  __int64 result; // rax

  *(_DWORD *)(a1 + 8) = 0; /*0x1800d5488*/
  *(float *)a1 = (float)a2; /*0x1800d548e*/
  *(float *)(a1 + 4) = (float)a3; /*0x1800d5492*/
  v14 = (float)a4; /*0x1800d54cc*/
  *(_DWORD *)(a1 + 16) = sub_1800D2440(a1: a10); /*0x1800d54d0*/
  *(_DWORD *)(a1 + 12) = 1065353216; /*0x1800d54d3*/
  *(_DWORD *)(a1 + 28) = 0; /*0x1800d54da*/
  *(float *)(a1 + 20) = v14; /*0x1800d54e1*/
  *(float *)(a1 + 24) = (float)a5; /*0x1800d54e7*/
  *(_DWORD *)(a1 + 36) = sub_1800D2440(a1: a12); /*0x1800d550f*/
  *(_DWORD *)(a1 + 32) = 1065353216; /*0x1800d5512*/
  *(_DWORD *)(a1 + 48) = 0; /*0x1800d5519*/
  *(float *)(a1 + 40) = (float)a8; /*0x1800d551f*/
  *(float *)(a1 + 44) = (float)a9; /*0x1800d5524*/
  v15 = sub_1800D2440(a1: a11); /*0x1800d5529*/
  *(float *)(a1 + 60) = (float)a8; /*0x1800d5536*/
  *(float *)(a1 + 64) = (float)a9; /*0x1800d553b*/
  *(_DWORD *)(a1 + 56) = v15; /*0x1800d5540*/
  *(_DWORD *)(a1 + 52) = 1065353216; /*0x1800d5543*/
  *(_DWORD *)(a1 + 68) = 0; /*0x1800d554a*/
  v16 = sub_1800D2440(a1: a11); /*0x1800d554d*/
  *(float *)(a1 + 80) = v14; /*0x1800d555a*/
  *(float *)(a1 + 84) = (float)a5; /*0x1800d5560*/
  *(_DWORD *)(a1 + 76) = v16; /*0x1800d5566*/
  *(_DWORD *)(a1 + 72) = 1065353216; /*0x1800d5569*/
  *(_DWORD *)(a1 + 88) = 0; /*0x1800d5570*/
  *(_DWORD *)(a1 + 96) = sub_1800D2440(a1: a12); /*0x1800d5595*/
  *(_DWORD *)(a1 + 92) = 1065353216; /*0x1800d5598*/
  *(_DWORD *)(a1 + 108) = 0; /*0x1800d559f*/
  *(float *)(a1 + 100) = (float)a6; /*0x1800d55a5*/
  *(float *)(a1 + 104) = (float)a7; /*0x1800d55aa*/
  result = sub_1800D2440(a1: a13); /*0x1800d55af*/
  *(_DWORD *)(a1 + 116) = result; /*0x1800d55d4*/
  *(_DWORD *)(a1 + 112) = 1065353216; /*0x1800d55d7*/
  return result; /*0x1800d55b4*/
}
