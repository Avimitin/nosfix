__int64 __fastcall sub_1802A8150(unsigned __int8 *a1)
{
  _QWORD *v1; // r9
  _DWORD *v3; // r10
  int v4; // r8d
  int v5; // edx
  __int64 v6; // rdx
  int v7; // edi
  int v8; // ecx
  __int64 v9; // rsi
  int v10; // eax
  unsigned __int8 *v11; // rbx
  __int64 v12; // rcx
  __int64 v13; // rax
  __int64 result; // rax
  int v15; // [rsp+20h] [rbp-18h] BYREF
  __int64 v16; // [rsp+24h] [rbp-14h]
  __int16 v17; // [rsp+2Ch] [rbp-Ch]
  char v18; // [rsp+2Eh] [rbp-Ah]
  char v19; // [rsp+2Fh] [rbp-9h]
  _QWORD *v20; // [rsp+40h] [rbp+8h]

  v1 = (_QWORD *)qword_1812D1E70; /*0x1802a8164*/
  if ( qword_1812D1E70 == 0 ) /*0x1802a817a*/
  {
    v1 = operator new(Size: 0x40u); /*0x1802a8189*/
    qword_1812D1E70 = (__int64)v1; /*0x1802a818c*/
    *v1 = &CDeviceDriver::`vftable'; /*0x1802a8193*/
    v1[1] = 0; /*0x1802a8196*/
    v1[2] = 0; /*0x1802a819a*/
    v1[3] = 0; /*0x1802a819e*/
    v1[4] = 0; /*0x1802a81a2*/
    v1[5] = 0; /*0x1802a81a6*/
    v1[6] = 0; /*0x1802a81aa*/
    *((_DWORD *)v1 + 14) = 0; /*0x1802a81ae*/
    *((_WORD *)v1 + 30) = 0; /*0x1802a81b1*/
  }
  v3 = (_DWORD *)qword_1812D1E68; /*0x1802a81b5*/
  v4 = a1[34]; /*0x1802a81bc*/
  v5 = a1[33]; /*0x1802a81c1*/
  *(_DWORD *)(qword_1812D1E68 + 2536) = a1[32]; /*0x1802a81c9*/
  v3[635] = v5; /*0x1802a81d0*/
  v3[636] = v4; /*0x1802a81d7*/
  if ( v1 == nullptr ) /*0x1802a81e1*/
  {
    v20 = operator new(Size: 0x40u); /*0x1802a81ec*/
    v3 = (_DWORD *)qword_1812D1E68; /*0x1802a81f1*/
    qword_1812D1E70 = (__int64)v20; /*0x1802a81f8*/
    *v20 = &CDeviceDriver::`vftable'; /*0x1802a81ff*/
    v20[1] = 0; /*0x1802a8202*/
    v20[2] = 0; /*0x1802a8206*/
    v20[3] = 0; /*0x1802a820a*/
    v20[4] = 0; /*0x1802a820e*/
    v20[5] = 0; /*0x1802a8212*/
    v20[6] = 0; /*0x1802a8216*/
    *((_DWORD *)v20 + 14) = 0; /*0x1802a821a*/
    *((_WORD *)v20 + 30) = 0; /*0x1802a821d*/
  }
  v6 = a1[37]; /*0x1802a8221*/
  v7 = 0; /*0x1802a8225*/
  v8 = a1[36]; /*0x1802a8227*/
  v9 = 28; /*0x1802a822b*/
  v10 = a1[35]; /*0x1802a8230*/
  v11 = a1 + 42; /*0x1802a8234*/
  v3[638] = v10; /*0x1802a8238*/
  v3[639] = v8; /*0x1802a823f*/
  v3[640] = v6; /*0x1802a8246*/
  v19 = 0; /*0x1802a824d*/
  do /*0x1802a82aa*/
  {
    v12 = v11[1]; /*0x1802a8264*/
    v17 = *(_WORD *)(v11 - 1); /*0x1802a8278*/
    v16 = 0; /*0x1802a827d*/
    v15 = v7; /*0x1802a8282*/
    v18 = v12; /*0x1802a8286*/
    v13 = sub_180353AF0(a1: v12, a2: v6); /*0x1802a828a*/
    result = sub_180351450(a1: v13 + 5536, a2: &v15); /*0x1802a829b*/
    ++v7; /*0x1802a82a0*/
    v11 += 3; /*0x1802a82a2*/
    --v9; /*0x1802a82a6*/
  }
  while ( v9 != 0 ); /*0x1802a82aa*/
  return result; /*0x1802a82ac*/
}
