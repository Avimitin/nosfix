__int64 __fastcall sub_1801C3B20(__int64 a1)
{
  __int64 v2; // rbx
  _OWORD *v3; // rdx
  int *v4; // rcx
  __int64 i; // rdx
  int v6; // eax
  __int64 v7; // rbx
  unsigned __int8 v8; // al
  __int64 v9; // rax
  __int64 v10; // rcx
  __int64 v11; // rcx
  unsigned __int64 v12; // rdx
  __int64 v13; // rcx
  _QWORD **v14; // r8
  __int64 v15; // r9
  __int64 v16; // r10
  int v17; // esi
  _QWORD *v18; // rbx
  __int64 v19; // rdi
  _QWORD *j; // rdi
  __int64 v21; // rax
  __int64 v22; // rbx
  _OWORD *v23; // rdx
  __int64 v24; // rbx
  __int128 v26; // [rsp+40h] [rbp-68h] BYREF
  __int128 v27; // [rsp+50h] [rbp-58h] BYREF
  LARGE_INTEGER PerformanceCount; // [rsp+60h] [rbp-48h] BYREF

  PerformanceCount.QuadPart = 0; /*0x1801c3b57*/
  v2 = ((__int64 (*)(void))sub_1802C5260)(); /*0x1801c3b5c*/
  QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c3b5f*/
  unknown_libname_4(a1: &v27); /*0x1801c3b6a*/
  *(LARGE_INTEGER *)&v26 = PerformanceCount; /*0x1801c3b7f*/
  v3 = *(_OWORD **)(v2 + 24); /*0x1801c3b8b*/
  *((_QWORD *)&v26 + 1) = "Update"; /*0x1801c3b8f*/
  v27 = xmmword_1812D3E10; /*0x1801c3b94*/
  if ( *(_OWORD **)(v2 + 32) == v3 ) /*0x1801c3b9d*/
  {
    sub_1800C7E20(a1: v2 + 16, a2: v3, a3: &v26); /*0x1801c3bbc*/
  }
  else
  {
    *v3 = v26; /*0x1801c3ba4*/
    v3[1] = v27; /*0x1801c3bac*/
    *(_QWORD *)(v2 + 24) += 32LL; /*0x1801c3bb0*/
  }
  v4 = (int *)&unk_1812CF394; /*0x1801c3bc1*/
  for ( i = 128; i != 0; --i ) /*0x1801c3bc8*/
  {
    v6 = *(v4 - 1); /*0x1801c3bd0*/
    if ( v6 >= 1 ) /*0x1801c3bd6*/
      *(v4 - 1) = v6 + 1; /*0x1801c3bda*/
    if ( *v4 >= 1 ) /*0x1801c3be2*/
      ++*v4; /*0x1801c3be6*/
    v4 += 2; /*0x1801c3be8*/
  }
  if ( byte_1812CF8F9 != 0 ) /*0x1801c3bf9*/
  {
    v4 = (int *)(*(_DWORD *)(qword_1812D1E88 + 16) >> 15); /*0x1801c3c09*/
    if ( (*(_DWORD *)(qword_1812D1E88 + 16) & 0x8000) != 0 ) /*0x1801c3c0f*/
    {
      avs2_core_381(a1: "Service", a2: "Trigger"); /*0x1801c3c1f*/
      v7 = qword_1812D1E80; /*0x1801c3c25*/
      v8 = *(_BYTE *)(qword_1812D1E80 + 50); /*0x1801c3c2c*/
      if ( v8 != 0 && *(int *)(qword_1812D1E80 + 44) <= 0 ) /*0x1801c3c38*/
      {
        avs2_core_381(a1: "Service", a2: "nAddCoinCount"); /*0x1801c3c48*/
        ++*(_DWORD *)(v7 + 28); /*0x1801c3c4e*/
        v8 = *(_BYTE *)(v7 + 50); /*0x1801c3c51*/
        *(_DWORD *)(v7 + 44) = 10; /*0x1801c3c55*/
      }
      avs2_core_381(a1: "Service", a2: "m_bCoinSupplyByService = %d", v8); /*0x1801c3c6e*/
      avs2_core_381(a1: "Service", a2: "m_SreviceCoinChaTimer = %d", *(_DWORD *)(v7 + 44)); /*0x1801c3c86*/
    }
  }
  if ( (dword_184B48678 & 1) == 0 ) /*0x1801c3c94*/
  {
    dword_184B48688 = 0; /*0x1801c3c99*/
    dword_184B48678 |= 1u; /*0x1801c3ca0*/
    byte_184B4868C = 0; /*0x1801c3cb4*/
    qword_184B48680 = (__int64)&ShopClosingUtilitySpace::CShopClosingWatch::`vftable'; /*0x1801c3cbb*/
    qword_184B48690 = 0; /*0x1801c3cc2*/
    atexit(a1: sub_1803A6AC0); /*0x1801c3cc9*/
  }
  if ( (_BYTE)dword_184B48688 != 0 ) /*0x1801c3cd5*/
  {
    v9 = sub_180374980(a1: v4); /*0x1801c3cd7*/
    sub_1803748E0(a1: v9); /*0x1801c3cdf*/
  }
  if ( *(_DWORD *)(sub_1801865B0(a1: v4) + 16) == 0 /*0x1801c3d19*/
    && *(_BYTE *)(sub_1801865B0(a1: v10) + 84) == 0
    && ((int)avs2_ea3_14(a1: 18, a2: 0, a3: &PerformanceCount, a4: 12) < 0 || PerformanceCount.LowPart != 3) )
  {
    *(_BYTE *)(sub_1801865B0(a1: v11) + 84) = 1; /*0x1801c3d20*/
  }
  sub_1801CFD40(a1: qword_1811CEEB8); /*0x1801c3d2b*/
  if ( *(_QWORD *)(a1 + 176) != 0 ) /*0x1801c3d3a*/
    sub_180356BA0(); /*0x1801c3d3c*/
  avs2_core_8(a1: 0); /*0x1801c3d43*/
  sub_18030F0E0(a1: &qword_1813E5A20); /*0x1801c3d50*/
  if ( (dword_1813EE5F4 & 1) == 0 ) /*0x1801c3d5d*/
  {
    qword_1813EE608 = 0; /*0x1801c3d62*/
    dword_1813EE5F4 |= 1u; /*0x1801c3d6c*/
    xmmword_1813EE5F8 = 0; /*0x1801c3d79*/
    atexit(a1: sub_18038E7E0); /*0x1801c3d81*/
  }
  if ( (unsigned __int8)sub_1802BD9D0() != 0 /*0x1801c3dd5*/
    && *(_BYTE *)(sub_1801865B0(a1: v13) + 36) != 0
    && (unsigned __int8)sub_1802BDAB0() == 0
    && byte_1812CF8F9 != 0
    && (v16 = qword_1812D1EA8, v17 = 0, *(int *)(qword_1812D1EA8 + 16584) > 0) )
  {
    v18 = (_QWORD *)xmmword_1813EE5F8; /*0x1801c3ddb*/
    v19 = 0; /*0x1801c3de2*/
    do /*0x1801c3e6f*/
    {
      if ( *(_DWORD *)(v19 + v16 + 76) == 2 ) /*0x1801c3df6*/
      {
        v13 = 0; /*0x1801c3dff*/
        v15 = 0; /*0x1801c3e0f*/
        v12 = (__int64)(*((_QWORD *)&xmmword_1813EE5F8 + 1) - (_QWORD)v18) >> 4; /*0x1801c3e12*/
        if ( v12 != 0 ) /*0x1801c3e19*/
        {
          v14 = (_QWORD **)v18; /*0x1801c3e1b*/
          while ( (int)v13 < 0 || (int)v13 >= (int)v12 || **v14 != 0 ) /*0x1801c3e2e*/
          {
            v13 = (unsigned int)(v13 + 1); /*0x1801c3e30*/
            ++v15; /*0x1801c3e32*/
            v14 += 2; /*0x1801c3e38*/
            if ( (int)v13 >= v12 ) /*0x1801c3e3f*/
              goto LABEL_44; /*0x1801c3e3f*/
          }
          sub_18012C060(a1: v18[2 * v15], a2: *(unsigned int *)(v19 + v16 + 64), a3: *(unsigned int *)(v19 + v16 + 68)); /*0x1801c3e4f*/
          v16 = qword_1812D1EA8; /*0x1801c3e54*/
          v18 = (_QWORD *)xmmword_1813EE5F8; /*0x1801c3e5b*/
        }
      }
LABEL_44:
      ++v17; /*0x1801c3e62*/
      v19 += 16; /*0x1801c3e64*/
    }
    while ( v17 < *(_DWORD *)(v16 + 16584) ); /*0x1801c3e6f*/
  }
  else
  {
    v18 = (_QWORD *)xmmword_1813EE5F8; /*0x1801c3e7f*/
  }
  for ( j = *((_QWORD **)&xmmword_1813EE5F8 + 1); v18 != j; v18 += 2 ) /*0x1801c3e98*/
    sub_18012C240(a1: *v18, a2: v12, a3: v14, a4: v15); /*0x1801c3ea3*/
  v21 = sub_1802C5260(a1: v13, a2: v12, a3: v14, a4: v15); /*0x1801c3eb1*/
  PerformanceCount.QuadPart = 0; /*0x1801c3ebb*/
  v22 = v21; /*0x1801c3ec0*/
  QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c3ec3*/
  unknown_libname_4(a1: &v27); /*0x1801c3ece*/
  v23 = *(_OWORD **)(v22 + 24); /*0x1801c3ee6*/
  *(LARGE_INTEGER *)&v26 = PerformanceCount; /*0x1801c3efa*/
  *((_QWORD *)&v26 + 1) = "Update(NBTRenderer)"; /*0x1801c3f03*/
  v27 = xmmword_1812D3E50; /*0x1801c3f08*/
  if ( *(_OWORD **)(v22 + 32) == v23 ) /*0x1801c3f11*/
  {
    sub_1800C7E20(a1: v22 + 16, a2: v23, a3: &v26); /*0x1801c3f30*/
  }
  else
  {
    *v23 = v26; /*0x1801c3f18*/
    v23[1] = v27; /*0x1801c3f20*/
    *(_QWORD *)(v22 + 24) += 32LL; /*0x1801c3f24*/
  }
  v24 = *(_QWORD *)(sub_1803383C0() + 88); /*0x1801c3f3a*/
  D3DXMatrixLookAtLH(a1: v24 + 44, a2: v24 + 8, a3: v24 + 20, a4: v24 + 32); /*0x1801c3f4e*/
  D3DXMatrixOrthoOffCenterLH(a1: v24 + 108); /*0x1801c3f85*/
  return D3DXMatrixPerspectiveFovLH(a1: v24 + 172); /*0x1801c3fb6*/
}
