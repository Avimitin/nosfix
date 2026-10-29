char __fastcall sub_1801A5990(__int64 a1)
{
  int v1; // eax
  __int64 v3; // rdx
  int v4; // ecx
  int v5; // xmm7_4
  __int64 v6; // rcx
  __int64 v7; // rax
  int v8; // edx
  __int64 v9; // rcx
  _QWORD *v10; // rbx
  __int64 i; // rsi
  int v12; // edx
  int v13; // r8d
  int v14; // r9d
  int v15; // esi
  int v16; // ebx
  int v17; // eax
  __m128 v18; // xmm1
  int v19; // ecx
  __int128 *v20; // rax
  __int128 v21; // xmm0
  int v22; // edx
  int v23; // r8d
  int v24; // r9d
  int v25; // ecx
  __int128 *v26; // rax
  __int128 v27; // xmm0
  int v28; // edx
  int v29; // r8d
  int v30; // r9d
  int v31; // ecx
  __int128 *v32; // rax
  __int128 v33; // xmm0
  __m128i v34; // xmm0
  int v35; // ecx
  int v36; // edx
  int v37; // r8d
  int v38; // r9d
  _OWORD *v39; // rax
  unsigned int v40; // edx
  __int64 v41; // rbx
  __int64 v42; // rax
  int v44; // [rsp+68h] [rbp-A0h] BYREF
  int v45; // [rsp+6Ch] [rbp-9Ch] BYREF
  float v46; // [rsp+70h] [rbp-98h] BYREF
  int v47; // [rsp+78h] [rbp-90h] BYREF
  int v48; // [rsp+80h] [rbp-88h]
  float v49; // [rsp+88h] [rbp-80h] BYREF
  unsigned __int64 v50; // [rsp+90h] [rbp-78h] BYREF
  int v51; // [rsp+98h] [rbp-70h]
  unsigned __int64 v52; // [rsp+9Ch] [rbp-6Ch]
  int v53; // [rsp+A4h] [rbp-64h]
  unsigned __int64 v54; // [rsp+A8h] [rbp-60h]
  int v55; // [rsp+B0h] [rbp-58h]
  unsigned __int64 v56; // [rsp+B4h] [rbp-54h]
  int v57; // [rsp+BCh] [rbp-4Ch]
  _QWORD v58[8]; // [rsp+C8h] [rbp-40h] BYREF

  v1 = *(_DWORD *)(a1 + 104); /*0x1801a59bd*/
  if ( v1 > 0 ) /*0x1801a59c5*/
    *(_DWORD *)(a1 + 104) = v1 - 1; /*0x1801a59c9*/
  v3 = (unsigned int)dword_18070A610; /*0x1801a59d1*/
  if ( dword_1813668E0 >= *(_DWORD *)(a1 + 24) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908) ) /*0x1801a59f3*/
  {
    *(_BYTE *)(a1 + 17) = 1; /*0x1801a59f5*/
    v3 = (unsigned int)dword_18070A610; /*0x1801a5a07*/
  }
  v4 = *(_DWORD *)(a1 + 20); /*0x1801a5a45*/
  v44 = 0; /*0x1801a5a54*/
  v45 = 0; /*0x1801a5a5c*/
  v46 = 0.0; /*0x1801a5a64*/
  sub_180305430( /*0x1801a5a9e*/
    a1: v4,
    a2: v3,
    a3: (int *)(a1 + 28),
    a4: (int *)(a1 + 32),
    a5: &v46,
    a6: &v49,
    a7: (float *)&v44,
    a8: (float *)&v45,
    a9: -1,
    a10: -1,
    a11: nullptr,
    a12: nullptr);
  v5 = v45; /*0x1801a5aa9*/
  *(_DWORD *)(a1 + 28) = 640; /*0x1801a5aaf*/
  v7 = sub_18018B090(a1: v6); /*0x1801a5ab6*/
  v8 = *(_DWORD *)(a1 + 32); /*0x1801a5abb*/
  v9 = (unsigned int)(720 - *(_DWORD *)(v7 + 60)); /*0x1801a5ac3*/
  if ( v8 > (int)v9 ) /*0x1801a5ac8*/
  {
    if ( *(_BYTE *)(a1 + 208) != 0 ) /*0x1801a5ad1*/
    {
      v5 = 1061997773; /*0x1801a5aed*/
    }
    else
    {
      v8 = 720 - *(_DWORD *)(sub_18018B090(a1: v9) + 60); /*0x1801a5add*/
      *(_DWORD *)(a1 + 32) = v8; /*0x1801a5ae0*/
    }
  }
  *(_DWORD *)(a1 + 36) = *(_DWORD *)(a1 + 28); /*0x1801a5af8*/
  *(_DWORD *)(a1 + 40) = v8; /*0x1801a5afb*/
  if ( BYTE2(dword_1813E473C) != 0 ) /*0x1801a5b05*/
  {
    v10 = v58; /*0x1801a5b13*/
    for ( i = 4; i != 0; --i ) /*0x1801a5b1f*/
    {
      unknown_libname_4(a1: v10); /*0x1801a5b27*/
      v10 += 2; /*0x1801a5b2c*/
    }
    v15 = (int)(float)((float)(*(_DWORD *)(a1 + 196) >> 1) * v46); /*0x1801a5b54*/
    v16 = (int)fmaxf((float)(*(_DWORD *)(a1 + 200) >> 1) * v46, 3.0); /*0x1801a5b6e*/
    if ( v15 > 0 && v16 > 0 ) /*0x1801a5b7c*/
    {
      v17 = *(_DWORD *)(a1 + 28) - v15; /*0x1801a5b8a*/
      v48 = 0; /*0x1801a5b8c*/
      v18 = (__m128)COERCE_UNSIGNED_INT((float)(*(_DWORD *)(a1 + 32) - v16)); /*0x1801a5bb4*/
      v51 = 0; /*0x1801a5bb7*/
      v50 = _mm_unpacklo_ps((__m128)COERCE_UNSIGNED_INT((float)v17), v18).m128_u64[0]; /*0x1801a5bc0*/
      sub_1800D23B0(a1: (unsigned int)&v47, a2: v12, a3: v13, a4: v14, a5: v5); /*0x1801a5bc5*/
      v19 = *(_DWORD *)(a1 + 28) - 1; /*0x1801a5bd0*/
      v21 = *v20; /*0x1801a5bdd*/
      LODWORD(v20) = *(_DWORD *)(a1 + 32) - v16; /*0x1801a5be3*/
      v48 = 0; /*0x1801a5be5*/
      *(_OWORD *)v58 = v21; /*0x1801a5bed*/
      v53 = 0; /*0x1801a5c08*/
      v52 = _mm_unpacklo_ps( /*0x1801a5c11*/
              (__m128)COERCE_UNSIGNED_INT((float)(v15 + v19)),
              (__m128)COERCE_UNSIGNED_INT((float)(int)v20)).m128_u64[0];
      sub_1800D23B0(a1: (unsigned int)&v47, a2: v22, a3: v23, a4: v24, a5: v5); /*0x1801a5c16*/
      v25 = *(_DWORD *)(a1 + 32) - 1; /*0x1801a5c21*/
      v27 = *v26; /*0x1801a5c2e*/
      LODWORD(v26) = *(_DWORD *)(a1 + 28) - v15; /*0x1801a5c34*/
      v48 = 0; /*0x1801a5c36*/
      *(_OWORD *)&v58[2] = v27; /*0x1801a5c3e*/
      v55 = 0; /*0x1801a5c59*/
      v54 = _mm_unpacklo_ps( /*0x1801a5c62*/
              (__m128)COERCE_UNSIGNED_INT((float)(int)v26),
              (__m128)COERCE_UNSIGNED_INT((float)(v16 + v25))).m128_u64[0];
      sub_1800D23B0(a1: (unsigned int)&v47, a2: v28, a3: v29, a4: v30, a5: v5); /*0x1801a5c67*/
      v31 = *(_DWORD *)(a1 + 28) - 1; /*0x1801a5c6f*/
      v33 = *v32; /*0x1801a5c79*/
      v48 = 0; /*0x1801a5c7c*/
      *(_OWORD *)&v58[4] = v33; /*0x1801a5c88*/
      v34 = _mm_cvtsi32_si128(v15 + v31); /*0x1801a5c8c*/
      v35 = *(_DWORD *)(a1 + 32) - 1; /*0x1801a5c93*/
      v57 = 0; /*0x1801a5c95*/
      v56 = _mm_unpacklo_ps(_mm_cvtepi32_ps(v34), (__m128)COERCE_UNSIGNED_INT((float)(v16 + v35))).m128_u64[0]; /*0x1801a5ca7*/
      sub_1800D23B0(a1: (unsigned int)&v47, a2: v36, a3: v37, a4: v38, a5: v5); /*0x1801a5cba*/
      *(_OWORD *)&v58[6] = *v39; /*0x1801a5cc2*/
    }
    v40 = *(_DWORD *)(a1 + 204); /*0x1801a5cc6*/
    if ( v40 == 1 ) /*0x1801a5ccf*/
    {
      v41 = *(int *)(a1 + 168); /*0x1801a5cd1*/
      v42 = sub_180339800(); /*0x1801a5cd8*/
      *(_DWORD *)(a1 + 204) = sub_1800D2820( /*0x1801a5d09*/
                                a1: a1 + 128,
                                a2: *(_QWORD *)(v42 + 8 * v41),
                                a3: 1342177280,
                                a4: &v50,
                                a5: v58,
                                a6: nullptr);
      (*(void (__fastcall **)(__int64))(*(_QWORD *)(a1 + 128) + 32LL))(a1: a1 + 128); /*0x1801a5d1d*/
    }
    else
    {
      sub_1800D3060(a1: a1 + 128, a2: v40, a3: (__int64)&v50); /*0x1801a5d2d*/
      sub_1800D2EC0(a1: a1 + 128, a2: *(unsigned int *)(a1 + 204), a3: v58); /*0x1801a5d43*/
    }
  }
  return 0; /*0x1801a5d5a*/
}
