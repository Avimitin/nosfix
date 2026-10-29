__int64 __fastcall sub_180004870(__int64 a1, __int64 a2, __int64 a3, float a4, float a5, float a6)
{
  int v7; // edx
  int v11; // ecx
  _WORD *v12; // rax
  __int64 v13; // r13
  __int64 v14; // rsi
  char *v15; // rdi
  __int64 v16; // xmm0_8
  __int16 v17; // r12
  __int16 v18; // bx
  __int16 v19; // ax
  __int16 v20; // cx
  __int16 v21; // r14
  int v22; // r14d
  char *v23; // rbx
  __int64 v24; // r12
  __int64 v25; // rcx
  char *v26; // rax
  __int64 *v27; // rbx
  float v28; // xmm0_4
  float v29; // xmm7_4
  float v30; // xmm6_4
  __int16 v31; // ax
  int v32; // eax
  __int64 v33; // rdx
  __int64 v34; // r8
  int v35; // eax
  bool v36; // zf
  __int64 v37; // rax
  int v38; // eax
  int v39; // eax
  __int64 *v40; // rcx
  __int64 v41; // rbx
  unsigned __int16 v42; // ax
  __int16 v43; // [rsp+38h] [rbp-D0h]
  __int16 v44; // [rsp+3Ah] [rbp-CEh]
  __int16 v45; // [rsp+3Ch] [rbp-CCh]
  int v46; // [rsp+40h] [rbp-C8h] BYREF
  __int64 v47; // [rsp+44h] [rbp-C4h]
  __int64 v48; // [rsp+4Ch] [rbp-BCh] BYREF
  __int64 v49; // [rsp+54h] [rbp-B4h]
  __int64 v50; // [rsp+5Ch] [rbp-ACh] BYREF
  __int64 v51; // [rsp+64h] [rbp-A4h]
  __int64 v52; // [rsp+6Ch] [rbp-9Ch] BYREF
  __int64 v53; // [rsp+74h] [rbp-94h]
  _DWORD v54[4]; // [rsp+80h] [rbp-88h] BYREF
  int v55; // [rsp+90h] [rbp-78h] BYREF
  int *v56; // [rsp+94h] [rbp-74h]
  int v57; // [rsp+A0h] [rbp-68h] BYREF
  char *v58; // [rsp+A4h] [rbp-64h]
  int v59; // [rsp+B0h] [rbp-58h] BYREF
  char *v60; // [rsp+B4h] [rbp-54h]
  float v61; // [rsp+C0h] [rbp-48h] BYREF
  __int64 v62; // [rsp+C4h] [rbp-44h]
  float v63; // [rsp+CCh] [rbp-3Ch]

  v7 = 0; /*0x18000489b*/
  if ( a2 == 0 ) /*0x1800048aa*/
    return 2147500037LL; /*0x1800048b1*/
  v11 = 0; /*0x1800048b6*/
  v12 = &unk_18003883A; /*0x1800048b8*/
  while ( *v12 != 0xFFFF ) /*0x1800048d7*/
  {
    if ( v12 == (_WORD *)&unk_180046802 ) /*0x1800048dc*/
      return 2147500037LL; /*0x1800048dc*/
    v12 += 28; /*0x1800048de*/
    ++v11; /*0x1800048e2*/
    if ( (__int64)v12 >= (__int64)&word_18004683A ) /*0x1800048e7*/
      goto LABEL_9; /*0x1800048e7*/
  }
  v7 = v11; /*0x1800048eb*/
LABEL_9:
  v13 = v7; /*0x1800048ed*/
  v14 = 56LL * v7; /*0x180004940*/
  v15 = &byte_180038830[v14]; /*0x180004944*/
  (*(void (__fastcall **)(_QWORD, _QWORD, _QWORD, _QWORD, char *))(**(_QWORD **)(a1 + 8) + 24LL))( /*0x18000494d*/
    a1: *(_QWORD *)(a1 + 8),
    a2: *(unsigned __int16 *)(a2 + 2),
    a3: 0,
    a4: (unsigned int)(int)a6,
    a5: &byte_180038830[v14]);
  if ( *(_QWORD *)&byte_180038830[v14] == 0 ) /*0x180004954*/
    return 2147500037LL; /*0x18000495b*/
  v16 = *(_QWORD *)(a2 + 16); /*0x180004964*/
  *(_OWORD *)&byte_180038830[v14 + 8] = *(_OWORD *)a2; /*0x180004988*/
  *(_QWORD *)&byte_180038830[v14 + 24] = v16; /*0x18000498d*/
  v43 = sub_180001240(a1: &unk_188F5FEC0, a2: "COIN"); /*0x1800049a6*/
  v17 = sub_180001240(a1: &unk_188F5FEC0, a2: "BGM"); /*0x1800049be*/
  v45 = v17; /*0x1800049c2*/
  v18 = sub_180001240(a1: &unk_188F5FEC0, a2: "Default"); /*0x1800049da*/
  v19 = sub_180001240(a1: &unk_188F5FEC0, a2: "Reverb"); /*0x1800049dd*/
  v20 = *(_WORD *)(a2 + 4); /*0x1800049e2*/
  v21 = v19; /*0x1800049e7*/
  v44 = v19; /*0x1800049eb*/
  if ( v20 == v43 || v20 == v18 || v20 == v19 ) /*0x180004a07*/
  {
LABEL_22:
    v27 = *(__int64 **)v15; /*0x180004acd*/
    if ( *(_QWORD *)v15 == 0 ) /*0x180004ad3*/
      return 2147500037LL; /*0x180004ad3*/
    if ( a4 < -10000.0 || a4 > 0.0 ) /*0x180004aff*/
    {
      a4 = 0.0; /*0x180004b7a*/
      *(_DWORD *)&byte_180038830[v14 + 32] = 0; /*0x180004b7e*/
    }
    else
    {
      *(float *)&byte_180038830[v14 + 32] = a4; /*0x180004b08*/
    }
    v28 = powf(X: 10.0, Y: (float)(a4 / 100.0) / 20.0); /*0x180004ba2*/
    if ( a5 < -10000.0 || a5 > 10000.0 ) /*0x180004bc2*/
    {
      v29 = v28; /*0x180004c04*/
    }
    else
    {
      *(float *)&byte_180038830[v14 + 36] = a5; /*0x180004bd0*/
      if ( a5 >= 0.0 ) /*0x180004bd7*/
        v29 = (float)(1.0 - (float)(a5 / 10000.0)) * v28; /*0x180004bec*/
      else
        v29 = v28; /*0x180004bd9*/
      if ( a5 < 0.0 ) /*0x180004bf4*/
      {
        v30 = (float)((float)(a5 / 10000.0) + 1.0) * v28; /*0x180004bfe*/
        goto LABEL_38; /*0x180004c02*/
      }
    }
    v30 = v28; /*0x180004c07*/
LABEL_38:
    v59 = 1; /*0x180004c0a*/
    v49 = qword_188F75890; /*0x180004c1a*/
    HIDWORD(v48) = 0; /*0x180004c24*/
    v60 = (char *)&v48 + 4; /*0x180004c28*/
    HIDWORD(v50) = 0; /*0x180004c33*/
    v51 = qword_188F75820; /*0x180004c37*/
    HIDWORD(v52) = 0; /*0x180004c41*/
    *(_QWORD *)&v54[1] = (char *)&v50 + 4; /*0x180004c45*/
    v46 = 0; /*0x180004c51*/
    v53 = qword_188F75828; /*0x180004c55*/
    v54[0] = 1; /*0x180004c5f*/
    v58 = (char *)&v52 + 4; /*0x180004c67*/
    v57 = 1; /*0x180004c72*/
    v47 = qword_188F75878; /*0x180004c79*/
    v55 = 1; /*0x180004c83*/
    v56 = &v46; /*0x180004c8a*/
    v31 = *(_WORD *)(a2 + 4); /*0x180004c8e*/
    if ( v31 == v43 ) /*0x180004c9b*/
    {
      (*(void (__fastcall **)(__int64 *, _DWORD *))(*v27 + 80))(a1: v27, a2: v54); /*0x180004ca5*/
      v32 = *(_DWORD *)(a2 + 8); /*0x180004ca8*/
      if ( v32 == 1 ) /*0x180004caf*/
      {
        *(float *)&v62 = v30; /*0x180004cb5*/
        v33 = v51; /*0x180004cc3*/
        v34 = 1; /*0x180004cc8*/
LABEL_58:
        v40 = *(__int64 **)v15; /*0x180004ddf*/
        v61 = v29; /*0x180004de2*/
        (*(void (__fastcall **)(__int64 *, __int64, __int64, __int64, float *))(*v40 + 88))( /*0x180004dea*/
          a1: v40,
          a2: v33,
          a3: v34,
          a4: 2,
          a5: &v61);
        goto LABEL_59; /*0x180004dea*/
      }
      if ( v32 == 2 ) /*0x180004cd3*/
      {
        v33 = v51; /*0x180004ce2*/
LABEL_57:
        v63 = v30; /*0x180004dc9*/
        v62 = 0; /*0x180004dd4*/
        v34 = 2; /*0x180004ddc*/
        goto LABEL_58; /*0x180004ddc*/
      }
    }
    else if ( v31 == v21 ) /*0x180004cf0*/
    {
      (*(void (__fastcall **)(__int64 *, int *))(*v27 + 80))(a1: v27, a2: &v55); /*0x180004cf9*/
      v35 = *(_DWORD *)(a2 + 8); /*0x180004cfc*/
      if ( v35 == 1 ) /*0x180004d03*/
      {
        *(float *)&v62 = v30; /*0x180004d09*/
        v33 = v47; /*0x180004d17*/
        v34 = 1; /*0x180004d1c*/
        goto LABEL_58; /*0x180004d1f*/
      }
      if ( v35 == 2 ) /*0x180004d27*/
      {
        v33 = v47; /*0x180004d36*/
        goto LABEL_57; /*0x180004d3b*/
      }
    }
    else
    {
      v36 = v31 == v17; /*0x180004d40*/
      v37 = *v27; /*0x180004d44*/
      if ( v36 ) /*0x180004d47*/
      {
        (*(void (__fastcall **)(__int64 *, int *))(v37 + 80))(a1: v27, a2: &v57); /*0x180004d4d*/
        v38 = *(_DWORD *)(a2 + 8); /*0x180004d50*/
        if ( v38 == 1 ) /*0x180004d57*/
        {
          *(float *)&v62 = v30; /*0x180004d5d*/
          v33 = v53; /*0x180004d6b*/
          v34 = 1; /*0x180004d70*/
          goto LABEL_58; /*0x180004d73*/
        }
        if ( v38 == 2 ) /*0x180004d78*/
        {
          v33 = v53; /*0x180004d83*/
          goto LABEL_57; /*0x180004d88*/
        }
      }
      else
      {
        (*(void (__fastcall **)(__int64 *, int *))(v37 + 80))(a1: v27, a2: &v59); /*0x180004d8e*/
        v39 = *(_DWORD *)(a2 + 8); /*0x180004d91*/
        if ( v39 == 1 ) /*0x180004d98*/
        {
          *(float *)&v62 = v30; /*0x180004d9e*/
          v33 = v49; /*0x180004dac*/
          v34 = 1; /*0x180004db1*/
          goto LABEL_58; /*0x180004db4*/
        }
        if ( v39 == 2 ) /*0x180004db9*/
        {
          v33 = v49; /*0x180004dc4*/
          goto LABEL_57; /*0x180004dc4*/
        }
      }
    }
LABEL_59:
    v41 = **(_QWORD **)v15; /*0x180004e28*/
    v42 = (*(__int64 (__fastcall **)(_QWORD, const char *))(v41 + 40))(a1: *(_QWORD *)v15, a2: "Volume"); /*0x180004e35*/
    (*(void (__fastcall **)(_QWORD, _QWORD))(v41 + 48))(a1: *(_QWORD *)v15, a2: v42); /*0x180004e41*/
    (***(void (__fastcall ****)(_QWORD))v15)(a1: *(_QWORD *)v15); /*0x180004e4a*/
    *(_DWORD *)&byte_180038830[v14 + 44] = sub_180001260(a1: (__int64)&unk_188F5FEC0); /*0x180004e5f*/
    QueryPerformanceCounter(lpPerformanceCount: (LARGE_INTEGER *)((char *)&unk_180038860 + v14)); /*0x180004e66*/
    *(float *)&byte_180038830[v14 + 40] = a6; /*0x180004e6e*/
    return 0; /*0x180004e6c*/
  }
  v22 = 0; /*0x180004a0d*/
  v23 = (char *)&unk_18003883C; /*0x180004a10*/
  v24 = 0; /*0x180004a17*/
  while ( 1 ) /*0x180004a20*/
  {
    if ( v24 == v13 ) /*0x180004a23*/
      goto LABEL_20; /*0x180004a23*/
    v25 = *(_QWORD *)(v23 - 12); /*0x180004a29*/
    if ( v25 == 0 || *(_WORD *)v23 != *(_WORD *)(a2 + 4) ) /*0x180004a3a*/
      goto LABEL_20; /*0x180004a3a*/
    if ( *((_WORD *)v23 + 1) > *(_WORD *)(a2 + 6) ) /*0x180004a4a*/
      break; /*0x180004a4a*/
    (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)v25 + 8LL))(a1: v25, a2: 1); /*0x180004a53*/
    (*(void (__fastcall **)(_QWORD))(**(_QWORD **)(v23 - 12) + 24LL))(a1: *(_QWORD *)(v23 - 12)); /*0x180004a5d*/
    v26 = &byte_180038830[56 * v22]; /*0x180004a6e*/
    *(_QWORD *)v26 = 0; /*0x180004a73*/
    *((_QWORD *)v26 + 1) = 0; /*0x180004a76*/
    *((_QWORD *)v26 + 2) = 0; /*0x180004a7a*/
    *((_QWORD *)v26 + 3) = 0; /*0x180004a7e*/
    *((_QWORD *)v26 + 4) = 0; /*0x180004a82*/
    *((_QWORD *)v26 + 5) = 0; /*0x180004a86*/
    *((_QWORD *)v26 + 6) = 0; /*0x180004a8a*/
    *(_QWORD *)(v23 - 4) = -1; /*0x180004a90*/
    *((_DWORD *)v23 + 3) = -1082130432; /*0x180004a98*/
    *((_DWORD *)v23 + 4) = -1082130432; /*0x180004a9f*/
    *(_QWORD *)(v23 + 4) = 0; /*0x180004aa6*/
LABEL_20:
    ++v22; /*0x180004aaa*/
    ++v24; /*0x180004aad*/
    v23 += 56; /*0x180004ab0*/
    if ( v22 >= 1024 ) /*0x180004abb*/
    {
      v21 = v44; /*0x180004ac1*/
      v17 = v45; /*0x180004ac7*/
      goto LABEL_22; /*0x180004ac7*/
    }
  }
  (*(void (__fastcall **)(_QWORD, __int64))(**(_QWORD **)v15 + 8LL))(a1: *(_QWORD *)v15, a2: 1); /*0x180004b17*/
  (*(void (__fastcall **)(_QWORD))(**(_QWORD **)v15 + 24LL))(a1: *(_QWORD *)v15); /*0x180004b20*/
  *(_QWORD *)v15 = 0; /*0x180004b25*/
  *((_QWORD *)v15 + 1) = 0; /*0x180004b28*/
  *((_QWORD *)v15 + 2) = 0; /*0x180004b2c*/
  *((_QWORD *)v15 + 3) = 0; /*0x180004b30*/
  *((_QWORD *)v15 + 4) = 0; /*0x180004b34*/
  *((_QWORD *)v15 + 5) = 0; /*0x180004b38*/
  *((_QWORD *)v15 + 6) = 0; /*0x180004b3c*/
  *(_QWORD *)&byte_180038830[v14 + 8] = -1; /*0x180004b47*/
  *(_QWORD *)&byte_180038830[v14 + 16] = 0; /*0x180004b50*/
  *(_DWORD *)&byte_180038830[v14 + 24] = -1082130432; /*0x180004b59*/
  *(_DWORD *)&byte_180038830[v14 + 28] = -1082130432; /*0x180004b61*/
  return 2147500037LL; /*0x180004eae*/
}
