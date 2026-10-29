char __fastcall sub_1801A84F0(__int64 *a1)
{
  int v1; // eax
  int v3; // r14d
  int v4; // r12d
  int v5; // edx
  int v6; // ecx
  unsigned int v7; // ecx
  int v8; // eax
  int v9; // edx
  int *v10; // r13
  int *v11; // r15
  __int64 v12; // rax
  float v13; // xmm10_4
  int v14; // ecx
  float *v15; // rbx
  __int64 v16; // rax
  int v17; // edi
  __int64 v18; // rdx
  __int64 v19; // rcx
  char v20; // al
  __int128 *v21; // rbx
  __int64 i; // rdi
  __int64 v23; // rcx
  int v24; // edx
  int v25; // ecx
  int v26; // r8d
  int v27; // r9d
  int v28; // eax
  __m128i v29; // xmm0
  int v30; // eax
  __int128 *v31; // rax
  __int128 v32; // xmm0
  __m128 v33; // xmm1
  int v34; // edx
  int v35; // r8d
  int v36; // r9d
  int v37; // ecx
  __int128 *v38; // rax
  __int128 v39; // xmm0
  int v40; // edx
  int v41; // r8d
  int v42; // r9d
  __int128 *v43; // rax
  __int128 v44; // xmm0
  __m128 v45; // xmm1
  int v46; // edx
  int v47; // r8d
  int v48; // r9d
  __int128 *v49; // rax
  __int64 v50; // rbx
  __int64 v51; // rax
  int v52; // edi
  int v53; // edx
  __int64 v54; // rax
  _QWORD **v55; // rbx
  bool v56; // zf
  __int64 v57; // rdx
  int v58; // r9d
  __int64 v59; // rcx
  unsigned int v60; // eax
  __m128i v61; // xmm11
  __int64 v62; // rax
  float v63; // xmm11_4
  int v64; // ebx
  int v65; // eax
  int v66; // eax
  float v67; // r15d
  int v68; // r13d
  __int64 v69; // rcx
  char v70; // al
  int v71; // xmm9_4
  __int128 *v72; // rbx
  __int64 j; // rdi
  float v74; // xmm13_4
  __int64 v75; // rcx
  int v76; // edi
  int v77; // edx
  int v78; // ebx
  int v79; // r8d
  int v80; // r9d
  __m128 v81; // xmm8
  __m128 v82; // xmm6
  __int128 *v83; // rax
  __int128 v84; // xmm0
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
  __int128 *v99; // rax
  __int64 v100; // rbx
  __int64 v101; // rax
  int v102; // edi
  int v103; // edx
  __int64 v104; // rax
  int v105; // eax
  _QWORD *v106; // rbx
  __int64 v107; // rcx
  __int64 v108; // rax
  __int64 v109; // rdx
  float v110; // xmm6_4
  float v111; // xmm9_4
  __int64 v112; // rax
  int v113; // ebx
  int v114; // eax
  __int64 v115; // rdx
  int v116; // r13d
  float v117; // xmm7_4
  float v118; // xmm8_4
  float v119; // r14d
  __int64 v120; // rax
  int v121; // ebx
  int v122; // eax
  __int64 v123; // rcx
  float v124; // xmm1_4
  int v125; // ebx
  int v126; // r15d
  float v127; // xmm9_4
  float v128; // xmm10_4
  float v129; // xmm11_4
  float v130; // xmm12_4
  float v131; // xmm13_4
  float v132; // xmm14_4
  float v133; // xmm1_4
  float v134; // xmm4_4
  float v135; // xmm3_4
  float v136; // xmm8_4
  float v137; // xmm5_4
  float v138; // xmm0_4
  float v139; // xmm15_4
  float v140; // xmm2_4
  float v141; // xmm12_4
  float v142; // xmm13_4
  float v143; // xmm14_4
  float v144; // xmm1_4
  float v145; // xmm8_4
  float v146; // xmm9_4
  float v147; // xmm10_4
  float v148; // xmm11_4
  float v149; // xmm7_4
  float v150; // xmm0_4
  float v151; // xmm3_4
  float v152; // xmm4_4
  float v153; // xmm4_4
  float v154; // xmm3_4
  char v155; // al
  int v156; // edi
  int v157; // r9d
  int v158; // r14d
  int v159; // ebx
  int v160; // r9d
  int v161; // r14d
  _QWORD *v162; // rbx
  int v164; // [rsp+20h] [rbp-E0h]
  float v165; // [rsp+70h] [rbp-90h] BYREF
  unsigned __int64 v166; // [rsp+74h] [rbp-8Ch] BYREF
  float v167; // [rsp+7Ch] [rbp-84h] BYREF
  unsigned int v168; // [rsp+80h] [rbp-80h]
  float v169; // [rsp+84h] [rbp-7Ch] BYREF
  int v170; // [rsp+88h] [rbp-78h]
  float v171; // [rsp+8Ch] [rbp-74h]
  float v172; // [rsp+90h] [rbp-70h] BYREF
  float v173; // [rsp+94h] [rbp-6Ch] BYREF
  float v174; // [rsp+98h] [rbp-68h]
  float v175; // [rsp+9Ch] [rbp-64h] BYREF
  float v176; // [rsp+A0h] [rbp-60h] BYREF
  int v177; // [rsp+A4h] [rbp-5Ch] BYREF
  _QWORD *v178; // [rsp+A8h] [rbp-58h]
  int v179; // [rsp+B0h] [rbp-50h]
  float v180; // [rsp+B8h] [rbp-48h]
  float v181; // [rsp+BCh] [rbp-44h]
  float v182; // [rsp+C0h] [rbp-40h]
  unsigned __int64 v183; // [rsp+C8h] [rbp-38h] BYREF
  float v184; // [rsp+D0h] [rbp-30h]
  float v185; // [rsp+D4h] [rbp-2Ch]
  int v186; // [rsp+D8h] [rbp-28h]
  unsigned int v187; // [rsp+DCh] [rbp-24h] BYREF
  int v188; // [rsp+E0h] [rbp-20h]
  int v189; // [rsp+E4h] [rbp-1Ch]
  int v190; // [rsp+E8h] [rbp-18h] BYREF
  int v191; // [rsp+ECh] [rbp-14h] BYREF
  __int64 v192; // [rsp+F0h] [rbp-10h]
  float v193; // [rsp+F8h] [rbp-8h]
  float v194; // [rsp+100h] [rbp+0h] BYREF
  float v195; // [rsp+104h] [rbp+4h]
  float v196; // [rsp+108h] [rbp+8h]
  float v197; // [rsp+10Ch] [rbp+Ch]
  unsigned __int64 v198; // [rsp+110h] [rbp+10h] BYREF
  float v199; // [rsp+118h] [rbp+18h]
  float v200; // [rsp+11Ch] [rbp+1Ch]
  float v201; // [rsp+120h] [rbp+20h] BYREF
  float v202; // [rsp+124h] [rbp+24h]
  float v203; // [rsp+128h] [rbp+28h]
  float v204; // [rsp+12Ch] [rbp+2Ch]
  int v205; // [rsp+130h] [rbp+30h]
  float v206; // [rsp+134h] [rbp+34h]
  float k; // [rsp+138h] [rbp+38h]
  int v208; // [rsp+13Ch] [rbp+3Ch]
  unsigned __int64 v209; // [rsp+140h] [rbp+40h] BYREF
  int v210; // [rsp+148h] [rbp+48h]
  unsigned __int64 v211; // [rsp+14Ch] [rbp+4Ch]
  int v212; // [rsp+154h] [rbp+54h]
  unsigned __int64 v213; // [rsp+158h] [rbp+58h]
  int v214; // [rsp+160h] [rbp+60h]
  unsigned __int64 v215; // [rsp+164h] [rbp+64h]
  int v216; // [rsp+16Ch] [rbp+6Ch]
  __int128 v217; // [rsp+170h] [rbp+70h] BYREF
  __int128 v218; // [rsp+180h] [rbp+80h]
  __int128 v219; // [rsp+190h] [rbp+90h]
  __int128 v220; // [rsp+1A0h] [rbp+A0h]

  v1 = *((_DWORD *)a1 + 26); /*0x1801a8517*/
  if ( v1 > 0 ) /*0x1801a851f*/
    *((_DWORD *)a1 + 26) = v1 - 1; /*0x1801a8523*/
  v3 = -2; /*0x1801a8531*/
  v4 = 0; /*0x1801a853a*/
  v5 = *(_DWORD *)(sub_18018B090(a1: *((unsigned int *)a1 + 16)) + 88); /*0x1801a853d*/
  if ( v6 < 4 ) /*0x1801a8540*/
    v3 = 0; /*0x1801a8540*/
  if ( v6 == 3 ) /*0x1801a854c*/
    v3 = -1; /*0x1801a854c*/
  if ( v5 > 0 ) /*0x1801a8557*/
    v5 = 0; /*0x1801a8557*/
  v192 = 2; /*0x1801a855b*/
  v168 = 2; /*0x1801a8562*/
  v7 = 2; /*0x1801a8565*/
  if ( v5 > v3 ) /*0x1801a8567*/
    v3 = v5; /*0x1801a8567*/
  if ( v3 < 0 ) /*0x1801a8571*/
  {
    v8 = *((_DWORD *)a1 + 16); /*0x1801a8573*/
    if ( v8 > 4 ) /*0x1801a8578*/
    {
      if ( v3 + v8 <= 4 ) /*0x1801a857f*/
        v7 = 4; /*0x1801a857f*/
      v168 = v7; /*0x1801a8582*/
    }
  }
  if ( (unsigned int)(*((_DWORD *)a1 + 16) % 2) <= 1 && *((int *)a1 + 16) >= 3 && v3 == -1 ) /*0x1801a85ac*/
    v4 = dword_1813E476C / -2; /*0x1801a85b7*/
  v9 = *((_DWORD *)a1 + 6); /*0x1801a85c3*/
  v208 = v3 * dword_1813E476C / 2; /*0x1801a85dc*/
  if ( dword_1813668E0 >= v9 - dword_18070A610 - HIDWORD(qword_181366908) ) /*0x1801a85df*/
    *((_BYTE *)a1 + 17) = 1; /*0x1801a85e1*/
  if ( *(_DWORD *)(a1[1] + 48) != 2 ) /*0x1801a85ed*/
  {
    if ( *((_DWORD *)a1 + 79) != 0 ) /*0x1801a8679*/
    {
      v16 = *a1; /*0x1801a8771*/
      v13 = (float)(180 * *((_DWORD *)a1 + 83)) / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300); /*0x1801a8778*/
      v193 = v13; /*0x1801a877d*/
      if ( (void **)v16 == &TrillNotes::`vftable' ) /*0x1801a8786*/
      {
        v17 = *((_DWORD *)a1 + 12); /*0x1801a878c*/
      }
      else
      {
        v17 = (*(__int64 (__fastcall **)(__int64 *))(v16 + 72))(a1); /*0x1801a9881*/
        v16 = *a1; /*0x1801a9883*/
      }
      if ( (void **)v16 == &TrillNotes::`vftable' ) /*0x1801a8792*/
        v18 = *((unsigned int *)a1 + 11); /*0x1801a8798*/
      else
        v18 = (*(unsigned int (__fastcall **)(__int64 *))(v16 + 64))(a1); /*0x1801a9891*/
      v15 = (float *)a1 + 91; /*0x1801a87b6*/
      v11 = (int *)(a1 + 4); /*0x1801a87c1*/
      v10 = (int *)a1 + 7; /*0x1801a87c9*/
      sub_180305430( /*0x1801a87ee*/
        a1: *((_DWORD *)a1 + 5),
        a2: v18,
        a3: (int *)a1 + 7,
        a4: (int *)a1 + 8,
        a5: nullptr,
        a6: (float *)a1 + 89,
        a7: (float *)a1 + 90,
        a8: (float *)a1 + 91,
        a9: v18,
        a10: v17,
        a11: nullptr,
        a12: nullptr);
    }
    else
    {
      v10 = (int *)a1 + 7; /*0x1801a8682*/
      *((_DWORD *)a1 + 84) = *((_DWORD *)a1 + 7); /*0x1801a8686*/
      v11 = (int *)(a1 + 4); /*0x1801a868c*/
      *((_DWORD *)a1 + 85) = *((_DWORD *)a1 + 8); /*0x1801a8693*/
      v12 = *a1; /*0x1801a86c2*/
      v13 = (float)(180 * (v9 - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908))) /*0x1801a86c9*/
          / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300);
      v193 = v13; /*0x1801a86ce*/
      if ( (void **)v12 == &TrillNotes::`vftable' ) /*0x1801a86d7*/
      {
        v167 = *((float *)a1 + 12); /*0x1801a86e0*/
      }
      else
      {
        LODWORD(v167) = (*(__int64 (__fastcall **)(__int64 *))(v12 + 72))(a1); /*0x1801a9862*/
        v12 = *a1; /*0x1801a9866*/
      }
      if ( (void **)v12 == &TrillNotes::`vftable' ) /*0x1801a86e7*/
        v14 = *((_DWORD *)a1 + 11); /*0x1801a86ed*/
      else
        v14 = (*(__int64 (__fastcall **)(__int64 *))(v12 + 64))(a1); /*0x1801a9874*/
      v15 = (float *)a1 + 91; /*0x1801a86fe*/
      sub_180305430( /*0x1801a873f*/
        a1: *((_DWORD *)a1 + 5),
        a2: LODWORD(v167),
        a3: (int *)a1 + 7,
        a4: (int *)a1 + 8,
        a5: nullptr,
        a6: (float *)a1 + 89,
        a7: (float *)a1 + 90,
        a8: (float *)a1 + 91,
        a9: v14,
        a10: SLODWORD(v167),
        a11: nullptr,
        a12: nullptr);
      *((_DWORD *)a1 + 9) = *v10; /*0x1801a8748*/
      *((_DWORD *)a1 + 10) = *v11; /*0x1801a874e*/
    }
    v19 = a1[1]; /*0x1801a87f3*/
    if ( *(_DWORD *)(v19 + 48) != 2 && BYTE1(dword_1813E473C) != 0 ) /*0x1801a8818*/
    {
      if ( (void **)*a1 == &TrillNotes::`vftable' ) /*0x1801a882b*/
        v20 = *(_BYTE *)(v19 + 97); /*0x1801a8831*/
      else
        v20 = (*(__int64 (__fastcall **)(__int64 *))(*a1 + 192))(a1); /*0x1801a989b*/
      if ( v20 != 0 ) /*0x1801a8837*/
        *v15 = *v15 * 0.25; /*0x1801a8841*/
      v21 = &v217; /*0x1801a8845*/
      for ( i = 4; i != 0; --i ) /*0x1801a8849*/
        unknown_libname_4(a1: v21++); /*0x1801a8853*/
      v23 = *((_DWORD *)a1 + 60) / v168; /*0x1801a8870*/
      v170 = (int)(float)((float)(int)v23 * *((float *)a1 + 89)); /*0x1801a8884*/
      v25 = (int)(float)((float)((float)(*((_DWORD *)a1 + 61) >> 1) * *((float *)a1 + 89)) /*0x1801a88b5*/
                       * (float)((float)*(int *)(sub_18018B090(a1: v23) + 84) / 10.0));
      v169 = *(float *)&v25; /*0x1801a88b9*/
      if ( v26 > 0 && v25 > 0 ) /*0x1801a88c7*/
      {
        v28 = *v10 - v26; /*0x1801a88d1*/
        v179 = 0; /*0x1801a88d4*/
        v29 = _mm_cvtsi32_si128(v4 + v28); /*0x1801a88da*/
        v30 = *v11 - v25; /*0x1801a88e1*/
        v210 = 0; /*0x1801a88f3*/
        v209 = _mm_unpacklo_ps(_mm_cvtepi32_ps(v29), (__m128)COERCE_UNSIGNED_INT((float)v30)).m128_u64[0]; /*0x1801a8901*/
        sub_1800D23B0(a1: (unsigned int)&v183, a2: v24, a3: v26, a4: v27, a5: *((_DWORD *)a1 + 91)); /*0x1801a891a*/
        v179 = 0; /*0x1801a8923*/
        v32 = *v31; /*0x1801a8926*/
        LODWORD(v31) = v4 + v170 + *v10 - 1; /*0x1801a8932*/
        v217 = v32; /*0x1801a8935*/
        v33 = (__m128)COERCE_UNSIGNED_INT((float)(*v11 - LODWORD(v169))); /*0x1801a894c*/
        v212 = 0; /*0x1801a894f*/
        v211 = _mm_unpacklo_ps((__m128)COERCE_UNSIGNED_INT((float)(int)v31), v33).m128_u64[0]; /*0x1801a895d*/
        sub_1800D23B0(a1: (unsigned int)&v183, a2: v34, a3: v35, a4: v36, a5: *((_DWORD *)a1 + 91)); /*0x1801a8976*/
        v179 = 0; /*0x1801a8980*/
        v37 = *v11 + LODWORD(v169) - 1; /*0x1801a8983*/
        v39 = *v38; /*0x1801a8986*/
        LODWORD(v38) = *v10 - v170; /*0x1801a898d*/
        v218 = v39; /*0x1801a8990*/
        v214 = 0; /*0x1801a89ae*/
        v213 = _mm_unpacklo_ps( /*0x1801a89bc*/
                 (__m128)COERCE_UNSIGNED_INT((float)(v4 + (int)v38)),
                 (__m128)COERCE_UNSIGNED_INT((float)v37)).m128_u64[0];
        sub_1800D23B0(a1: (unsigned int)&v183, a2: v40, a3: v41, a4: v42, a5: *((_DWORD *)a1 + 91)); /*0x1801a89d5*/
        v44 = *v43; /*0x1801a89df*/
        LODWORD(v43) = v4 + v170 + *v10 - 1; /*0x1801a89eb*/
        v219 = v44; /*0x1801a89ee*/
        v45 = (__m128)COERCE_UNSIGNED_INT((float)(*v11 + LODWORD(v169) - 1)); /*0x1801a8a03*/
        v179 = 0; /*0x1801a8a08*/
        v216 = 0; /*0x1801a8a0f*/
        v215 = _mm_unpacklo_ps((__m128)COERCE_UNSIGNED_INT((float)(int)v43), v45).m128_u64[0]; /*0x1801a8a1d*/
        sub_1800D23B0(a1: (unsigned int)&v183, a2: v46, a3: v47, a4: v48, a5: *((_DWORD *)a1 + 91)); /*0x1801a8a36*/
        v220 = *v49; /*0x1801a8a3e*/
      }
      if ( *((_DWORD *)a1 + 70) == 1 ) /*0x1801a8a4c*/
      {
        v50 = *((int *)a1 + 52); /*0x1801a8a4e*/
        v51 = sub_180339800(); /*0x1801a8a55*/
        *((_DWORD *)a1 + 70) = sub_1800D2820( /*0x1801a8a86*/
                                 a1: (__int64)(a1 + 16),
                                 a2: *(_QWORD *)(v51 + 8 * v50),
                                 a3: 1342177280,
                                 a4: &v209,
                                 a5: &v217,
                                 a6: nullptr);
        (*(void (__fastcall **)(__int64 *))(a1[16] + 32))(a1: a1 + 16); /*0x1801a8a9a*/
      }
      else
      {
        v52 = 80; /*0x1801a8aab*/
        if ( *(_DWORD *)(a1[1] + 48) != 1 ) /*0x1801a8ab4*/
          v52 = 90; /*0x1801a8ab4*/
        *((_DWORD *)a1 + 52) = v52; /*0x1801a8ab7*/
        v53 = v3 + v52 + *((_DWORD *)a1 + 16); /*0x1801a8ac2*/
        if ( v52 + 9 < v53 ) /*0x1801a8aca*/
          v53 = v52 + 9; /*0x1801a8aca*/
        if ( v53 > v52 ) /*0x1801a8acf*/
          v52 = v53; /*0x1801a8acf*/
        *((_DWORD *)a1 + 52) = v52; /*0x1801a8ad2*/
        v54 = sub_180339800(); /*0x1801a8ad8*/
        sub_1800D30C0(a1: a1 + 16, a2: *((unsigned int *)a1 + 70), a3: *(_QWORD *)(v54 + 8LL * v52)); /*0x1801a8af1*/
        sub_1800D3060(a1: (__int64)(a1 + 16), a2: *((_DWORD *)a1 + 70), a3: (__int64)&v209); /*0x1801a8b07*/
        sub_1800D2EC0(a1: a1 + 16, a2: *((unsigned int *)a1 + 70), a3: &v217); /*0x1801a8b1d*/
      }
    }
    v55 = (_QWORD **)(a1 + 36); /*0x1801a8b22*/
    do /*0x1801a8b8f*/
    {
      (*(void (__fastcall **)(_QWORD, _QWORD, _QWORD))(*(_QWORD *)**v55 + 56LL))( /*0x1801a8b43*/
        a1: **v55,
        a2: (unsigned int)(v4 + *v10),
        a3: (unsigned int)*v11);
      (*(void (__fastcall **)(_QWORD))(*(_QWORD *)**v55 + 200LL))(a1: **v55); /*0x1801a8b58*/
      v164 = *((_DWORD *)a1 + 90); /*0x1801a8b74*/
      (*(void (__fastcall **)(_QWORD))(*(_QWORD *)**v55 + 144LL))(a1: **v55); /*0x1801a8b80*/
      v56 = v192-- == 1; /*0x1801a8b86*/
      ++v55; /*0x1801a8b8b*/
    }
    while ( !v56 ); /*0x1801a8b8f*/
    if ( (unsigned __int8)sub_1800D20E0(a1: *(_QWORD *)a1[37]) != 0 ) /*0x1801a8ba2*/
      (*(void (__fastcall **)(_QWORD, _QWORD))(**(_QWORD **)a1[37] + 32LL))(a1: *(_QWORD *)a1[37], a2: 0); /*0x1801a8bb3*/
    v59 = a1[38]; /*0x1801a8bb6*/
    if ( v59 != 0 ) /*0x1801a8bc0*/
      sub_1802B9B90(a1: v59, a2: v4 + *v10, a3: *v11, a4: v58, a5: v164); /*0x1801a8bcc*/
    v60 = 180 * (*((_DWORD *)a1 + 78) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908)); /*0x1801a8bf8*/
    v169 = 1.0; /*0x1801a8c02*/
    v187 = 0; /*0x1801a8c09*/
    v165 = 0.0; /*0x1801a8c10*/
    v61 = _mm_cvtsi32_si128(v60); /*0x1801a8c18*/
    v62 = *a1; /*0x1801a8c1d*/
    v63 = _mm_cvtepi32_ps(v61).m128_f32[0] / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300); /*0x1801a8c24*/
    v182 = v63; /*0x1801a8c29*/
    if ( (void **)v62 == &TrillNotes::`vftable' ) /*0x1801a8c32*/
    {
      v64 = *((_DWORD *)a1 + 12); /*0x1801a8c38*/
    }
    else
    {
      v64 = (*(__int64 (__fastcall **)(__int64 *))(v62 + 72))(a1); /*0x1801a98ac*/
      v62 = *a1; /*0x1801a98ae*/
    }
    if ( (void **)v62 == &TrillNotes::`vftable' ) /*0x1801a8c3e*/
      v65 = *((_DWORD *)a1 + 11); /*0x1801a8c44*/
    else
      v65 = (*(__int64 (__fastcall **)(__int64 *))(v62 + 64))(a1); /*0x1801a98b9*/
    sub_180305430( /*0x1801a8c8c*/
      a1: *((_DWORD *)a1 + 5),
      a2: v57,
      a3: &v177,
      a4: (int *)&v167,
      a5: nullptr,
      a6: &v169,
      a7: (float *)&v187,
      a8: &v165,
      a9: v65,
      a10: v64,
      a11: nullptr,
      a12: nullptr);
    if ( *((_DWORD *)a1 + 79) != 0 ) /*0x1801a8c98*/
    {
      v66 = *v11; /*0x1801a8c9a*/
      v67 = v167; /*0x1801a8c9d*/
      *(float *)&v186 = v167; /*0x1801a8ca2*/
      if ( SLODWORD(v167) > v66 ) /*0x1801a8ca9*/
      {
        v68 = *v10; /*0x1801a8cab*/
        v67 = *(float *)&v66; /*0x1801a8caf*/
        v182 = v13; /*0x1801a8cb2*/
        v63 = v13; /*0x1801a8cb8*/
        v186 = v66; /*0x1801a8cbc*/
        goto LABEL_69; /*0x1801a8cbf*/
      }
    }
    else
    {
      v67 = v167; /*0x1801a8cc1*/
      *(float *)&v186 = v167; /*0x1801a8cc6*/
    }
    v68 = v177; /*0x1801a8cca*/
LABEL_69:
    v69 = a1[1]; /*0x1801a8cce*/
    v167 = *(float *)&v68; /*0x1801a8cd2*/
    if ( *(_DWORD *)(v69 + 48) == 2 || BYTE1(dword_1813E473C) == 0 ) /*0x1801a8ce8*/
    {
      *(float *)&v71 = v165; /*0x1801a8fb7*/
      v74 = v169; /*0x1801a8fbe*/
      *(float *)&v177 = v165; /*0x1801a8fc4*/
    }
    else
    {
      if ( (void **)*a1 == &TrillNotes::`vftable' ) /*0x1801a8cf4*/
        v70 = *(_BYTE *)(v69 + 97); /*0x1801a8cfa*/
      else
        v70 = (*(__int64 (__fastcall **)(__int64 *))(*a1 + 192))(a1); /*0x1801a98c4*/
      *(float *)&v71 = v165; /*0x1801a8cfe*/
      if ( v70 != 0 ) /*0x1801a8d07*/
        *(float *)&v71 = v165 * 0.25; /*0x1801a8d09*/
      v177 = v71; /*0x1801a8d0e*/
      v72 = &v217; /*0x1801a8d14*/
      for ( j = 4; j != 0; --j ) /*0x1801a8d18*/
        unknown_libname_4(a1: v72++); /*0x1801a8d23*/
      v74 = v169; /*0x1801a8d3b*/
      v75 = *((_DWORD *)a1 + 68) / v168; /*0x1801a8d46*/
      v76 = (int)(float)((float)(int)v75 * v169); /*0x1801a8d52*/
      v78 = (int)(float)((float)((float)(*((_DWORD *)a1 + 69) >> 1) * v74) /*0x1801a8d80*/
                       * (float)((float)*(int *)(sub_18018B090(a1: v75) + 84) / 10.0));
      if ( v76 > 0 && v78 > 0 ) /*0x1801a8d8e*/
      {
        v179 = 0; /*0x1801a8da1*/
        v81 = (__m128)COERCE_UNSIGNED_INT((float)(v4 + v68 - v76)); /*0x1801a8dca*/
        v82 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v67) - v78)); /*0x1801a8dd9*/
        v210 = 0; /*0x1801a8ddc*/
        v209 = _mm_unpacklo_ps(v81, v82).m128_u64[0]; /*0x1801a8de2*/
        sub_1800D23B0(a1: (unsigned int)&v183, a2: v77, a3: v79, a4: v80, a5: v71); /*0x1801a8de7*/
        v179 = 0; /*0x1801a8def*/
        v84 = *v83; /*0x1801a8e07*/
        v212 = 0; /*0x1801a8e11*/
        v217 = v84; /*0x1801a8e1c*/
        v85 = (__m128)COERCE_UNSIGNED_INT((float)(v4 + v68 + v76 - 1)); /*0x1801a8e24*/
        v211 = _mm_unpacklo_ps(v85, v82).m128_u64[0]; /*0x1801a8e2d*/
        sub_1800D23B0(a1: (unsigned int)&v183, a2: v86, a3: v87, a4: v88, a5: v71); /*0x1801a8e32*/
        v179 = 0; /*0x1801a8e37*/
        v218 = *v89; /*0x1801a8e5e*/
        v90 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v67) + v78 - 1)); /*0x1801a8e70*/
        v214 = 0; /*0x1801a8e73*/
        v213 = _mm_unpacklo_ps(v81, v90).m128_u64[0]; /*0x1801a8e79*/
        sub_1800D23B0(a1: (unsigned int)&v183, a2: v91, a3: v92, a4: v93, a5: v71); /*0x1801a8e7e*/
        v179 = 0; /*0x1801a8e83*/
        v95 = *v94; /*0x1801a8ea1*/
        v216 = 0; /*0x1801a8ea7*/
        v219 = v95; /*0x1801a8eaa*/
        v215 = _mm_unpacklo_ps(v85, v90).m128_u64[0]; /*0x1801a8eb7*/
        sub_1800D23B0(a1: (unsigned int)&v183, a2: v96, a3: v97, a4: v98, a5: v71); /*0x1801a8ebc*/
        v220 = *v99; /*0x1801a8ec4*/
      }
      if ( *((_DWORD *)a1 + 71) == 1 ) /*0x1801a8ed2*/
      {
        v100 = *((int *)a1 + 53); /*0x1801a8ed4*/
        v101 = sub_180339800(); /*0x1801a8edb*/
        *((_DWORD *)a1 + 71) = sub_1800D2820( /*0x1801a8f0b*/
                                 a1: (__int64)(a1 + 21),
                                 a2: *(_QWORD *)(v101 + 8 * v100),
                                 a3: 1342177280,
                                 a4: &v209,
                                 a5: &v217,
                                 a6: nullptr);
        (*(void (__fastcall **)(__int64 *))(a1[21] + 32))(a1: a1 + 21); /*0x1801a8f1f*/
      }
      else
      {
        v102 = 100; /*0x1801a8f37*/
        if ( *(_DWORD *)(a1[1] + 48) != 1 ) /*0x1801a8f40*/
          v102 = 110; /*0x1801a8f40*/
        *((_DWORD *)a1 + 53) = v102; /*0x1801a8f43*/
        v103 = v3 + v102 + *((_DWORD *)a1 + 16); /*0x1801a8f4e*/
        if ( v102 + 9 < v103 ) /*0x1801a8f56*/
          v103 = v102 + 9; /*0x1801a8f56*/
        if ( v103 > v102 ) /*0x1801a8f5b*/
          v102 = v103; /*0x1801a8f5b*/
        *((_DWORD *)a1 + 53) = v102; /*0x1801a8f5e*/
        v104 = sub_180339800(); /*0x1801a8f64*/
        sub_1800D30C0(a1: a1 + 21, a2: *((unsigned int *)a1 + 71), a3: *(_QWORD *)(v104 + 8LL * v102)); /*0x1801a8f7d*/
        sub_1800D3060(a1: (__int64)(a1 + 21), a2: *((_DWORD *)a1 + 71), a3: (__int64)&v209); /*0x1801a8f93*/
        sub_1800D2EC0(a1: a1 + 21, a2: *((unsigned int *)a1 + 71), a3: &v217); /*0x1801a8fa9*/
      }
    }
    v105 = 0; /*0x1801a8fcd*/
    v106 = a1 + 46; /*0x1801a8fd0*/
    v170 = 0; /*0x1801a8fd7*/
    v178 = a1 + 46; /*0x1801a8fda*/
    do /*0x1801a9799*/
    {
      v107 = 7LL * v105; /*0x1801a8fe2*/
      v108 = *v106; /*0x1801a8fe6*/
      v192 = (__int64)&a1[v107 + 46]; /*0x1801a8ff3*/
      (*(void (**)(void))(v108 + 24))(); /*0x1801a8ff7*/
      v110 = v63 - *(float *)&dword_1813E4758; /*0x1801a9003*/
      v173 = *(float *)&v71; /*0x1801a900b*/
      v166 = __PAIR64__(LODWORD(v74), v187); /*0x1801a9011*/
      v176 = *(float *)&v68; /*0x1801a902d*/
      v175 = v67; /*0x1801a9031*/
      v111 = *(float *)&dword_1813668D4 / 80.0; /*0x1801a9035*/
      for ( k = *(float *)&dword_1813668D4 / 80.0; v110 > *(float *)&dword_1813668D4; v110 = v110 - v111 ) /*0x1801a9044*/
        ; /*0x1801a9046*/
      v112 = *a1; /*0x1801a9050*/
      if ( (void **)*a1 == &TrillNotes::`vftable' ) /*0x1801a9056*/
      {
        v113 = *((_DWORD *)a1 + 12); /*0x1801a905c*/
      }
      else
      {
        v113 = (*(__int64 (__fastcall **)(__int64 *))(v112 + 72))(a1); /*0x1801a98d5*/
        v112 = *a1; /*0x1801a98d7*/
      }
      if ( (void **)v112 == &TrillNotes::`vftable' ) /*0x1801a9062*/
        v114 = *((_DWORD *)a1 + 11); /*0x1801a9068*/
      else
        v114 = (*(__int64 (__fastcall **)(__int64 *))(v112 + 64))(a1); /*0x1801a98e2*/
      sub_180305430( /*0x1801a90ad*/
        a1: *((_DWORD *)a1 + 5),
        a2: v109,
        a3: (int *)&v176,
        a4: (int *)&v175,
        a5: nullptr,
        a6: (float *)&v166 + 1,
        a7: (float *)&v166,
        a8: &v173,
        a9: v114,
        a10: v113,
        a11: nullptr,
        a12: nullptr);
      v116 = 4 /*0x1801a9110*/
           - (int)(float)((float)((float)(v110
                                        - (float)((float)(180
                                                        * (*((_DWORD *)a1 + 6)
                                                         - dword_18070A610
                                                         - HIDWORD(qword_181366914)
                                                         - HIDWORD(qword_181366908)))
                                                / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300)))
                                / v111)
                        + 0.5)
           % 4;
      v205 = v116 + 90; /*0x1801a9117*/
      if ( !__OFSUB__(v116, v116 + 90) ) /*0x1801a911d*/
      {
        v117 = *(float *)&v166; /*0x1801a9123*/
        v118 = *((float *)&v166 + 1); /*0x1801a9131*/
        v119 = v176; /*0x1801a9138*/
        v174 = *(float *)&v166; /*0x1801a913c*/
        v171 = v173; /*0x1801a9141*/
        v168 = LODWORD(v175); /*0x1801a9146*/
        do /*0x1801a974d*/
        {
          v120 = *a1; /*0x1801a9150*/
          v166 = 0; /*0x1801a9163*/
          v172 = 0.0; /*0x1801a916f*/
          v110 = fmaxf(v13, v110 - v111); /*0x1801a9176*/
          if ( (void **)v120 == &TrillNotes::`vftable' ) /*0x1801a917d*/
          {
            v121 = *((_DWORD *)a1 + 12); /*0x1801a9183*/
          }
          else
          {
            v121 = (*(__int64 (__fastcall **)(__int64 *))(v120 + 72))(a1); /*0x1801a98f0*/
            v120 = *a1; /*0x1801a98f2*/
          }
          if ( (void **)v120 == &TrillNotes::`vftable' ) /*0x1801a9189*/
            v122 = *((_DWORD *)a1 + 11); /*0x1801a918f*/
          else
            v122 = (*(__int64 (__fastcall **)(__int64 *))(v120 + 64))(a1); /*0x1801a98fd*/
          sub_180305430( /*0x1801a91d6*/
            a1: *((_DWORD *)a1 + 5),
            a2: v115,
            a3: &v190,
            a4: &v191,
            a5: nullptr,
            a6: (float *)&v166 + 1,
            a7: (float *)&v166,
            a8: &v172,
            a9: v122,
            a10: v121,
            a11: nullptr,
            a12: nullptr);
          v123 = a1[1]; /*0x1801a91ee*/
          v56 = *(_DWORD *)(v123 + 48) == 1; /*0x1801a91f2*/
          v124 = (float)(v208 - 10 + *((_DWORD *)a1 + 15) / 2); /*0x1801a91fa*/
          v125 = (int)(float)(v118 * v124); /*0x1801a9202*/
          v206 = *((float *)&v166 + 1); /*0x1801a9212*/
          v126 = (int)(float)(*((float *)&v166 + 1) * v124); /*0x1801a921c*/
          if ( v56 ) /*0x1801a9221*/
          {
            v127 = *(float *)&xmmword_1812D3308; /*0x1801a9223*/
            v128 = *((float *)&xmmword_1812D3308 + 1); /*0x1801a922c*/
            v129 = *((float *)&xmmword_1812D3308 + 2); /*0x1801a9235*/
            v130 = *(float *)&xmmword_1812D3318; /*0x1801a923e*/
            v131 = *((float *)&xmmword_1812D3318 + 1); /*0x1801a9247*/
            v132 = *((float *)&xmmword_1812D3318 + 2); /*0x1801a9250*/
          }
          else
          {
            v127 = *(float *)&xmmword_1812D32E8; /*0x1801a925b*/
            v128 = *((float *)&xmmword_1812D32E8 + 1); /*0x1801a9264*/
            v129 = *((float *)&xmmword_1812D32E8 + 2); /*0x1801a926d*/
            v130 = *(float *)&xmmword_1812D32F8; /*0x1801a9276*/
            v131 = *((float *)&xmmword_1812D32F8 + 1); /*0x1801a927f*/
            v132 = *((float *)&xmmword_1812D32F8 + 2); /*0x1801a9288*/
          }
          v133 = v132; /*0x1801a9294*/
          v180 = v132; /*0x1801a9298*/
          v134 = v129; /*0x1801a929d*/
          v135 = v128; /*0x1801a92a1*/
          v136 = v131; /*0x1801a92a5*/
          v137 = v130; /*0x1801a92a9*/
          v138 = v127; /*0x1801a92ad*/
          if ( v170 == 1 ) /*0x1801a92b4*/
          {
            v127 = *(float *)&xmmword_1812D3328; /*0x1801a92b6*/
            v128 = *((float *)&xmmword_1812D3328 + 1); /*0x1801a92bf*/
            v129 = *((float *)&xmmword_1812D3328 + 2); /*0x1801a92c8*/
            v130 = *(float *)&xmmword_1812D3338; /*0x1801a92d1*/
            v131 = *((float *)&xmmword_1812D3338 + 1); /*0x1801a92da*/
            v132 = *((float *)&xmmword_1812D3338 + 2); /*0x1801a92e3*/
          }
          v139 = *(float *)&v166; /*0x1801a92f4*/
          v140 = v172; /*0x1801a92ff*/
          if ( *(float *)&v166 >= 1.0 ) /*0x1801a9304*/
          {
            v144 = v171; /*0x1801a9399*/
            *(float *)&v166 = v137 * *(float *)&v166; /*0x1801a93a8*/
            v150 = v138 * v117; /*0x1801a93b3*/
            v151 = v135 * v117; /*0x1801a93b7*/
            v152 = v134 * v117; /*0x1801a93bb*/
            *((float *)&v166 + 1) = v136 * v139; /*0x1801a93c4*/
            v146 = v127 * v117; /*0x1801a93d1*/
            v147 = v128 * v117; /*0x1801a93d6*/
            v148 = v129 * v117; /*0x1801a93db*/
            v180 = v180 * v139; /*0x1801a93e3*/
            v145 = v172 * 0.0; /*0x1801a93eb*/
            v149 = v171 * 0.0; /*0x1801a93f0*/
            v181 = v171; /*0x1801a93f4*/
            v175 = v150; /*0x1801a93f9*/
            v176 = v151; /*0x1801a93fe*/
            v173 = v152; /*0x1801a9403*/
            *(float *)&v188 = v171; /*0x1801a9408*/
            v171 = v172; /*0x1801a940d*/
            v141 = v130 * v139; /*0x1801a9412*/
            v142 = v131 * v139; /*0x1801a9417*/
            v143 = v132 * v139; /*0x1801a941c*/
          }
          else
          {
            v141 = v130 * v117; /*0x1801a9319*/
            v142 = v131 * v117; /*0x1801a931e*/
            v143 = v132 * v117; /*0x1801a9323*/
            v175 = v138 * *(float *)&v166; /*0x1801a9328*/
            *((float *)&v166 + 1) = v136 * v174; /*0x1801a933b*/
            v181 = v172; /*0x1801a9342*/
            v140 = v171; /*0x1801a9347*/
            v176 = v135 * *(float *)&v166; /*0x1801a934c*/
            v173 = v134 * *(float *)&v166; /*0x1801a9361*/
            v180 = v133 * v174; /*0x1801a9369*/
            v144 = v172; /*0x1801a936e*/
            v145 = v171 * 0.0; /*0x1801a9371*/
            *(float *)&v166 = v137 * v174; /*0x1801a9376*/
            *(float *)&v188 = v172; /*0x1801a937c*/
            v146 = v127 * v139; /*0x1801a9381*/
            v147 = v128 * v139; /*0x1801a9386*/
            v148 = v129 * v139; /*0x1801a938b*/
            v149 = v172 * 0.0; /*0x1801a9390*/
          }
          v165 = v140; /*0x1801a9421*/
          v153 = v149; /*0x1801a9427*/
          v174 = v149; /*0x1801a942a*/
          v154 = v145; /*0x1801a942f*/
          *(float *)&v189 = v145; /*0x1801a9433*/
          if ( v170 == 1 ) /*0x1801a943b*/
          {
            v181 = 0.80000001; /*0x1801a9445*/
            v149 = 0.80000001; /*0x1801a944a*/
            v165 = 0.80000001; /*0x1801a944d*/
            v145 = 0.80000001; /*0x1801a9453*/
            *(float *)&v188 = 0.80000001; /*0x1801a9457*/
            v144 = 0.80000001; /*0x1801a945c*/
            v171 = 0.80000001; /*0x1801a945f*/
            v140 = 0.80000001; /*0x1801a9464*/
            *(float *)&v189 = 0.80000001; /*0x1801a9467*/
            v154 = 0.80000001; /*0x1801a946c*/
            v174 = 0.80000001; /*0x1801a946f*/
            v153 = 0.80000001; /*0x1801a9474*/
          }
          if ( (void **)*a1 == &TrillNotes::`vftable' ) /*0x1801a947d*/
          {
            v155 = *(_BYTE *)(v123 + 97); /*0x1801a9483*/
          }
          else
          {
            v155 = (*(__int64 (__fastcall **)(__int64 *))(*a1 + 192))(a1); /*0x1801a9908*/
            v140 = v171; /*0x1801a990e*/
            v144 = *(float *)&v188; /*0x1801a9913*/
            v154 = *(float *)&v189; /*0x1801a9918*/
            v153 = v174; /*0x1801a991d*/
          }
          if ( v155 != 0 ) /*0x1801a9489*/
          {
            v145 = v154 * 0.25; /*0x1801a94a1*/
            v181 = v144 * 0.25; /*0x1801a94a6*/
            v149 = v153 * 0.25; /*0x1801a94b2*/
            v165 = v140 * 0.25; /*0x1801a94b6*/
          }
          unknown_libname_4(a1: &v201); /*0x1801a94c0*/
          unknown_libname_4(a1: &v183); /*0x1801a94c9*/
          unknown_libname_4(a1: &v194); /*0x1801a94d2*/
          unknown_libname_4(a1: &v198); /*0x1801a94db*/
          if ( v116 % 4 > 1 ) /*0x1801a94ff*/
          {
            v203 = v173; /*0x1801a9580*/
            v197 = v181; /*0x1801a9595*/
            v201 = v175; /*0x1801a95a0*/
            v202 = v176; /*0x1801a95ab*/
            v200 = v165; /*0x1801a95b5*/
            v204 = v149; /*0x1801a95ba*/
            v183 = v166; /*0x1801a95bf*/
            v184 = v180; /*0x1801a95c4*/
            v185 = v145; /*0x1801a95c9*/
            v194 = v146; /*0x1801a95cf*/
            v195 = v147; /*0x1801a95d5*/
            v196 = v148; /*0x1801a95db*/
            v198 = __PAIR64__(LODWORD(v142), LODWORD(v141)); /*0x1801a95e1*/
            v199 = v143; /*0x1801a95ed*/
          }
          else
          {
            v204 = v181; /*0x1801a9506*/
            v185 = v165; /*0x1801a9511*/
            v196 = v173; /*0x1801a951b*/
            v194 = v175; /*0x1801a9526*/
            v195 = v176; /*0x1801a9531*/
            v201 = v146; /*0x1801a9540*/
            v202 = v147; /*0x1801a9546*/
            v203 = v148; /*0x1801a954c*/
            v183 = __PAIR64__(LODWORD(v142), LODWORD(v141)); /*0x1801a9552*/
            v184 = v143; /*0x1801a955e*/
            v197 = v149; /*0x1801a9564*/
            v198 = v166; /*0x1801a9569*/
            v199 = v180; /*0x1801a956e*/
            v200 = v145; /*0x1801a9573*/
          }
          if ( (v116 & 1) != 0 ) /*0x1801a95f7*/
            v126 += v126 / -4; /*0x1801a9618*/
          else
            v125 += v125 / -4; /*0x1801a9606*/
          v156 = v190; /*0x1801a961f*/
          if ( v170 != 0 ) /*0x1801a9622*/
          {
            if ( (unsigned int)(v116 % 4) <= 1 ) /*0x1801a96a2*/
            {
              v161 = LODWORD(v119) - v125; /*0x1801a96da*/
              v159 = v191; /*0x1801a96dd*/
              sub_1800D5370( /*0x1801a9709*/
                a1: (_DWORD *)v192,
                a2: v161 + v4,
                a3: v168,
                a4: v161 + v4 + 10,
                a5: v168,
                a6: v4 + v190 - v126 + 10,
                a7: v191,
                a8: v4 + v190 - v126,
                a9: v191,
                a10: (__int64)&v183,
                a11: (__int64)&v201,
                a12: (__int64)&v198,
                a13: (__int64)&v194);
            }
            else
            {
              v160 = v125 + LODWORD(v119); /*0x1801a96ab*/
              v159 = v191; /*0x1801a96af*/
              sub_1800D5370( /*0x1801a96d0*/
                a1: (_DWORD *)v192,
                a2: v4 + v160 - 10,
                a3: v168,
                a4: v4 + v160,
                a5: v168,
                a6: v4 + v126 + v190,
                a7: v191,
                a8: v4 + v126 + v190 - 10,
                a9: v191,
                a10: (__int64)&v183,
                a11: (__int64)&v201,
                a12: (__int64)&v198,
                a13: (__int64)&v194);
            }
          }
          else
          {
            v157 = v125 + LODWORD(v119); /*0x1801a9624*/
            v158 = LODWORD(v119) - v125; /*0x1801a9646*/
            v159 = v191; /*0x1801a9649*/
            sub_1800D5370( /*0x1801a9679*/
              a1: (_DWORD *)a1 + 92,
              a2: v4 + v158,
              a3: v168,
              a4: v4 + v157,
              a5: v168,
              a6: v4 + v126 + v190,
              a7: v191,
              a8: v4 + v190 - v126,
              a9: v191,
              a10: (__int64)&v183,
              a11: (__int64)&v201,
              a12: (__int64)&v198,
              a13: (__int64)&v194);
          }
          v13 = v193; /*0x1801a970e*/
          v117 = v139; /*0x1801a9714*/
          v119 = *(float *)&v156; /*0x1801a971c*/
          v168 = v159; /*0x1801a971f*/
          v171 = v172; /*0x1801a9727*/
          v174 = v139; /*0x1801a972c*/
          if ( v193 >= v110 ) /*0x1801a9731*/
            break; /*0x1801a9731*/
          v118 = v206; /*0x1801a9733*/
          v111 = k; /*0x1801a9740*/
          ++v116; /*0x1801a9746*/
        }
        while ( v116 < v205 ); /*0x1801a974d*/
        v63 = v182; /*0x1801a9753*/
        v74 = v169; /*0x1801a9760*/
        v67 = *(float *)&v186; /*0x1801a9769*/
      }
      v162 = v178; /*0x1801a976d*/
      (*(void (__fastcall **)(__int64))(*v178 + 32LL))(a1: v192); /*0x1801a9778*/
      v106 = v162 + 7; /*0x1801a977e*/
      v71 = v177; /*0x1801a9782*/
      v105 = v170 + 1; /*0x1801a9788*/
      *(float *)&v68 = v167; /*0x1801a978a*/
      v170 = v105; /*0x1801a978f*/
      v178 = v106; /*0x1801a9792*/
    }
    while ( v105 < 2 ); /*0x1801a9799*/
    if ( *((_DWORD *)a1 + 79) == 1 ) /*0x1801a981e*/
    {
      if ( (int)--*((_DWORD *)a1 + 86) <= 0 ) /*0x1801a982d*/
        *((_BYTE *)a1 + 329) = 1; /*0x1801a982f*/
      *((_BYTE *)a1 + 328) = 0; /*0x1801a9836*/
    }
  }
  return 0; /*0x1801a983f*/
}
