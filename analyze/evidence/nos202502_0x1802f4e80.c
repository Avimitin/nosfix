char __fastcall sub_1802F4E80(__int64 *a1)
{
  int v1; // eax
  __int64 v3; // rax
  int v4; // ecx
  int v5; // r12d
  int v6; // edx
  bool v7; // zf
  int v8; // ecx
  int v9; // eax
  int v10; // edx
  int *v11; // r13
  __int64 v12; // rax
  float v13; // xmm12_4
  int v14; // edx
  float *v15; // rdi
  int v16; // edx
  __int64 v17; // rax
  int v18; // edx
  int v19; // r9d
  __int64 v20; // rcx
  __int64 v21; // rcx
  char v22; // al
  __int128 *v23; // rbx
  __int64 i; // rdi
  __int64 v25; // rcx
  int v26; // edx
  int v27; // ecx
  int v28; // r8d
  int v29; // r9d
  int v30; // eax
  __m128i v31; // xmm0
  int v32; // eax
  __int128 *v33; // rax
  __int128 v34; // xmm0
  __m128 v35; // xmm1
  int v36; // edx
  int v37; // r8d
  int v38; // r9d
  __int128 *v39; // rax
  int v40; // ecx
  __int128 v41; // xmm0
  int v42; // edx
  int v43; // r8d
  int v44; // r9d
  __int128 v45; // xmm0
  int v46; // eax
  int v47; // ecx
  int v48; // edx
  int v49; // r8d
  int v50; // r9d
  __int64 v51; // rbx
  __int64 v52; // rax
  __int64 v53; // rcx
  int v54; // ecx
  unsigned int v55; // eax
  int v56; // eax
  int v57; // edi
  int v58; // r8d
  __int64 v59; // rax
  unsigned int v60; // eax
  __m128i v61; // xmm9
  __int64 v62; // rax
  float v63; // xmm9_4
  int v64; // ebx
  int v65; // eax
  int v66; // eax
  float v67; // r13d
  float v68; // eax
  __int64 v69; // rcx
  float v70; // xmm14_4
  float v71; // xmm13_4
  char v72; // al
  float v73; // xmm11_4
  __int128 *v74; // rbx
  __int64 j; // rdi
  __int64 v76; // rcx
  int v77; // edx
  int v78; // ecx
  int v79; // r8d
  int v80; // r9d
  int v81; // ebx
  __m128 v82; // xmm8
  __m128 v83; // xmm6
  __int128 *v84; // rax
  __m128 v85; // xmm7
  int v86; // edx
  int v87; // r8d
  int v88; // r9d
  __int128 *v89; // rax
  __m128 v90; // xmm6
  int v91; // edx
  int v92; // r8d
  int v93; // r9d
  __int128 *v94; // rax
  __int128 v95; // xmm0
  int v96; // edx
  int v97; // r8d
  int v98; // r9d
  __int64 v99; // rbx
  __int64 v100; // rax
  int v101; // edi
  int v102; // edx
  __int64 v103; // rax
  __int64 *v104; // rdi
  __int64 *v105; // r12
  int v106; // edx
  float v107; // xmm9_4
  float v108; // xmm6_4
  __int64 v109; // rax
  int v110; // ebx
  int v111; // eax
  int v112; // edx
  int v113; // r15d
  float v114; // xmm8_4
  float v115; // r12d
  float v116; // r13d
  __int64 v117; // rax
  int v118; // ebx
  int v119; // eax
  int v120; // kr04_4
  __int64 v121; // rcx
  int v122; // ebx
  int v123; // edx
  int v124; // xmm4_4
  int v125; // xmm5_4
  int v126; // xmm8_4
  int v127; // xmm0_4
  int v128; // xmm10_4
  int v129; // xmm11_4
  int v130; // xmm12_4
  int v131; // xmm1_4
  float v132; // xmm6_4
  float v133; // xmm1_4
  float v134; // xmm3_4
  float v135; // xmm8_4
  float v136; // xmm4_4
  float v137; // xmm0_4
  float v138; // xmm10_4
  float v139; // xmm11_4
  float v140; // xmm12_4
  float v141; // xmm13_4
  float v142; // xmm14_4
  float v143; // xmm15_4
  float v144; // xmm2_4
  float v145; // xmm5_4
  __int64 v146; // rax
  char v147; // al
  float v148; // edi
  int v149; // r9d
  int v150; // ecx
  int v151; // eax
  int v152; // edx
  float v153; // ebx
  int v155; // [rsp+20h] [rbp-E0h]
  float v156; // [rsp+70h] [rbp-90h] BYREF
  float v157; // [rsp+74h] [rbp-8Ch] BYREF
  float v158; // [rsp+78h] [rbp-88h] BYREF
  int v159; // [rsp+7Ch] [rbp-84h]
  float k; // [rsp+80h] [rbp-80h] BYREF
  float v161; // [rsp+84h] [rbp-7Ch] BYREF
  unsigned int v162; // [rsp+88h] [rbp-78h]
  float v163; // [rsp+90h] [rbp-70h] BYREF
  float v164; // [rsp+94h] [rbp-6Ch]
  __int64 v165; // [rsp+98h] [rbp-68h]
  float v166; // [rsp+A0h] [rbp-60h]
  float v167; // [rsp+A8h] [rbp-58h] BYREF
  int v168; // [rsp+ACh] [rbp-54h]
  int v169; // [rsp+B0h] [rbp-50h]
  unsigned __int64 v170; // [rsp+B4h] [rbp-4Ch] BYREF
  int v171; // [rsp+BCh] [rbp-44h]
  unsigned __int64 v172; // [rsp+C0h] [rbp-40h]
  int v173; // [rsp+C8h] [rbp-38h] BYREF
  int v174; // [rsp+CCh] [rbp-34h]
  int v175; // [rsp+D0h] [rbp-30h]
  float v176; // [rsp+D4h] [rbp-2Ch]
  float v177; // [rsp+D8h] [rbp-28h]
  int v178; // [rsp+DCh] [rbp-24h]
  int v179; // [rsp+E0h] [rbp-20h]
  float v180; // [rsp+E4h] [rbp-1Ch] BYREF
  float v181; // [rsp+E8h] [rbp-18h] BYREF
  int v182; // [rsp+ECh] [rbp-14h]
  int v183; // [rsp+F0h] [rbp-10h]
  int v184; // [rsp+F4h] [rbp-Ch]
  float v185; // [rsp+F8h] [rbp-8h]
  float v186; // [rsp+FCh] [rbp-4h]
  unsigned __int64 v187; // [rsp+100h] [rbp+0h] BYREF
  int v188; // [rsp+108h] [rbp+8h]
  unsigned __int64 v189; // [rsp+10Ch] [rbp+Ch]
  int v190; // [rsp+114h] [rbp+14h]
  unsigned __int64 v191; // [rsp+118h] [rbp+18h]
  int v192; // [rsp+120h] [rbp+20h]
  unsigned __int64 v193; // [rsp+124h] [rbp+24h]
  int v194; // [rsp+12Ch] [rbp+2Ch]
  __int128 v195; // [rsp+130h] [rbp+30h] BYREF
  __int128 v196; // [rsp+140h] [rbp+40h]
  __int128 v197; // [rsp+150h] [rbp+50h]
  __int128 v198; // [rsp+160h] [rbp+60h]

  v1 = *((_DWORD *)a1 + 26); /*0x1802f4ea4*/
  if ( v1 > 0 ) /*0x1802f4eac*/
    *((_DWORD *)a1 + 26) = v1 - 1; /*0x1802f4eb0*/
  v3 = sub_18018B090(a1: *((unsigned int *)a1 + 16)); /*0x1802f4eb6*/
  v5 = -2; /*0x1802f4ebe*/
  v159 = 0; /*0x1802f4ec7*/
  v6 = *(_DWORD *)(v3 + 88); /*0x1802f4ecc*/
  if ( v4 < 4 ) /*0x1802f4ecf*/
    v5 = 0; /*0x1802f4ecf*/
  v7 = v4 == 3; /*0x1802f4ed3*/
  v8 = 2; /*0x1802f4edf*/
  if ( v7 ) /*0x1802f4ee4*/
    v5 = -1; /*0x1802f4ee4*/
  v162 = 2; /*0x1802f4ee8*/
  if ( v6 > 0 ) /*0x1802f4eed*/
    v6 = 0; /*0x1802f4eed*/
  if ( v6 > v5 ) /*0x1802f4ef4*/
    v5 = v6; /*0x1802f4ef4*/
  if ( v5 < 0 ) /*0x1802f4efb*/
  {
    v9 = *((_DWORD *)a1 + 16); /*0x1802f4efd*/
    if ( v9 > 4 ) /*0x1802f4f03*/
    {
      if ( v5 + v9 <= 4 ) /*0x1802f4f0b*/
        v8 = 4; /*0x1802f4f0b*/
      v162 = v8; /*0x1802f4f0f*/
    }
  }
  if ( (unsigned int)(*((_DWORD *)a1 + 16) % 2) <= 1 && *((int *)a1 + 16) >= 3 && v5 == -1 ) /*0x1802f4f39*/
    v159 = dword_1813E476C / -2; /*0x1802f4f44*/
  v10 = *((_DWORD *)a1 + 6); /*0x1802f4f51*/
  v178 = v5 * dword_1813E476C / 2; /*0x1802f4f6a*/
  if ( dword_1813668E0 >= v10 - dword_18070A610 - HIDWORD(qword_181366908) ) /*0x1802f4f6d*/
    *((_BYTE *)a1 + 17) = 1; /*0x1802f4f6f*/
  if ( *(_DWORD *)(a1[1] + 48) != 2 ) /*0x1802f4f7b*/
  {
    if ( *((_DWORD *)a1 + 75) != 0 ) /*0x1802f4fed*/
    {
      v17 = *a1; /*0x1802f50e5*/
      v13 = (float)(180 * *((_DWORD *)a1 + 80)) / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300); /*0x1802f50ec*/
      v177 = v13; /*0x1802f50f1*/
      if ( (void **)v17 == &LongNotes::`vftable' ) /*0x1802f50fa*/
      {
        v156 = *((float *)a1 + 12); /*0x1802f5103*/
      }
      else
      {
        LODWORD(v156) = (*(__int64 (__fastcall **)(__int64 *))(v17 + 72))(a1); /*0x1802f5ff5*/
        v17 = *a1; /*0x1802f5ff9*/
      }
      if ( (void **)v17 == &LongNotes::`vftable' ) /*0x1802f510a*/
        v18 = *((_DWORD *)a1 + 11); /*0x1802f5110*/
      else
        v18 = (*(__int64 (__fastcall **)(__int64 *))(v17 + 64))(a1); /*0x1802f6007*/
      v15 = (float *)(a1 + 43); /*0x1802f512e*/
      v11 = (int *)(a1 + 4); /*0x1802f513a*/
      sub_180305430( /*0x1802f5169*/
        a1: *((_DWORD *)a1 + 5),
        a2: v18,
        a3: (_DWORD)a1 + 28,
        a4: (_DWORD)a1 + 32,
        a5: 0,
        a6: (__int64)(a1 + 42),
        a7: (__int64)a1 + 340,
        a8: (__int64)(a1 + 43),
        a9: v18,
        a10: LODWORD(v156),
        a11: 0,
        a12: 0);
      v20 = a1[36]; /*0x1802f516e*/
      if ( v20 != 0 ) /*0x1802f5178*/
        sub_1802B9B90(a1: v20, a2: *((_DWORD *)a1 + 7), a3: *v11, a4: v19, a5: v155); /*0x1802f5181*/
    }
    else
    {
      v11 = (int *)(a1 + 4); /*0x1802f4ff6*/
      *((_DWORD *)a1 + 82) = *((_DWORD *)a1 + 7); /*0x1802f4ffa*/
      *((_DWORD *)a1 + 83) = *((_DWORD *)a1 + 8); /*0x1802f5004*/
      v12 = *a1; /*0x1802f5033*/
      v13 = (float)(180 * (v10 - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908))) /*0x1802f503a*/
          / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300);
      v177 = v13; /*0x1802f503f*/
      if ( (void **)v12 == &LongNotes::`vftable' ) /*0x1802f5048*/
      {
        v156 = *((float *)a1 + 12); /*0x1802f5051*/
      }
      else
      {
        LODWORD(v156) = (*(__int64 (__fastcall **)(__int64 *))(v12 + 72))(a1); /*0x1802f5fd6*/
        v12 = *a1; /*0x1802f5fda*/
      }
      if ( (void **)v12 == &LongNotes::`vftable' ) /*0x1802f5058*/
        v14 = *((_DWORD *)a1 + 11); /*0x1802f505e*/
      else
        v14 = (*(__int64 (__fastcall **)(__int64 *))(v12 + 64))(a1); /*0x1802f5fe8*/
      v15 = (float *)(a1 + 43); /*0x1802f507c*/
      sub_180305430( /*0x1802f50b3*/
        a1: *((_DWORD *)a1 + 5),
        a2: v14,
        a3: (_DWORD)a1 + 28,
        a4: (_DWORD)a1 + 32,
        a5: 0,
        a6: (__int64)(a1 + 42),
        a7: (__int64)a1 + 340,
        a8: (__int64)(a1 + 43),
        a9: v14,
        a10: LODWORD(v156),
        a11: 0,
        a12: 0);
      *((_DWORD *)a1 + 9) = *((_DWORD *)a1 + 7); /*0x1802f50bb*/
      *((_DWORD *)a1 + 10) = *v11; /*0x1802f50c2*/
    }
    v21 = a1[1]; /*0x1802f5186*/
    if ( *(_DWORD *)(v21 + 48) != 2 && BYTE1(dword_1813E473C) != 0 ) /*0x1802f51ac*/
    {
      if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f51b8*/
        v22 = *(_BYTE *)(v21 + 97); /*0x1802f51be*/
      else
        v22 = (*(__int64 (__fastcall **)(__int64 *))(*a1 + 192))(a1); /*0x1802f6011*/
      if ( v22 != 0 ) /*0x1802f51c4*/
        *v15 = *v15 * 0.25; /*0x1802f51cf*/
      v23 = &v195; /*0x1802f51d3*/
      for ( i = 4; i != 0; --i ) /*0x1802f51d7*/
        unknown_libname_4(a1: v23++); /*0x1802f51e3*/
      v25 = *((_DWORD *)a1 + 60) / v162; /*0x1802f5200*/
      LODWORD(v156) = (int)(float)((float)(int)v25 * *((float *)a1 + 84)); /*0x1802f5214*/
      v27 = (int)(float)((float)((float)(*((_DWORD *)a1 + 61) >> 1) * *((float *)a1 + 84)) /*0x1802f5246*/
                       * (float)((float)*(int *)(sub_18018B090(a1: v25) + 84) / 10.0));
      v157 = *(float *)&v27; /*0x1802f524a*/
      if ( v28 > 0 && v27 > 0 ) /*0x1802f5259*/
      {
        v30 = *((_DWORD *)a1 + 7) - v28; /*0x1802f5262*/
        LODWORD(v165) = 0; /*0x1802f5265*/
        v31 = _mm_cvtsi32_si128(v159 + v30); /*0x1802f526c*/
        v32 = *v11 - v27; /*0x1802f5274*/
        v188 = 0; /*0x1802f5286*/
        v187 = _mm_unpacklo_ps(_mm_cvtepi32_ps(v31), (__m128)COERCE_UNSIGNED_INT((float)v32)).m128_u64[0]; /*0x1802f5294*/
        v33 = (__int128 *)sub_1800D23B0(a1: (unsigned int)&v167, a2: v26, a3: v28, a4: v29, a5: *((_DWORD *)a1 + 86)); /*0x1802f52ad*/
        LODWORD(v165) = 0; /*0x1802f52b8*/
        v34 = *v33; /*0x1802f52bb*/
        LODWORD(v33) = v159 - 1 + LODWORD(v156) + *((_DWORD *)a1 + 7); /*0x1802f52c5*/
        v195 = v34; /*0x1802f52c7*/
        v35 = (__m128)COERCE_UNSIGNED_INT((float)(*v11 - LODWORD(v157))); /*0x1802f52e4*/
        v190 = 0; /*0x1802f52e7*/
        v189 = _mm_unpacklo_ps((__m128)COERCE_UNSIGNED_INT((float)(int)v33), v35).m128_u64[0]; /*0x1802f52f5*/
        v39 = (__int128 *)sub_1800D23B0(a1: (unsigned int)&v167, a2: v36, a3: v37, a4: v38, a5: *((_DWORD *)a1 + 86)); /*0x1802f530e*/
        LODWORD(v165) = 0; /*0x1802f5319*/
        v40 = *v11 + LODWORD(v157) - 1; /*0x1802f531c*/
        v41 = *v39; /*0x1802f5320*/
        LODWORD(v39) = *((_DWORD *)a1 + 7) - LODWORD(v156); /*0x1802f5326*/
        v196 = v41; /*0x1802f532e*/
        v192 = 0; /*0x1802f5346*/
        v191 = _mm_unpacklo_ps( /*0x1802f5354*/
                 (__m128)COERCE_UNSIGNED_INT((float)(v159 + (int)v39)),
                 (__m128)COERCE_UNSIGNED_INT((float)v40)).m128_u64[0];
        v45 = *(_OWORD *)sub_1800D23B0(a1: (unsigned int)&v167, a2: v42, a3: v43, a4: v44, a5: *((_DWORD *)a1 + 86)); /*0x1802f5378*/
        v46 = v159 - 1 + LODWORD(v156) + *((_DWORD *)a1 + 7); /*0x1802f5382*/
        v197 = v45; /*0x1802f5384*/
        v47 = *v11 + LODWORD(v157) - 1; /*0x1802f5395*/
        LODWORD(v165) = 0; /*0x1802f539b*/
        v194 = 0; /*0x1802f539e*/
        v193 = _mm_unpacklo_ps((__m128)COERCE_UNSIGNED_INT((float)v46), (__m128)COERCE_UNSIGNED_INT((float)v47)).m128_u64[0]; /*0x1802f53b7*/
        v198 = *(_OWORD *)sub_1800D23B0(a1: (unsigned int)&v167, a2: v48, a3: v49, a4: v50, a5: *((_DWORD *)a1 + 86)); /*0x1802f53d8*/
      }
      if ( *((_DWORD *)a1 + 70) == 1 ) /*0x1802f53e3*/
      {
        v51 = *((int *)a1 + 52); /*0x1802f53e5*/
        v52 = sub_180339800(); /*0x1802f53ec*/
        *((_DWORD *)a1 + 70) = sub_1800D2820( /*0x1802f541d*/
                                 a1: (int)a1 + 128,
                                 a2: *(_QWORD *)(v52 + 8 * v51),
                                 a3: 1342177280,
                                 a4: (unsigned int)&v187,
                                 a5: (__int64)&v195,
                                 a6: 0);
        (*(void (__fastcall **)(__int64 *))(a1[16] + 32))(a1: a1 + 16); /*0x1802f5431*/
      }
      else
      {
        if ( HIDWORD(qword_1813E4730) == 1 ) /*0x1802f5440*/
        {
          v54 = *((_DWORD *)a1 + 22); /*0x1802f5449*/
          if ( v54 == 1 ) /*0x1802f544f*/
          {
            v53 = 6; /*0x1802f5451*/
          }
          else
          {
            v55 = 2; /*0x1802f545b*/
            if ( v54 == 3 ) /*0x1802f5465*/
              v55 = 7; /*0x1802f5465*/
            v53 = v55; /*0x1802f5468*/
          }
        }
        else
        {
          v53 = 2; /*0x1802f5442*/
        }
        v56 = sub_180301CB0(a1: v53, a2: *(unsigned int *)(a1[1] + 48)); /*0x1802f5471*/
        *((_DWORD *)a1 + 52) = v56; /*0x1802f5476*/
        v57 = v56; /*0x1802f547c*/
        v58 = v5 + v56 + *((_DWORD *)a1 + 16); /*0x1802f5485*/
        if ( v56 + 9 < v58 ) /*0x1802f548e*/
          v58 = v56 + 9; /*0x1802f548e*/
        if ( v58 > v56 ) /*0x1802f5495*/
          v57 = v58; /*0x1802f5495*/
        *((_DWORD *)a1 + 52) = v57; /*0x1802f5499*/
        v59 = sub_180339800(); /*0x1802f549f*/
        sub_1800D30C0(a1: a1 + 16, a2: *((unsigned int *)a1 + 70), a3: *(_QWORD *)(v59 + 8LL * v57)); /*0x1802f54b8*/
        sub_1800D3060(a1: a1 + 16, a2: *((unsigned int *)a1 + 70), a3: &v187); /*0x1802f54ce*/
        sub_1800D2EC0(a1: a1 + 16, a2: *((unsigned int *)a1 + 70), a3: &v195); /*0x1802f54e4*/
      }
    }
    v60 = 180 * (*((_DWORD *)a1 + 74) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908)); /*0x1802f5509*/
    v161 = 1.0; /*0x1802f5513*/
    v158 = 0.0; /*0x1802f551a*/
    v157 = 0.0; /*0x1802f5522*/
    v61 = _mm_cvtsi32_si128(v60); /*0x1802f552a*/
    v62 = *a1; /*0x1802f552f*/
    v63 = _mm_cvtepi32_ps(v61).m128_f32[0] / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300); /*0x1802f5536*/
    if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f553e*/
    {
      v64 = *((_DWORD *)a1 + 12); /*0x1802f5544*/
    }
    else
    {
      v64 = (*(__int64 (__fastcall **)(__int64 *))(v62 + 72))(a1); /*0x1802f6022*/
      v62 = *a1; /*0x1802f6024*/
    }
    if ( (void **)v62 == &LongNotes::`vftable' ) /*0x1802f554a*/
      v65 = *((_DWORD *)a1 + 11); /*0x1802f5550*/
    else
      v65 = (*(__int64 (__fastcall **)(__int64 *))(v62 + 64))(a1); /*0x1802f602f*/
    sub_180305430( /*0x1802f5599*/
      a1: *((_DWORD *)a1 + 5),
      a2: v16,
      a3: (unsigned int)&k,
      a4: (unsigned int)&v156,
      a5: 0,
      a6: (__int64)&v161,
      a7: (__int64)&v158,
      a8: (__int64)&v157,
      a9: v65,
      a10: v64,
      a11: 0,
      a12: 0);
    if ( *((_DWORD *)a1 + 75) != 0 ) /*0x1802f55a5*/
    {
      v66 = *v11; /*0x1802f55a7*/
      v67 = v156; /*0x1802f55ab*/
      if ( SLODWORD(v156) > v66 ) /*0x1802f55b3*/
      {
        v67 = *(float *)&v66; /*0x1802f55b5*/
        v68 = *((float *)a1 + 7); /*0x1802f55b8*/
        goto LABEL_69; /*0x1802f55bb*/
      }
    }
    else
    {
      v67 = v156; /*0x1802f55bd*/
    }
    v68 = k; /*0x1802f55c2*/
LABEL_69:
    v69 = a1[1]; /*0x1802f55c5*/
    v70 = v161; /*0x1802f55c9*/
    v71 = v158; /*0x1802f55cf*/
    v7 = *(_DWORD *)(v69 + 48) == 2; /*0x1802f55de*/
    v156 = v68; /*0x1802f55e2*/
    if ( v7 || BYTE1(dword_1813E473C) == 0 ) /*0x1802f55fc*/
    {
      v73 = v157; /*0x1802f58a2*/
    }
    else
    {
      if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f5608*/
        v72 = *(_BYTE *)(v69 + 97); /*0x1802f560e*/
      else
        v72 = (*(__int64 (__fastcall **)(__int64 *))(*a1 + 192))(a1); /*0x1802f603a*/
      v73 = v157; /*0x1802f5612*/
      if ( v72 != 0 ) /*0x1802f561b*/
        v73 = v157 * 0.25; /*0x1802f561d*/
      v74 = &v195; /*0x1802f5622*/
      for ( j = 4; j != 0; --j ) /*0x1802f5626*/
        unknown_libname_4(a1: v74++); /*0x1802f5633*/
      v76 = *((_DWORD *)a1 + 68) / v162; /*0x1802f5650*/
      LODWORD(v166) = (int)(float)((float)(int)v76 * v70); /*0x1802f5660*/
      v78 = (int)(float)((float)((float)(*((_DWORD *)a1 + 69) >> 1) * v70) /*0x1802f568d*/
                       * (float)((float)*(int *)(sub_18018B090(a1: v76) + 84) / 10.0));
      v158 = *(float *)&v78; /*0x1802f5691*/
      if ( v77 > 0 && v78 > 0 ) /*0x1802f569f*/
      {
        v81 = v159; /*0x1802f56ad*/
        LODWORD(v165) = 0; /*0x1802f56c0*/
        v82 = (__m128)COERCE_UNSIGNED_INT((float)(v159 + LODWORD(v156) - v77)); /*0x1802f56d5*/
        v188 = 0; /*0x1802f56e3*/
        v83 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v67) - v78)); /*0x1802f56e6*/
        v187 = _mm_unpacklo_ps(v82, v83).m128_u64[0]; /*0x1802f56ec*/
        v84 = (__int128 *)sub_1800D23B0(a1: (unsigned int)&v167, a2: v77, a3: v79, a4: v80, a5: LODWORD(v73)); /*0x1802f56f1*/
        LODWORD(v165) = 0; /*0x1802f5704*/
        v195 = *v84; /*0x1802f571b*/
        v190 = 0; /*0x1802f5721*/
        v85 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v156) + LODWORD(v166) + v81 - 1)); /*0x1802f572c*/
        v189 = _mm_unpacklo_ps(v85, v83).m128_u64[0]; /*0x1802f5735*/
        v89 = (__int128 *)sub_1800D23B0(a1: (unsigned int)&v167, a2: v86, a3: v87, a4: v88, a5: LODWORD(v73)); /*0x1802f573a*/
        LODWORD(v165) = 0; /*0x1802f574e*/
        v196 = *v89; /*0x1802f5765*/
        v90 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v67) + LODWORD(v158) - 1)); /*0x1802f5773*/
        v192 = 0; /*0x1802f5776*/
        v191 = _mm_unpacklo_ps(v82, v90).m128_u64[0]; /*0x1802f577c*/
        v94 = (__int128 *)sub_1800D23B0(a1: (unsigned int)&v167, a2: v91, a3: v92, a4: v93, a5: LODWORD(v73)); /*0x1802f5781*/
        LODWORD(v165) = 0; /*0x1802f5795*/
        v95 = *v94; /*0x1802f57a0*/
        v194 = 0; /*0x1802f57a5*/
        v197 = v95; /*0x1802f57a8*/
        v193 = _mm_unpacklo_ps(v85, v90).m128_u64[0]; /*0x1802f57b2*/
        v198 = *(_OWORD *)sub_1800D23B0(a1: (unsigned int)&v167, a2: v96, a3: v97, a4: v98, a5: LODWORD(v73)); /*0x1802f57bf*/
      }
      if ( *((_DWORD *)a1 + 71) == 1 ) /*0x1802f57ca*/
      {
        v99 = *((int *)a1 + 53); /*0x1802f57cc*/
        v100 = sub_180339800(); /*0x1802f57d3*/
        *((_DWORD *)a1 + 71) = sub_1800D2820( /*0x1802f5804*/
                                 a1: (int)a1 + 168,
                                 a2: *(_QWORD *)(v100 + 8 * v99),
                                 a3: 1342177280,
                                 a4: (unsigned int)&v187,
                                 a5: (__int64)&v195,
                                 a6: 0);
        (*(void (__fastcall **)(__int64 *))(a1[21] + 32))(a1: a1 + 21); /*0x1802f5818*/
      }
      else
      {
        v101 = 100; /*0x1802f5829*/
        if ( *(_DWORD *)(a1[1] + 48) != 1 ) /*0x1802f5832*/
          v101 = 110; /*0x1802f5832*/
        *((_DWORD *)a1 + 53) = v101; /*0x1802f5835*/
        v102 = v5 + v101 + *((_DWORD *)a1 + 16); /*0x1802f5840*/
        if ( v101 + 9 < v102 ) /*0x1802f5848*/
          v102 = v101 + 9; /*0x1802f5848*/
        if ( v102 > v101 ) /*0x1802f584d*/
          v101 = v102; /*0x1802f584d*/
        *((_DWORD *)a1 + 53) = v101; /*0x1802f5850*/
        v103 = sub_180339800(); /*0x1802f5856*/
        sub_1800D30C0(a1: a1 + 21, a2: *((unsigned int *)a1 + 71), a3: *(_QWORD *)(v103 + 8LL * v101)); /*0x1802f586f*/
        sub_1800D3060(a1: a1 + 21, a2: *((unsigned int *)a1 + 71), a3: &v187); /*0x1802f5885*/
        sub_1800D2EC0(a1: a1 + 21, a2: *((unsigned int *)a1 + 71), a3: &v195); /*0x1802f589b*/
      }
    }
    v104 = a1 + 44; /*0x1802f58a9*/
    (*(void (__fastcall **)(__int64 *))(a1[44] + 24))(a1: a1 + 44); /*0x1802f58b6*/
    v105 = a1 + 51; /*0x1802f58b9*/
    (*(void (__fastcall **)(__int64 *))(a1[51] + 24))(a1: a1 + 51); /*0x1802f58c7*/
    v107 = v63 - *(float *)&dword_1813E4758; /*0x1802f58d2*/
    v108 = *(float *)&dword_1813668D4 / 30.0; /*0x1802f58e2*/
    v157 = v67; /*0x1802f58ee*/
    v185 = *(float *)&dword_1813668D4 / 30.0; /*0x1802f58f7*/
    v158 = v70; /*0x1802f58fc*/
    v161 = v71; /*0x1802f5903*/
    for ( k = v73; v107 > *(float *)&dword_1813668D4; v107 = v107 - v108 ) /*0x1802f590f*/
      ; /*0x1802f5911*/
    v109 = *a1; /*0x1802f591c*/
    if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f5922*/
    {
      v110 = *((_DWORD *)a1 + 12); /*0x1802f5928*/
    }
    else
    {
      v110 = (*(__int64 (__fastcall **)(__int64 *))(v109 + 72))(a1); /*0x1802f604b*/
      v109 = *a1; /*0x1802f604d*/
    }
    if ( (void **)v109 == &LongNotes::`vftable' ) /*0x1802f592e*/
      v111 = *((_DWORD *)a1 + 11); /*0x1802f5934*/
    else
      v111 = (*(__int64 (__fastcall **)(__int64 *))(v109 + 64))(a1); /*0x1802f6058*/
    sub_180305430( /*0x1802f597d*/
      a1: *((_DWORD *)a1 + 5),
      a2: v106,
      a3: (unsigned int)&v156,
      a4: (unsigned int)&v157,
      a5: 0,
      a6: (__int64)&v158,
      a7: (__int64)&v161,
      a8: (__int64)&k,
      a9: v111,
      a10: v110,
      a11: 0,
      a12: 0);
    v113 = 4 /*0x1802f59e0*/
         - (int)(float)((float)((float)(v107
                                      - (float)((float)(180
                                                      * (*((_DWORD *)a1 + 6)
                                                       + *((_DWORD *)a1 + 80)
                                                       - dword_18070A610
                                                       - HIDWORD(qword_181366914)
                                                       - HIDWORD(qword_181366908)))
                                              / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300)))
                              / v108)
                      + 0.5)
         % 4;
    v184 = v113 + 37; /*0x1802f59e7*/
    if ( !__OFSUB__(v113, v113 + 37) ) /*0x1802f59ed*/
    {
      v114 = v158; /*0x1802f5a01*/
      v115 = v156; /*0x1802f5a08*/
      v116 = v157; /*0x1802f5a10*/
      v182 = v159 - 2; /*0x1802f5a18*/
      v166 = v161; /*0x1802f5a1b*/
      *(float *)&v162 = k; /*0x1802f5a20*/
      v183 = v159 + 2; /*0x1802f5a2e*/
      do /*0x1802f5ee9*/
      {
        v117 = *a1; /*0x1802f5a31*/
        v158 = 0.0; /*0x1802f5a38*/
        v161 = 0.0; /*0x1802f5a44*/
        v156 = 0.0; /*0x1802f5a4f*/
        v107 = fmaxf(v13, v107 - v108); /*0x1802f5a57*/
        if ( (void **)v117 == &LongNotes::`vftable' ) /*0x1802f5a5f*/
        {
          v118 = *((_DWORD *)a1 + 12); /*0x1802f5a65*/
        }
        else
        {
          v118 = (*(__int64 (__fastcall **)(__int64 *))(v117 + 72))(a1); /*0x1802f6066*/
          v117 = *a1; /*0x1802f6068*/
        }
        if ( (void **)v117 == &LongNotes::`vftable' ) /*0x1802f5a6b*/
          v119 = *((_DWORD *)a1 + 11); /*0x1802f5a71*/
        else
          v119 = (*(__int64 (__fastcall **)(__int64 *))(v117 + 64))(a1); /*0x1802f6073*/
        sub_180305430( /*0x1802f5ab9*/
          a1: *((_DWORD *)a1 + 5),
          a2: v112,
          a3: (unsigned int)&v180,
          a4: (unsigned int)&v181,
          a5: 0,
          a6: (__int64)&v158,
          a7: (__int64)&v161,
          a8: (__int64)&v156,
          a9: v119,
          a10: v118,
          a11: 0,
          a12: 0);
        v120 = *((_DWORD *)a1 + 15); /*0x1802f5ac5*/
        v163 = *((float *)&qword_181366074 + 1); /*0x1802f5afc*/
        v121 = a1[1]; /*0x1802f5b01*/
        v164 = *(float *)&dword_18136607C; /*0x1802f5b05*/
        v171 = dword_18136607C; /*0x1802f5b17*/
        v173 = dword_181366058; /*0x1802f5b2b*/
        v174 = dword_18136605C; /*0x1802f5b34*/
        v175 = dword_181366060; /*0x1802f5b3f*/
        v176 = *(float *)&dword_181366064; /*0x1802f5b45*/
        v122 = (int)(float)((float)(v178 - 10 + v120 / 2) * v114); /*0x1802f5b4a*/
        v186 = v158; /*0x1802f5b58*/
        v167 = *(float *)&dword_181366068; /*0x1802f5b77*/
        v123 = (int)(float)((float)(v178 - 10 + v120 / 2) * v158); /*0x1802f5b7c*/
        v158 = *(float *)&dword_181366068; /*0x1802f5b88*/
        v168 = dword_18136606C; /*0x1802f5b96*/
        k = *(float *)&dword_18136606C; /*0x1802f5b9b*/
        v169 = dword_181366070; /*0x1802f5ba8*/
        v165 = qword_181366080; /*0x1802f5bad*/
        v172 = qword_181366080; /*0x1802f5bb2*/
        v170 = qword_181366074; /*0x1802f5bb7*/
        v7 = *(_DWORD *)(v121 + 48) == 1; /*0x1802f5bbc*/
        v179 = v123; /*0x1802f5bc0*/
        v157 = *(float *)&dword_181366070; /*0x1802f5bc3*/
        if ( v7 ) /*0x1802f5bc9*/
        {
          v124 = dword_181366038; /*0x1802f5bcb*/
          v125 = dword_18136603C; /*0x1802f5bd3*/
          v126 = dword_181366040; /*0x1802f5bdb*/
          v127 = dword_181366044; /*0x1802f5be4*/
          v128 = dword_181366048; /*0x1802f5bec*/
          v129 = dword_18136604C; /*0x1802f5bf5*/
          v130 = dword_181366050; /*0x1802f5bfe*/
          v131 = dword_181366054; /*0x1802f5c07*/
        }
        else
        {
          v124 = dword_181366018; /*0x1802f5c11*/
          v125 = dword_18136601C; /*0x1802f5c19*/
          v126 = dword_181366020; /*0x1802f5c21*/
          v127 = dword_181366024; /*0x1802f5c2a*/
          v128 = dword_181366028; /*0x1802f5c32*/
          v129 = dword_18136602C; /*0x1802f5c3b*/
          v130 = dword_181366030; /*0x1802f5c44*/
          v131 = dword_181366034; /*0x1802f5c4d*/
        }
        v132 = v161; /*0x1802f5c5d*/
        if ( v161 >= 1.0 ) /*0x1802f5c6a*/
        {
          v133 = *(float *)&v131 * v156; /*0x1802f5ced*/
          v137 = *(float *)&v127 * *(float *)&v162; /*0x1802f5cf3*/
          v144 = *(float *)&dword_181366064 * *(float *)&v162; /*0x1802f5cf8*/
          v163 = *(float *)&v124 * v166; /*0x1802f5d05*/
          v167 = v158 * v161; /*0x1802f5d19*/
          v164 = *(float *)&v125 * v166; /*0x1802f5d24*/
          *(float *)&v165 = *(float *)&v126 * v166; /*0x1802f5d2e*/
          v135 = v156; /*0x1802f5d34*/
          v136 = v157 * v161; /*0x1802f5d3b*/
          v138 = *(float *)&v128 * v161; /*0x1802f5d3f*/
          v139 = *(float *)&v129 * v161; /*0x1802f5d44*/
          v140 = *(float *)&v130 * v161; /*0x1802f5d49*/
          v145 = k * v161; /*0x1802f5d4e*/
          v134 = *(float *)&qword_181366074 * v156; /*0x1802f5d52*/
          v141 = *(float *)&dword_181366058 * v166; /*0x1802f5d57*/
          v142 = *(float *)&dword_18136605C * v166; /*0x1802f5d5c*/
          v143 = *(float *)&dword_181366060 * v166; /*0x1802f5d61*/
        }
        else
        {
          v133 = *(float *)&v131 * *(float *)&v162; /*0x1802f5c70*/
          v134 = *(float *)&qword_181366074 * *(float *)&v162; /*0x1802f5c75*/
          v163 = *(float *)&v124 * v161; /*0x1802f5c82*/
          v167 = v158 * v166; /*0x1802f5c96*/
          v164 = *(float *)&v125 * v161; /*0x1802f5ca1*/
          *(float *)&v165 = *(float *)&v126 * v161; /*0x1802f5cab*/
          v135 = v156; /*0x1802f5cb1*/
          v136 = v157 * v166; /*0x1802f5cb8*/
          v137 = *(float *)&v127 * v156; /*0x1802f5cbc*/
          v138 = *(float *)&v128 * v166; /*0x1802f5cc1*/
          v139 = *(float *)&v129 * v166; /*0x1802f5cc6*/
          v140 = *(float *)&v130 * v166; /*0x1802f5ccb*/
          v141 = *(float *)&dword_181366058 * v161; /*0x1802f5cd0*/
          v142 = *(float *)&dword_18136605C * v161; /*0x1802f5cd5*/
          v143 = *(float *)&dword_181366060 * v161; /*0x1802f5cda*/
          v144 = *(float *)&dword_181366064 * v156; /*0x1802f5cdf*/
          v145 = k * v166; /*0x1802f5ce4*/
        }
        v146 = *a1; /*0x1802f5d66*/
        v172 = __PAIR64__(LODWORD(v133), LODWORD(v140)); /*0x1802f5d69*/
        v171 = LODWORD(v139); /*0x1802f5d6f*/
        v176 = v144; /*0x1802f5d7b*/
        v175 = LODWORD(v143); /*0x1802f5d80*/
        v174 = LODWORD(v142); /*0x1802f5d86*/
        v173 = LODWORD(v141); /*0x1802f5d8c*/
        v170 = __PAIR64__(LODWORD(v138), LODWORD(v134)); /*0x1802f5d92*/
        v169 = LODWORD(v136); /*0x1802f5d97*/
        v168 = LODWORD(v145); /*0x1802f5d9c*/
        *((float *)&v165 + 1) = v137; /*0x1802f5da1*/
        if ( (void **)v146 == &LongNotes::`vftable' ) /*0x1802f5dae*/
        {
          v147 = *(_BYTE *)(v121 + 97); /*0x1802f5db4*/
        }
        else
        {
          v147 = (*(__int64 (__fastcall **)(__int64 *))(v146 + 192))(a1); /*0x1802f607e*/
          v137 = *((float *)&v165 + 1); /*0x1802f6084*/
          v133 = *((float *)&v172 + 1); /*0x1802f6089*/
          v144 = v176; /*0x1802f608e*/
          LODWORD(v134) = v170; /*0x1802f6093*/
          v123 = v179; /*0x1802f6098*/
        }
        if ( v147 != 0 ) /*0x1802f5dba*/
        {
          *((float *)&v165 + 1) = v137 * 0.25; /*0x1802f5dd9*/
          *((float *)&v172 + 1) = v133 * 0.25; /*0x1802f5dde*/
          v176 = v144 * 0.25; /*0x1802f5de3*/
          *(float *)&v170 = v134 * 0.25; /*0x1802f5de8*/
        }
        v148 = v180; /*0x1802f5df6*/
        v149 = v159 + v122 + LODWORD(v115); /*0x1802f5df9*/
        v150 = v159 + LODWORD(v180) - v123; /*0x1802f5e00*/
        v151 = v159 + v123 + LODWORD(v180); /*0x1802f5e09*/
        v152 = LODWORD(v115) - v122; /*0x1802f5e0c*/
        v153 = v181; /*0x1802f5e0e*/
        sub_1800D5370( /*0x1802f5e57*/
          a1: (_DWORD)a1 + 352,
          a2: v159 + v152,
          a3: LODWORD(v116),
          a4: v149,
          a5: LODWORD(v116),
          a6: v151,
          a7: LODWORD(v181),
          a8: v150,
          a9: LODWORD(v181),
          a10: (__int64)&v170 + 4,
          a11: (__int64)&v163,
          a12: (__int64)&v170 + 4,
          a13: (__int64)&v163);
        sub_1800D5370( /*0x1802f5eb5*/
          a1: (_DWORD)a1 + 408,
          a2: v182 + LODWORD(v115),
          a3: LODWORD(v116),
          a4: v183 + LODWORD(v115),
          a5: LODWORD(v116),
          a6: v183 + LODWORD(v148),
          a7: LODWORD(v153),
          a8: v182 + LODWORD(v148),
          a9: LODWORD(v153),
          a10: (__int64)&v167,
          a11: (__int64)&v173,
          a12: (__int64)&v167,
          a13: (__int64)&v173);
        v13 = v177; /*0x1802f5eba*/
        v115 = v148; /*0x1802f5ec0*/
        v116 = v153; /*0x1802f5ec7*/
        v166 = v132; /*0x1802f5eca*/
        *(float *)&v162 = v135; /*0x1802f5ecf*/
        if ( v177 >= v107 ) /*0x1802f5ed5*/
          break; /*0x1802f5ed5*/
        v108 = v185; /*0x1802f5ed7*/
        ++v113; /*0x1802f5edc*/
        v114 = v186; /*0x1802f5edf*/
      }
      while ( v113 < v184 ); /*0x1802f5ee9*/
      v104 = a1 + 44; /*0x1802f5ef8*/
      v105 = a1 + 51; /*0x1802f5eff*/
    }
    (*(void (__fastcall **)(__int64 *))(*v104 + 32))(a1: v104); /*0x1802f5f0c*/
    (*(void (__fastcall **)(__int64 *))(*v105 + 32))(a1: v105); /*0x1802f5f16*/
    if ( *((_DWORD *)a1 + 75) == 1 ) /*0x1802f5f8f*/
    {
      if ( *((_BYTE *)a1 + 312) == 0 && (unsigned __int8)sub_1802F4060(a1) == 0 ) /*0x1802f5fa4*/
        *((_BYTE *)a1 + 313) = 1; /*0x1802f5fa6*/
      *((_BYTE *)a1 + 312) = 0; /*0x1802f5fad*/
    }
  }
  return 0; /*0x1802f5fb6*/
}
