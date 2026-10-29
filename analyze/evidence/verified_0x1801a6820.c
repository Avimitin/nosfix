char __fastcall sub_1801A6820(__int64 *a1)
{
  int v1; // eax
  int v3; // r14d
  int v4; // r15d
  int v5; // edx
  int v6; // ecx
  unsigned int v7; // r12d
  __int64 v8; // rbx
  int v9; // eax
  int v10; // ecx
  __int64 v11; // rax
  int v12; // eax
  __int64 v13; // rcx
  float v14; // xmm10_4
  char v15; // al
  float v16; // xmm7_4
  _QWORD *v17; // rdi
  __int64 v18; // rcx
  int v19; // r12d
  int v20; // edx
  int v21; // edi
  int v22; // r8d
  int v23; // r9d
  int v24; // eax
  __int128 *v25; // rax
  __int128 v26; // xmm0
  int v27; // edx
  int v28; // r8d
  int v29; // r9d
  int v30; // ecx
  __int128 *v31; // rax
  __int128 v32; // xmm0
  int v33; // edx
  int v34; // r8d
  int v35; // r9d
  int v36; // ecx
  _OWORD *v37; // rax
  int v38; // edx
  int v39; // r8d
  int v40; // r9d
  _OWORD *v41; // rax
  __int64 v42; // rbx
  __int64 v43; // rax
  int v44; // edi
  int v45; // edx
  __int64 v46; // rax
  __int64 v47; // rdx
  _DWORD *v48; // rbx
  int v49; // ecx
  int v50; // r8d
  int v51; // edi
  float v52; // ebx
  int v53; // ecx
  int v54; // kr04_4
  float v55; // r10d
  __int64 v56; // rcx
  int v57; // r9d
  bool v58; // zf
  int v59; // xmm0_4
  int v60; // xmm4_4
  int v61; // xmm5_4
  int v62; // xmm2_4
  int v63; // xmm6_4
  int v64; // xmm7_4
  int v65; // xmm8_4
  int v66; // xmm3_4
  float v67; // xmm2_4
  __int64 v68; // rax
  float v69; // xmm3_4
  char v70; // al
  _DWORD *v71; // r12
  __int64 v72; // rcx
  int v73; // kr08_4
  int v74; // ebx
  __int64 v75; // rcx
  int v76; // edi
  int v77; // xmm0_4
  int v78; // xmm4_4
  int v79; // xmm5_4
  int v80; // xmm2_4
  int v81; // xmm6_4
  int v82; // xmm7_4
  int v83; // xmm8_4
  int v84; // xmm3_4
  float v85; // xmm2_4
  __int64 v86; // rax
  float v87; // xmm3_4
  char v88; // al
  float v90; // [rsp+70h] [rbp-90h] BYREF
  float v91; // [rsp+74h] [rbp-8Ch] BYREF
  float v92; // [rsp+78h] [rbp-88h] BYREF
  int v93; // [rsp+7Ch] [rbp-84h] BYREF
  float v94; // [rsp+80h] [rbp-80h] BYREF
  float v95; // [rsp+88h] [rbp-78h] BYREF
  float v96; // [rsp+8Ch] [rbp-74h]
  float v97; // [rsp+90h] [rbp-70h]
  float v98; // [rsp+94h] [rbp-6Ch]
  float v99; // [rsp+98h] [rbp-68h] BYREF
  float v100; // [rsp+9Ch] [rbp-64h]
  float v101; // [rsp+A0h] [rbp-60h]
  float v102; // [rsp+A4h] [rbp-5Ch]
  float v103; // [rsp+A8h] [rbp-58h] BYREF
  float v104; // [rsp+ACh] [rbp-54h] BYREF
  int v105; // [rsp+B0h] [rbp-50h]
  unsigned __int64 v106; // [rsp+B8h] [rbp-48h] BYREF
  int v107; // [rsp+C0h] [rbp-40h]
  unsigned __int64 v108; // [rsp+C4h] [rbp-3Ch]
  int v109; // [rsp+CCh] [rbp-34h]
  unsigned __int64 v110; // [rsp+D0h] [rbp-30h]
  int v111; // [rsp+D8h] [rbp-28h]
  unsigned __int64 v112; // [rsp+DCh] [rbp-24h]
  int v113; // [rsp+E4h] [rbp-1Ch]
  _QWORD v114[8]; // [rsp+F0h] [rbp-10h] BYREF

  v1 = *((_DWORD *)a1 + 26); /*0x1801a684a*/
  if ( v1 > 0 ) /*0x1801a6852*/
    *((_DWORD *)a1 + 26) = v1 - 1; /*0x1801a6856*/
  v3 = -2; /*0x1801a6864*/
  v4 = 0; /*0x1801a686d*/
  v5 = *(_DWORD *)(sub_18018B090(a1: *((unsigned int *)a1 + 16)) + 88); /*0x1801a6870*/
  if ( v6 < 4 ) /*0x1801a6873*/
    v3 = 0; /*0x1801a6873*/
  v7 = 2; /*0x1801a687a*/
  if ( v6 == 3 ) /*0x1801a6883*/
    v3 = -1; /*0x1801a6883*/
  if ( v5 > 0 ) /*0x1801a6889*/
    v5 = 0; /*0x1801a6889*/
  v8 = 4; /*0x1801a6890*/
  if ( v5 > v3 ) /*0x1801a6893*/
    v3 = v5; /*0x1801a6893*/
  if ( v3 < 0 ) /*0x1801a689a*/
  {
    v9 = *((_DWORD *)a1 + 16); /*0x1801a689c*/
    if ( v9 > 4 && v3 + v9 <= 4 ) /*0x1801a68a8*/
      v7 = 4; /*0x1801a68a8*/
  }
  if ( (unsigned int)(*((_DWORD *)a1 + 16) % 2) <= 1 && *((int *)a1 + 16) >= 3 && v3 == -1 ) /*0x1801a68d3*/
    v4 = dword_1813E476C / -2; /*0x1801a68de*/
  v10 = *((_DWORD *)a1 + 6) - dword_18070A610; /*0x1801a68ff*/
  v105 = v3 * dword_1813E476C / 2; /*0x1801a6902*/
  if ( dword_1813668E0 >= v10 - HIDWORD(qword_181366908) ) /*0x1801a690e*/
    *((_BYTE *)a1 + 17) = 1; /*0x1801a6910*/
  if ( *(_DWORD *)(a1[1] + 48) != 2 ) /*0x1801a692a*/
  {
    v92 = 0.0; /*0x1801a696d*/
    v90 = 0.0; /*0x1801a6987*/
    v11 = *a1; /*0x1801a6993*/
    v91 = 0.0; /*0x1801a6996*/
    if ( (void **)v11 == &GlissandoNotes::`vftable' ) /*0x1801a69a8*/
    {
      v93 = *((_DWORD *)a1 + 12); /*0x1801a69b1*/
    }
    else
    {
      v93 = (*(__int64 (__fastcall **)(__int64 *))(v11 + 72))(a1); /*0x1801a7390*/
      v11 = *a1; /*0x1801a7394*/
    }
    if ( (void **)v11 == &GlissandoNotes::`vftable' ) /*0x1801a69b8*/
      v12 = *((_DWORD *)a1 + 11); /*0x1801a69be*/
    else
      v12 = (*(__int64 (__fastcall **)(__int64 *))(v11 + 64))(a1); /*0x1801a739f*/
    sub_180305430( /*0x1801a6a12*/
      a1: *((_DWORD *)a1 + 5),
      a2: 0,
      a3: (int *)a1 + 7,
      a4: (int *)a1 + 8,
      a5: nullptr,
      a6: &v92,
      a7: &v90,
      a8: &v91,
      a9: v12,
      a10: v93,
      a11: nullptr,
      a12: nullptr);
    v13 = a1[1]; /*0x1801a6a1a*/
    v14 = v92; /*0x1801a6a1e*/
    *((_DWORD *)a1 + 9) = *((_DWORD *)a1 + 7); /*0x1801a6a2e*/
    *((_DWORD *)a1 + 10) = *((_DWORD *)a1 + 8); /*0x1801a6a34*/
    if ( *(_DWORD *)(v13 + 48) != 2 && BYTE1(dword_1813E473C) != 0 ) /*0x1801a6a48*/
    {
      if ( (void **)*a1 == &GlissandoNotes::`vftable' ) /*0x1801a6a5b*/
        v15 = *(_BYTE *)(v13 + 97); /*0x1801a6a61*/
      else
        v15 = (*(__int64 (__fastcall **)(__int64 *))(*a1 + 192))(a1); /*0x1801a73aa*/
      v16 = v91; /*0x1801a6a65*/
      if ( v15 != 0 ) /*0x1801a6a6d*/
        v16 = v91 * 0.25; /*0x1801a6a6f*/
      v17 = v114; /*0x1801a6a74*/
      do /*0x1801a6a88*/
      {
        unknown_libname_4(a1: v17); /*0x1801a6a7b*/
        v17 += 2; /*0x1801a6a80*/
        --v8; /*0x1801a6a84*/
      }
      while ( v8 != 0 ); /*0x1801a6a88*/
      v18 = *((_DWORD *)a1 + 49) / v7; /*0x1801a6a98*/
      v19 = (int)(float)((float)(int)v18 * v14); /*0x1801a6aa4*/
      v21 = (int)(float)((float)((float)(*((_DWORD *)a1 + 50) >> 1) * v14) /*0x1801a6ad7*/
                       * (float)((float)*(int *)(sub_18018B090(a1: v18) + 84) / 10.0));
      if ( v19 > 0 && v21 > 0 ) /*0x1801a6ae6*/
      {
        v97 = 0.0; /*0x1801a6b02*/
        v24 = *((_DWORD *)a1 + 7) + v4 - v19; /*0x1801a6b05*/
        v107 = 0; /*0x1801a6b0e*/
        v106 = _mm_unpacklo_ps( /*0x1801a6b2c*/
                 (__m128)COERCE_UNSIGNED_INT((float)v24),
                 (__m128)COERCE_UNSIGNED_INT((float)(*((_DWORD *)a1 + 8) - v21))).m128_u64[0];
        sub_1800D23B0(a1: (unsigned int)&v99, a2: v20, a3: v22, a4: v23, a5: LODWORD(v16)); /*0x1801a6b31*/
        v97 = 0.0; /*0x1801a6b43*/
        v26 = *v25; /*0x1801a6b49*/
        LODWORD(v25) = *((_DWORD *)a1 + 7) - 1; /*0x1801a6b4f*/
        v109 = 0; /*0x1801a6b51*/
        *(_OWORD *)v114 = v26; /*0x1801a6b56*/
        v108 = _mm_unpacklo_ps( /*0x1801a6b79*/
                 (__m128)COERCE_UNSIGNED_INT((float)((int)v25 + v19 + v4)),
                 (__m128)COERCE_UNSIGNED_INT((float)(*((_DWORD *)a1 + 8) - v21))).m128_u64[0];
        sub_1800D23B0(a1: (unsigned int)&v99, a2: v27, a3: v28, a4: v29, a5: LODWORD(v16)); /*0x1801a6b7e*/
        v30 = *((_DWORD *)a1 + 8) - 1; /*0x1801a6b89*/
        v97 = 0.0; /*0x1801a6b93*/
        v32 = *v31; /*0x1801a6b96*/
        v111 = 0; /*0x1801a6b9c*/
        LODWORD(v31) = *((_DWORD *)a1 + 7) + v4 - v19; /*0x1801a6ba5*/
        *(_OWORD *)&v114[2] = v32; /*0x1801a6ba8*/
        v110 = _mm_unpacklo_ps( /*0x1801a6bc6*/
                 (__m128)COERCE_UNSIGNED_INT((float)(int)v31),
                 (__m128)COERCE_UNSIGNED_INT((float)(v21 + v30))).m128_u64[0];
        sub_1800D23B0(a1: (unsigned int)&v99, a2: v33, a3: v34, a4: v35, a5: LODWORD(v16)); /*0x1801a6bcb*/
        v97 = 0.0; /*0x1801a6bd4*/
        v36 = *((_DWORD *)a1 + 7) - 1 + v19 + v4; /*0x1801a6bdf*/
        *(_OWORD *)&v114[4] = *v37; /*0x1801a6be1*/
        v112 = _mm_unpacklo_ps( /*0x1801a6c04*/
                 (__m128)COERCE_UNSIGNED_INT((float)v36),
                 (__m128)COERCE_UNSIGNED_INT((float)(v21 + *((_DWORD *)a1 + 8) - 1))).m128_u64[0];
        v113 = 0; /*0x1801a6c14*/
        sub_1800D23B0(a1: (unsigned int)&v99, a2: v38, a3: v39, a4: v40, a5: LODWORD(v16)); /*0x1801a6c1a*/
        *(_OWORD *)&v114[6] = *v41; /*0x1801a6c22*/
      }
      if ( *((_DWORD *)a1 + 51) == 1 ) /*0x1801a6c2d*/
      {
        v42 = *((int *)a1 + 42); /*0x1801a6c2f*/
        v43 = sub_180339800(); /*0x1801a6c36*/
        *((_DWORD *)a1 + 51) = sub_1800D2820( /*0x1801a6c67*/
                                 a1: (__int64)(a1 + 16),
                                 a2: *(_QWORD *)(v43 + 8 * v42),
                                 a3: 1342177280,
                                 a4: &v106,
                                 a5: v114,
                                 a6: nullptr);
        (*(void (__fastcall **)(__int64 *))(a1[16] + 32))(a1: a1 + 16); /*0x1801a6c7b*/
      }
      else
      {
        v44 = 60; /*0x1801a6c8c*/
        if ( *(_DWORD *)(a1[1] + 48) != 1 ) /*0x1801a6c95*/
          v44 = 70; /*0x1801a6c95*/
        *((_DWORD *)a1 + 42) = v44; /*0x1801a6c98*/
        v45 = v3 + v44 + *((_DWORD *)a1 + 16); /*0x1801a6ca3*/
        if ( v44 + 9 < v45 ) /*0x1801a6cab*/
          v45 = v44 + 9; /*0x1801a6cab*/
        if ( v45 > v44 ) /*0x1801a6cb0*/
          v44 = v45; /*0x1801a6cb0*/
        *((_DWORD *)a1 + 42) = v44; /*0x1801a6cb3*/
        v46 = sub_180339800(); /*0x1801a6cb9*/
        sub_1800D30C0(a1: a1 + 16, a2: *((unsigned int *)a1 + 51), a3: *(_QWORD *)(v46 + 8LL * v44)); /*0x1801a6cd2*/
        sub_1800D3060(a1: (__int64)(a1 + 16), a2: *((_DWORD *)a1 + 51), a3: (__int64)&v106); /*0x1801a6ce8*/
        sub_1800D2EC0(a1: a1 + 16, a2: *((unsigned int *)a1 + 51), a3: v114); /*0x1801a6cfe*/
      }
    }
    (*(void (__fastcall **)(__int64 *))(a1[27] + 24))(a1: a1 + 27); /*0x1801a6d19*/
    v48 = *(_DWORD **)(a1[1] + 64); /*0x1801a6d20*/
    if ( v48 != nullptr && BYTE1(dword_1813E473C) != 0 ) /*0x1801a6d34*/
    {
      v49 = v48[4]; /*0x1801a6d61*/
      v91 = 0.0; /*0x1801a6d6e*/
      v93 = 0; /*0x1801a6d76*/
      v94 = 0.0; /*0x1801a6d7e*/
      v103 = 0.0; /*0x1801a6d85*/
      v104 = 0.0; /*0x1801a6d8c*/
      sub_180305430( /*0x1801a6de4*/
        a1: v49,
        a2: v47,
        a3: (int *)&v92,
        a4: (int *)&v90,
        a5: nullptr,
        a6: &v94,
        a7: &v103,
        a8: &v104,
        a9: v48[22],
        a10: v48[23],
        a11: &v91,
        a12: &v93);
      v50 = v48[20]; /*0x1801a6de9*/
      if ( v50 <= 0 ) /*0x1801a6df0*/
      {
        v51 = LODWORD(v90); /*0x1801a6e2f*/
        v52 = v92; /*0x1801a6e33*/
      }
      else
      {
        v51 = *((_DWORD *)a1 + 8); /*0x1801a6df2*/
        if ( v50 >= v51 ) /*0x1801a6dfc*/
        {
          v53 = LODWORD(v90) - v51; /*0x1801a6e12*/
          if ( LODWORD(v90) == v51 ) /*0x1801a6e14*/
          {
            v51 = v48[20]; /*0x1801a6e18*/
            v52 = v92; /*0x1801a6e1b*/
          }
          else
          {
            v51 = v48[20]; /*0x1801a6e22*/
            LODWORD(v52) = LODWORD(v92) - (LODWORD(v92) - *((_DWORD *)a1 + 7)) * (LODWORD(v90) - v50) / v53; /*0x1801a6e2b*/
          }
        }
        else
        {
          v52 = *((float *)a1 + 7); /*0x1801a6dfe*/
        }
      }
      v54 = *((_DWORD *)a1 + 52); /*0x1801a6e52*/
      v95 = *(float *)&dword_1812D32B8; /*0x1801a6e64*/
      v96 = *(float *)&dword_1812D32BC; /*0x1801a6e72*/
      v97 = *(float *)&dword_1812D32C0; /*0x1801a6e88*/
      v99 = *(float *)&dword_1812D32B8; /*0x1801a6e92*/
      v100 = *(float *)&dword_1812D32BC; /*0x1801a6e97*/
      LODWORD(v55) = (int)(float)((float)(v54 / 2 + v105 - 6) * v14); /*0x1801a6e9c*/
      v56 = a1[1]; /*0x1801a6ea5*/
      v90 = v55; /*0x1801a6eac*/
      v101 = *(float *)&dword_1812D32C0; /*0x1801a6eb1*/
      v57 = (int)(float)((float)((v93 - LODWORD(v91)) / 2 + v105 - 6) * v94); /*0x1801a6ebb*/
      v98 = *(float *)&dword_1812D32C4; /*0x1801a6ec8*/
      v102 = *(float *)&dword_1812D32C4; /*0x1801a6ecd*/
      v58 = *(_DWORD *)(v56 + 48) == 1; /*0x1801a6ed2*/
      v94 = *(float *)&v57; /*0x1801a6ed6*/
      if ( v58 ) /*0x1801a6eda*/
      {
        v59 = dword_1812D3298; /*0x1801a6edc*/
        v60 = dword_1812D329C; /*0x1801a6ee4*/
        v61 = dword_1812D32A0; /*0x1801a6eec*/
        v62 = dword_1812D32A4; /*0x1801a6ef4*/
        v63 = dword_1812D32A8; /*0x1801a6efc*/
        v64 = dword_1812D32AC; /*0x1801a6f04*/
        v65 = dword_1812D32B0; /*0x1801a6f0c*/
        v66 = dword_1812D32B4; /*0x1801a6f15*/
      }
      else
      {
        v59 = dword_1812D3278; /*0x1801a6f1f*/
        v60 = dword_1812D327C; /*0x1801a6f27*/
        v61 = dword_1812D3280; /*0x1801a6f2f*/
        v62 = dword_1812D3284; /*0x1801a6f37*/
        v63 = dword_1812D3288; /*0x1801a6f3f*/
        v64 = dword_1812D328C; /*0x1801a6f47*/
        v65 = dword_1812D3290; /*0x1801a6f4f*/
        v66 = dword_1812D3294; /*0x1801a6f58*/
      }
      v67 = *(float *)&v62 * v104; /*0x1801a6f6c*/
      v68 = *a1; /*0x1801a6f71*/
      v69 = *(float *)&v66 * v104; /*0x1801a6f74*/
      v95 = *(float *)&v59 * v103; /*0x1801a6f92*/
      v96 = *(float *)&v60 * v103; /*0x1801a6f97*/
      v97 = *(float *)&v61 * v103; /*0x1801a6f9c*/
      v99 = *(float *)&v63 * v103; /*0x1801a6fa1*/
      v100 = *(float *)&v64 * v103; /*0x1801a6fa6*/
      v101 = *(float *)&v65 * v103; /*0x1801a6fab*/
      v98 = v67; /*0x1801a6fb1*/
      v102 = v69; /*0x1801a6fb6*/
      if ( (void **)v68 == &GlissandoNotes::`vftable' ) /*0x1801a6fbe*/
      {
        v70 = *(_BYTE *)(v56 + 97); /*0x1801a6fc4*/
      }
      else
      {
        v70 = (*(__int64 (__fastcall **)(__int64 *))(v68 + 192))(a1); /*0x1801a73b8*/
        v67 = v98; /*0x1801a73be*/
        v69 = v102; /*0x1801a73c3*/
        v55 = v90; /*0x1801a73c8*/
        *(float *)&v57 = v94; /*0x1801a73cd*/
      }
      if ( v70 != 0 ) /*0x1801a6fca*/
      {
        v98 = v67 * 0.25; /*0x1801a6fd6*/
        v102 = v69 * 0.25; /*0x1801a6fdb*/
      }
      v71 = (_DWORD *)a1 + 7; /*0x1801a6feb*/
      sub_1800D5370( /*0x1801a7043*/
        a1: (_DWORD *)a1 + 54,
        a2: v4 + *((_DWORD *)a1 + 7) - LODWORD(v55),
        a3: *((_DWORD *)a1 + 8),
        a4: v4 + *((_DWORD *)a1 + 7) + LODWORD(v55),
        a5: *((_DWORD *)a1 + 8),
        a6: v4 + v57 + LODWORD(v52),
        a7: v51,
        a8: v4 + LODWORD(v52) - v57,
        a9: v51,
        a10: (__int64)&v99,
        a11: (__int64)&v95,
        a12: (__int64)&v99,
        a13: (__int64)&v95);
    }
    else
    {
      v71 = (_DWORD *)a1 + 7; /*0x1801a704a*/
    }
    v72 = *(_QWORD *)(a1[1] + 72); /*0x1801a7052*/
    if ( v72 != 0 && BYTE1(dword_1813E473C) != 0 && *(_BYTE *)(v72 + 32) == 0 ) /*0x1801a7070*/
    {
      v103 = 0.0; /*0x1801a70a9*/
      v104 = 0.0; /*0x1801a70ac*/
      v94 = 0.0; /*0x1801a70af*/
      v90 = 0.0; /*0x1801a70b2*/
      v92 = 0.0; /*0x1801a70b6*/
      sub_180305430( /*0x1801a710a*/
        a1: *(_DWORD *)(v72 + 16),
        a2: 0,
        a3: &v93,
        a4: (int *)&v91,
        a5: nullptr,
        a6: &v94,
        a7: &v90,
        a8: &v92,
        a9: *(_DWORD *)(v72 + 88),
        a10: *(_DWORD *)(v72 + 92),
        a11: &v103,
        a12: &v104);
      v73 = *((_DWORD *)a1 + 52); /*0x1801a712a*/
      v99 = *(float *)&dword_1812D32B8; /*0x1801a713c*/
      v100 = *(float *)&dword_1812D32BC; /*0x1801a7148*/
      v101 = *(float *)&dword_1812D32C0; /*0x1801a715e*/
      v95 = *(float *)&dword_1812D32B8; /*0x1801a7168*/
      v96 = *(float *)&dword_1812D32BC; /*0x1801a716d*/
      v74 = (int)(float)((float)(v73 / 2 + v105 - 6) * v14); /*0x1801a7172*/
      v75 = a1[1]; /*0x1801a717a*/
      v97 = *(float *)&dword_1812D32C0; /*0x1801a7181*/
      v76 = (int)(float)((float)((LODWORD(v104) - LODWORD(v103)) / 2 + v105 - 6) * v94); /*0x1801a718b*/
      v102 = *(float *)&dword_1812D32C4; /*0x1801a7197*/
      v98 = *(float *)&dword_1812D32C4; /*0x1801a719c*/
      if ( *(_DWORD *)(v75 + 48) == 1 ) /*0x1801a71a5*/
      {
        v77 = dword_1812D3298; /*0x1801a71a7*/
        v78 = dword_1812D329C; /*0x1801a71af*/
        v79 = dword_1812D32A0; /*0x1801a71b7*/
        v80 = dword_1812D32A4; /*0x1801a71bf*/
        v81 = dword_1812D32A8; /*0x1801a71c7*/
        v82 = dword_1812D32AC; /*0x1801a71cf*/
        v83 = dword_1812D32B0; /*0x1801a71d7*/
        v84 = dword_1812D32B4; /*0x1801a71e0*/
      }
      else
      {
        v77 = dword_1812D3278; /*0x1801a71ea*/
        v78 = dword_1812D327C; /*0x1801a71f2*/
        v79 = dword_1812D3280; /*0x1801a71fa*/
        v80 = dword_1812D3284; /*0x1801a7202*/
        v81 = dword_1812D3288; /*0x1801a720a*/
        v82 = dword_1812D328C; /*0x1801a7212*/
        v83 = dword_1812D3290; /*0x1801a721a*/
        v84 = dword_1812D3294; /*0x1801a7223*/
      }
      v85 = *(float *)&v80 * v92; /*0x1801a7238*/
      v86 = *a1; /*0x1801a723e*/
      v87 = *(float *)&v84 * v92; /*0x1801a7241*/
      v99 = *(float *)&v77 * v90; /*0x1801a7260*/
      v100 = *(float *)&v78 * v90; /*0x1801a7265*/
      v101 = *(float *)&v79 * v90; /*0x1801a726a*/
      v95 = *(float *)&v81 * v90; /*0x1801a726f*/
      v96 = *(float *)&v82 * v90; /*0x1801a7274*/
      v97 = *(float *)&v83 * v90; /*0x1801a7279*/
      v102 = v85; /*0x1801a727f*/
      v98 = v87; /*0x1801a7284*/
      if ( (void **)v86 == &GlissandoNotes::`vftable' ) /*0x1801a728c*/
      {
        v88 = *(_BYTE *)(v75 + 97); /*0x1801a7292*/
      }
      else
      {
        v88 = (*(__int64 (__fastcall **)(__int64 *))(v86 + 192))(a1); /*0x1801a73d9*/
        v85 = v102; /*0x1801a73df*/
        v87 = v98; /*0x1801a73e4*/
      }
      if ( v88 != 0 ) /*0x1801a7298*/
      {
        v102 = v85 * 0.25; /*0x1801a72a4*/
        v98 = v87 * 0.25; /*0x1801a72a9*/
      }
      sub_1800D5370( /*0x1801a731b*/
        a1: (_DWORD *)a1 + 54,
        a2: v4 + v76 + v93,
        a3: SLODWORD(v91),
        a4: v4 + v93 - v76,
        a5: SLODWORD(v91),
        a6: v4 + *v71 - v74,
        a7: *((_DWORD *)a1 + 8),
        a8: v4 + *v71 + v74,
        a9: *((_DWORD *)a1 + 8),
        a10: (__int64)&v95,
        a11: (__int64)&v99,
        a12: (__int64)&v95,
        a13: (__int64)&v99);
    }
    (*(void (__fastcall **)(__int64 *))(a1[27] + 32))(a1: a1 + 27); /*0x1801a7326*/
  }
  return 0; /*0x1801a7366*/
}
