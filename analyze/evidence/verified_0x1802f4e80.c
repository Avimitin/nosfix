// nosfix analysis 2026-10-29: Note update/placement: computes 180*(note_frame-song_frame-offsets)/(legacy_rate*legacy_rate). Contains LongNotes handling. Must separate visual evaluation from state mutation.
char __fastcall sub_1802F4E80(__int64 *a1)
{
  int v1; // eax
  int v3; // r12d
  int v4; // edx
  int v5; // ecx
  bool v6; // zf
  int v7; // ecx
  int v8; // eax
  int v9; // edx
  __int64 *v10; // r13
  __int64 v11; // rax
  float v12; // xmm12_4
  __int64 v13; // rdx
  float *v14; // rdi
  __int64 v15; // rdx
  __int64 v16; // rax
  __int64 v17; // rdx
  int v18; // r9d
  __int64 v19; // rcx
  __int64 v20; // rcx
  char v21; // al
  __int128 *v22; // rbx
  __int64 i; // rdi
  int v24; // ecx
  int v25; // r8d
  int v26; // eax
  __m128i v27; // xmm0
  int v28; // eax
  float v29; // xmm1_4
  __int128 *v30; // rax
  __int128 v31; // xmm0
  __m128 v32; // xmm1
  __int128 *v33; // rax
  int v34; // ecx
  __int128 v35; // xmm0
  float v36; // xmm1_4
  __int128 v37; // xmm0
  int v38; // eax
  int v39; // ecx
  float v40; // xmm1_4
  __int64 v41; // rbx
  __int64 v42; // rax
  __int64 v43; // rcx
  int v44; // ecx
  unsigned int v45; // eax
  int v46; // eax
  int v47; // edi
  int v48; // r8d
  __int64 v49; // rax
  unsigned int v50; // eax
  __m128i v51; // xmm9
  __int64 v52; // rax
  float v53; // xmm9_4
  int v54; // ebx
  int v55; // eax
  int v56; // eax
  float v57; // r13d
  float v58; // eax
  __int64 v59; // rcx
  float v60; // xmm14_4
  float v61; // xmm13_4
  char v62; // al
  int v63; // xmm11_4
  __int128 *v64; // rbx
  __int64 j; // rdi
  int v66; // edx
  int v67; // ecx
  int v68; // ebx
  __m128 v69; // xmm8
  __m128 v70; // xmm6
  __int128 *v71; // rax
  __m128 v72; // xmm7
  __int128 *v73; // rax
  __m128 v74; // xmm6
  __int128 *v75; // rax
  __int128 v76; // xmm0
  __int64 v77; // rbx
  __int64 v78; // rax
  int v79; // edi
  int v80; // edx
  __int64 v81; // rax
  __int64 *v82; // rdi
  __int64 *v83; // r12
  __int64 v84; // rdx
  float v85; // xmm9_4
  float v86; // xmm6_4
  __int64 v87; // rax
  int v88; // ebx
  int v89; // eax
  __int64 v90; // rdx
  int v91; // r15d
  float v92; // xmm8_4
  int v93; // r12d
  int v94; // r13d
  __int64 v95; // rax
  int v96; // ebx
  int v97; // eax
  int v98; // kr04_4
  __int64 v99; // rcx
  int v100; // ebx
  int v101; // edx
  int v102; // xmm4_4
  int v103; // xmm5_4
  int v104; // xmm8_4
  int v105; // xmm0_4
  int v106; // xmm10_4
  int v107; // xmm11_4
  int v108; // xmm12_4
  int v109; // xmm1_4
  float v110; // xmm6_4
  float v111; // xmm1_4
  float v112; // xmm3_4
  int v113; // xmm8_4
  float v114; // xmm4_4
  float v115; // xmm0_4
  float v116; // xmm10_4
  float v117; // xmm11_4
  float v118; // xmm12_4
  float v119; // xmm13_4
  float v120; // xmm14_4
  float v121; // xmm15_4
  float v122; // xmm2_4
  float v123; // xmm5_4
  __int64 v124; // rax
  char v125; // al
  int v126; // edi
  int v127; // r9d
  int v128; // ecx
  int v129; // eax
  int v130; // edx
  int v131; // ebx
  int v133; // [rsp+20h] [rbp-E0h]
  float v134; // [rsp+70h] [rbp-90h] BYREF
  float v135; // [rsp+74h] [rbp-8Ch] BYREF
  float v136; // [rsp+78h] [rbp-88h] BYREF
  int v137; // [rsp+7Ch] [rbp-84h]
  float k; // [rsp+80h] [rbp-80h] BYREF
  float v139; // [rsp+84h] [rbp-7Ch] BYREF
  float v140; // [rsp+88h] [rbp-78h]
  float v141; // [rsp+90h] [rbp-70h] BYREF
  float v142; // [rsp+94h] [rbp-6Ch]
  __int64 v143; // [rsp+98h] [rbp-68h]
  float v144; // [rsp+A0h] [rbp-60h]
  float v145; // [rsp+A8h] [rbp-58h] BYREF
  int v146; // [rsp+ACh] [rbp-54h]
  int v147; // [rsp+B0h] [rbp-50h]
  unsigned __int64 v148; // [rsp+B4h] [rbp-4Ch] BYREF
  int v149; // [rsp+BCh] [rbp-44h]
  unsigned __int64 v150; // [rsp+C0h] [rbp-40h]
  int v151; // [rsp+C8h] [rbp-38h] BYREF
  int v152; // [rsp+CCh] [rbp-34h]
  int v153; // [rsp+D0h] [rbp-30h]
  float v154; // [rsp+D4h] [rbp-2Ch]
  float v155; // [rsp+D8h] [rbp-28h]
  int v156; // [rsp+DCh] [rbp-24h]
  int v157; // [rsp+E0h] [rbp-20h]
  int v158; // [rsp+E4h] [rbp-1Ch] BYREF
  int v159; // [rsp+E8h] [rbp-18h] BYREF
  int v160; // [rsp+ECh] [rbp-14h]
  int v161; // [rsp+F0h] [rbp-10h]
  int v162; // [rsp+F4h] [rbp-Ch]
  float v163; // [rsp+F8h] [rbp-8h]
  float v164; // [rsp+FCh] [rbp-4h]
  unsigned __int64 v165; // [rsp+100h] [rbp+0h] BYREF
  int v166; // [rsp+108h] [rbp+8h]
  unsigned __int64 v167; // [rsp+10Ch] [rbp+Ch]
  int v168; // [rsp+114h] [rbp+14h]
  unsigned __int64 v169; // [rsp+118h] [rbp+18h]
  int v170; // [rsp+120h] [rbp+20h]
  unsigned __int64 v171; // [rsp+124h] [rbp+24h]
  int v172; // [rsp+12Ch] [rbp+2Ch]
  __int128 v173; // [rsp+130h] [rbp+30h] BYREF
  __int128 v174; // [rsp+140h] [rbp+40h]
  __int128 v175; // [rsp+150h] [rbp+50h]
  __int128 v176; // [rsp+160h] [rbp+60h]

  v1 = *((_DWORD *)a1 + 26); /*0x1802f4ea4*/
  if ( v1 > 0 ) /*0x1802f4eac*/
    *((_DWORD *)a1 + 26) = v1 - 1; /*0x1802f4eb0*/
  v3 = -2; /*0x1802f4ebe*/
  v137 = 0; /*0x1802f4ec7*/
  v4 = *((_DWORD *)sub_18018B090() + 22); /*0x1802f4ecc*/
  if ( v5 < 4 ) /*0x1802f4ecf*/
    v3 = 0; /*0x1802f4ecf*/
  v6 = v5 == 3; /*0x1802f4ed3*/
  v7 = 2; /*0x1802f4edf*/
  if ( v6 ) /*0x1802f4ee4*/
    v3 = -1; /*0x1802f4ee4*/
  LODWORD(v140) = 2; /*0x1802f4ee8*/
  if ( v4 > 0 ) /*0x1802f4eed*/
    v4 = 0; /*0x1802f4eed*/
  if ( v4 > v3 ) /*0x1802f4ef4*/
    v3 = v4; /*0x1802f4ef4*/
  if ( v3 < 0 ) /*0x1802f4efb*/
  {
    v8 = *((_DWORD *)a1 + 16); /*0x1802f4efd*/
    if ( v8 > 4 ) /*0x1802f4f03*/
    {
      if ( v3 + v8 <= 4 ) /*0x1802f4f0b*/
        v7 = 4; /*0x1802f4f0b*/
      v140 = *(float *)&v7; /*0x1802f4f0f*/
    }
  }
  if ( (unsigned int)(*((_DWORD *)a1 + 16) % 2) <= 1 && *((int *)a1 + 16) >= 3 && v3 == -1 ) /*0x1802f4f39*/
    v137 = dword_1813E476C / -2; /*0x1802f4f44*/
  v9 = *((_DWORD *)a1 + 6); /*0x1802f4f51*/
  v156 = v3 * dword_1813E476C / 2; /*0x1802f4f6a*/
  if ( dword_1813668E0 >= v9 - dword_18070A610 - HIDWORD(qword_181366908) ) /*0x1802f4f6d*/
    *((_BYTE *)a1 + 17) = 1; /*0x1802f4f6f*/
  if ( *(_DWORD *)(a1[1] + 48) != 2 ) /*0x1802f4f7b*/
  {
    if ( *((_DWORD *)a1 + 75) != 0 ) /*0x1802f4fed*/
    {
      v16 = *a1; /*0x1802f50e5*/
      v12 = (float)(180 * *((_DWORD *)a1 + 80)) / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300); /*0x1802f50ec*/
      v155 = v12; /*0x1802f50f1*/
      if ( (void **)v16 == &LongNotes::`vftable' ) /*0x1802f50fa*/
      {
        v134 = *((float *)a1 + 12); /*0x1802f5103*/
      }
      else
      {
        LODWORD(v134) = (*(__int64 (__fastcall **)(__int64 *))(v16 + 72))(a1); /*0x1802f5ff5*/
        v16 = *a1; /*0x1802f5ff9*/
      }
      if ( (void **)v16 == &LongNotes::`vftable' ) /*0x1802f510a*/
        v17 = *((unsigned int *)a1 + 11); /*0x1802f5110*/
      else
        v17 = (*(unsigned int (__fastcall **)(__int64 *))(v16 + 64))(a1); /*0x1802f6007*/
      v14 = (float *)(a1 + 43); /*0x1802f512e*/
      v10 = a1 + 4; /*0x1802f513a*/
      sub_180305430( /*0x1802f5169*/
        a1: *((_DWORD *)a1 + 5),
        a2: v17,
        a3: (int *)a1 + 7,
        a4: (int *)a1 + 8,
        a5: nullptr,
        a6: (float *)a1 + 84,
        a7: (float *)a1 + 85,
        a8: (float *)a1 + 86,
        a9: v17,
        a10: SLODWORD(v134),
        a11: nullptr,
        a12: nullptr);
      v19 = a1[36]; /*0x1802f516e*/
      if ( v19 != 0 ) /*0x1802f5178*/
        sub_1802B9B90(a1: v19, a2: *((_DWORD *)a1 + 7), a3: *(_DWORD *)v10, a4: v18, a5: v133); /*0x1802f5181*/
    }
    else
    {
      v10 = a1 + 4; /*0x1802f4ff6*/
      *((_DWORD *)a1 + 82) = *((_DWORD *)a1 + 7); /*0x1802f4ffa*/
      *((_DWORD *)a1 + 83) = *((_DWORD *)a1 + 8); /*0x1802f5004*/
      v11 = *a1; /*0x1802f5033*/
      v12 = (float)(180 * (v9 - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908))) /*0x1802f503a*/
          / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300);
      v155 = v12; /*0x1802f503f*/
      if ( (void **)v11 == &LongNotes::`vftable' ) /*0x1802f5048*/
      {
        v134 = *((float *)a1 + 12); /*0x1802f5051*/
      }
      else
      {
        LODWORD(v134) = (*(__int64 (__fastcall **)(__int64 *))(v11 + 72))(a1); /*0x1802f5fd6*/
        v11 = *a1; /*0x1802f5fda*/
      }
      if ( (void **)v11 == &LongNotes::`vftable' ) /*0x1802f5058*/
        v13 = *((unsigned int *)a1 + 11); /*0x1802f505e*/
      else
        v13 = (*(unsigned int (__fastcall **)(__int64 *))(v11 + 64))(a1); /*0x1802f5fe8*/
      v14 = (float *)(a1 + 43); /*0x1802f507c*/
      sub_180305430( /*0x1802f50b3*/
        a1: *((_DWORD *)a1 + 5),
        a2: v13,
        a3: (int *)a1 + 7,
        a4: (int *)a1 + 8,
        a5: nullptr,
        a6: (float *)a1 + 84,
        a7: (float *)a1 + 85,
        a8: (float *)a1 + 86,
        a9: v13,
        a10: SLODWORD(v134),
        a11: nullptr,
        a12: nullptr);
      *((_DWORD *)a1 + 9) = *((_DWORD *)a1 + 7); /*0x1802f50bb*/
      *((_DWORD *)a1 + 10) = *(_DWORD *)v10; /*0x1802f50c2*/
    }
    v20 = a1[1]; /*0x1802f5186*/
    if ( *(_DWORD *)(v20 + 48) != 2 && BYTE1(dword_1813E473C) != 0 ) /*0x1802f51ac*/
    {
      if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f51b8*/
        v21 = *(_BYTE *)(v20 + 97); /*0x1802f51be*/
      else
        v21 = (*(__int64 (__fastcall **)(__int64 *))(*a1 + 192))(a1); /*0x1802f6011*/
      if ( v21 != 0 ) /*0x1802f51c4*/
        *v14 = *v14 * 0.25; /*0x1802f51cf*/
      v22 = &v173; /*0x1802f51d3*/
      for ( i = 4; i != 0; --i ) /*0x1802f51d7*/
        unknown_libname_4(a1: v22++); /*0x1802f51e3*/
      LODWORD(v134) = (int)(float)((float)(*((_DWORD *)a1 + 60) / LODWORD(v140)) * *((float *)a1 + 84)); /*0x1802f5214*/
      v24 = (int)(float)((float)((float)(*((_DWORD *)a1 + 61) >> 1) * *((float *)a1 + 84)) /*0x1802f5246*/
                       * (float)((float)*((int *)sub_18018B090() + 21) / 10.0));
      v135 = *(float *)&v24; /*0x1802f524a*/
      if ( v25 > 0 && v24 > 0 ) /*0x1802f5259*/
      {
        v26 = *((_DWORD *)a1 + 7) - v25; /*0x1802f5262*/
        LODWORD(v143) = 0; /*0x1802f5265*/
        v27 = _mm_cvtsi32_si128(v137 + v26); /*0x1802f526c*/
        v28 = *(_DWORD *)v10 - v24; /*0x1802f5274*/
        v166 = 0; /*0x1802f5286*/
        v29 = *((float *)a1 + 85); /*0x1802f528c*/
        v165 = _mm_unpacklo_ps(_mm_cvtepi32_ps(v27), (__m128)COERCE_UNSIGNED_INT((float)v28)).m128_u64[0]; /*0x1802f5294*/
        v30 = (__int128 *)sub_1800D23B0(a1: (__int64)&v145, a2: v29, a3: v29, a4: v29, a5: *((_DWORD *)a1 + 86)); /*0x1802f52ad*/
        LODWORD(v143) = 0; /*0x1802f52b8*/
        v31 = *v30; /*0x1802f52bb*/
        LODWORD(v30) = v137 - 1 + LODWORD(v134) + *((_DWORD *)a1 + 7); /*0x1802f52c5*/
        v173 = v31; /*0x1802f52c7*/
        v32 = (__m128)COERCE_UNSIGNED_INT((float)(*(_DWORD *)v10 - LODWORD(v135))); /*0x1802f52e4*/
        v168 = 0; /*0x1802f52e7*/
        *(_QWORD *)&v31 = _mm_unpacklo_ps((__m128)COERCE_UNSIGNED_INT((float)(int)v30), v32).m128_u64[0]; /*0x1802f52ea*/
        v32.m128_i32[0] = *((_DWORD *)a1 + 85); /*0x1802f52ed*/
        v167 = v31; /*0x1802f52f5*/
        v33 = (__int128 *)sub_1800D23B0( /*0x1802f530e*/
                            a1: (__int64)&v145,
                            a2: v32.m128_f32[0],
                            a3: v32.m128_f32[0],
                            a4: v32.m128_f32[0],
                            a5: *((_DWORD *)a1 + 86));
        LODWORD(v143) = 0; /*0x1802f5319*/
        v34 = *(_DWORD *)v10 + LODWORD(v135) - 1; /*0x1802f531c*/
        v35 = *v33; /*0x1802f5320*/
        LODWORD(v33) = *((_DWORD *)a1 + 7) - LODWORD(v134); /*0x1802f5326*/
        v174 = v35; /*0x1802f532e*/
        v170 = 0; /*0x1802f5346*/
        v36 = *((float *)a1 + 85); /*0x1802f534c*/
        v169 = _mm_unpacklo_ps( /*0x1802f5354*/
                 (__m128)COERCE_UNSIGNED_INT((float)(v137 + (int)v33)),
                 (__m128)COERCE_UNSIGNED_INT((float)v34)).m128_u64[0];
        v37 = *(_OWORD *)sub_1800D23B0(a1: (__int64)&v145, a2: v36, a3: v36, a4: v36, a5: *((_DWORD *)a1 + 86)); /*0x1802f5378*/
        v38 = v137 - 1 + LODWORD(v134) + *((_DWORD *)a1 + 7); /*0x1802f5382*/
        v175 = v37; /*0x1802f5384*/
        v39 = *(_DWORD *)v10 + LODWORD(v135) - 1; /*0x1802f5395*/
        LODWORD(v143) = 0; /*0x1802f539b*/
        v172 = 0; /*0x1802f539e*/
        v40 = *((float *)a1 + 85); /*0x1802f53af*/
        v171 = _mm_unpacklo_ps((__m128)COERCE_UNSIGNED_INT((float)v38), (__m128)COERCE_UNSIGNED_INT((float)v39)).m128_u64[0]; /*0x1802f53b7*/
        v176 = *(_OWORD *)sub_1800D23B0(a1: (__int64)&v145, a2: v40, a3: v40, a4: v40, a5: *((_DWORD *)a1 + 86)); /*0x1802f53d8*/
      }
      if ( *((_DWORD *)a1 + 70) == 1 ) /*0x1802f53e3*/
      {
        v41 = *((int *)a1 + 52); /*0x1802f53e5*/
        v42 = sub_180339800(); /*0x1802f53ec*/
        *((_DWORD *)a1 + 70) = sub_1800D2820( /*0x1802f541d*/
                                 a1: (__int64)(a1 + 16),
                                 a2: *(_QWORD *)(v42 + 8 * v41),
                                 a3: 1342177280,
                                 a4: &v165,
                                 a5: &v173,
                                 a6: nullptr);
        (*(void (__fastcall **)(__int64 *))(a1[16] + 32))(a1: a1 + 16); /*0x1802f5431*/
      }
      else
      {
        if ( HIDWORD(qword_1813E4730) == 1 ) /*0x1802f5440*/
        {
          v44 = *((_DWORD *)a1 + 22); /*0x1802f5449*/
          if ( v44 == 1 ) /*0x1802f544f*/
          {
            v43 = 6; /*0x1802f5451*/
          }
          else
          {
            v45 = 2; /*0x1802f545b*/
            if ( v44 == 3 ) /*0x1802f5465*/
              v45 = 7; /*0x1802f5465*/
            v43 = v45; /*0x1802f5468*/
          }
        }
        else
        {
          v43 = 2; /*0x1802f5442*/
        }
        v46 = sub_180301CB0(a1: v43, a2: *(unsigned int *)(a1[1] + 48)); /*0x1802f5471*/
        *((_DWORD *)a1 + 52) = v46; /*0x1802f5476*/
        v47 = v46; /*0x1802f547c*/
        v48 = v3 + v46 + *((_DWORD *)a1 + 16); /*0x1802f5485*/
        if ( v46 + 9 < v48 ) /*0x1802f548e*/
          v48 = v46 + 9; /*0x1802f548e*/
        if ( v48 > v46 ) /*0x1802f5495*/
          v47 = v48; /*0x1802f5495*/
        *((_DWORD *)a1 + 52) = v47; /*0x1802f5499*/
        v49 = sub_180339800(); /*0x1802f549f*/
        sub_1800D30C0(a1: a1 + 16, a2: *((unsigned int *)a1 + 70), a3: *(_QWORD *)(v49 + 8LL * v47)); /*0x1802f54b8*/
        sub_1800D3060(a1: (__int64)(a1 + 16), a2: *((_DWORD *)a1 + 70), a3: (__int64)&v165); /*0x1802f54ce*/
        sub_1800D2EC0(a1: (__int64)(a1 + 16), a2: *((_DWORD *)a1 + 70), a3: (float *)&v173); /*0x1802f54e4*/
      }
    }
    v50 = 180 * (*((_DWORD *)a1 + 74) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908)); /*0x1802f5509*/
    v139 = 1.0; /*0x1802f5513*/
    v136 = 0.0; /*0x1802f551a*/
    v135 = 0.0; /*0x1802f5522*/
    v51 = _mm_cvtsi32_si128(v50); /*0x1802f552a*/
    v52 = *a1; /*0x1802f552f*/
    v53 = _mm_cvtepi32_ps(v51).m128_f32[0] / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300); /*0x1802f5536*/
    if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f553e*/
    {
      v54 = *((_DWORD *)a1 + 12); /*0x1802f5544*/
    }
    else
    {
      v54 = (*(__int64 (__fastcall **)(__int64 *))(v52 + 72))(a1); /*0x1802f6022*/
      v52 = *a1; /*0x1802f6024*/
    }
    if ( (void **)v52 == &LongNotes::`vftable' ) /*0x1802f554a*/
      v55 = *((_DWORD *)a1 + 11); /*0x1802f5550*/
    else
      v55 = (*(__int64 (__fastcall **)(__int64 *))(v52 + 64))(a1); /*0x1802f602f*/
    sub_180305430( /*0x1802f5599*/
      a1: *((_DWORD *)a1 + 5),
      a2: v15,
      a3: (int *)&k,
      a4: (int *)&v134,
      a5: nullptr,
      a6: &v139,
      a7: &v136,
      a8: &v135,
      a9: v55,
      a10: v54,
      a11: nullptr,
      a12: nullptr);
    if ( *((_DWORD *)a1 + 75) != 0 ) /*0x1802f55a5*/
    {
      v56 = *(_DWORD *)v10; /*0x1802f55a7*/
      v57 = v134; /*0x1802f55ab*/
      if ( SLODWORD(v134) > v56 ) /*0x1802f55b3*/
      {
        v57 = *(float *)&v56; /*0x1802f55b5*/
        v58 = *((float *)a1 + 7); /*0x1802f55b8*/
        goto LABEL_69; /*0x1802f55bb*/
      }
    }
    else
    {
      v57 = v134; /*0x1802f55bd*/
    }
    v58 = k; /*0x1802f55c2*/
LABEL_69:
    v59 = a1[1]; /*0x1802f55c5*/
    v60 = v139; /*0x1802f55c9*/
    v61 = v136; /*0x1802f55cf*/
    v6 = *(_DWORD *)(v59 + 48) == 2; /*0x1802f55de*/
    v134 = v58; /*0x1802f55e2*/
    if ( v6 || BYTE1(dword_1813E473C) == 0 ) /*0x1802f55fc*/
    {
      *(float *)&v63 = v135; /*0x1802f58a2*/
    }
    else
    {
      if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f5608*/
        v62 = *(_BYTE *)(v59 + 97); /*0x1802f560e*/
      else
        v62 = (*(__int64 (__fastcall **)(__int64 *))(*a1 + 192))(a1); /*0x1802f603a*/
      *(float *)&v63 = v135; /*0x1802f5612*/
      if ( v62 != 0 ) /*0x1802f561b*/
        *(float *)&v63 = v135 * 0.25; /*0x1802f561d*/
      v64 = &v173; /*0x1802f5622*/
      for ( j = 4; j != 0; --j ) /*0x1802f5626*/
        unknown_libname_4(a1: v64++); /*0x1802f5633*/
      LODWORD(v144) = (int)(float)((float)(*((_DWORD *)a1 + 68) / LODWORD(v140)) * v60); /*0x1802f5660*/
      v67 = (int)(float)((float)((float)(*((_DWORD *)a1 + 69) >> 1) * v60) /*0x1802f568d*/
                       * (float)((float)*((int *)sub_18018B090() + 21) / 10.0));
      v136 = *(float *)&v67; /*0x1802f5691*/
      if ( v66 > 0 && v67 > 0 ) /*0x1802f569f*/
      {
        v68 = v137; /*0x1802f56ad*/
        LODWORD(v143) = 0; /*0x1802f56c0*/
        v69 = (__m128)COERCE_UNSIGNED_INT((float)(v137 + LODWORD(v134) - v66)); /*0x1802f56d5*/
        v166 = 0; /*0x1802f56e3*/
        v70 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v57) - v67)); /*0x1802f56e6*/
        v165 = _mm_unpacklo_ps(v69, v70).m128_u64[0]; /*0x1802f56ec*/
        v71 = (__int128 *)sub_1800D23B0(a1: (__int64)&v145, a2: v61, a3: v61, a4: v61, a5: v63); /*0x1802f56f1*/
        LODWORD(v143) = 0; /*0x1802f5704*/
        v173 = *v71; /*0x1802f571b*/
        v168 = 0; /*0x1802f5721*/
        v72 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v134) + LODWORD(v144) + v68 - 1)); /*0x1802f572c*/
        v167 = _mm_unpacklo_ps(v72, v70).m128_u64[0]; /*0x1802f5735*/
        v73 = (__int128 *)sub_1800D23B0(a1: (__int64)&v145, a2: v61, a3: v61, a4: v61, a5: v63); /*0x1802f573a*/
        LODWORD(v143) = 0; /*0x1802f574e*/
        v174 = *v73; /*0x1802f5765*/
        v74 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v57) + LODWORD(v136) - 1)); /*0x1802f5773*/
        v170 = 0; /*0x1802f5776*/
        v169 = _mm_unpacklo_ps(v69, v74).m128_u64[0]; /*0x1802f577c*/
        v75 = (__int128 *)sub_1800D23B0(a1: (__int64)&v145, a2: v61, a3: v61, a4: v61, a5: v63); /*0x1802f5781*/
        LODWORD(v143) = 0; /*0x1802f5795*/
        v76 = *v75; /*0x1802f57a0*/
        v172 = 0; /*0x1802f57a5*/
        v175 = v76; /*0x1802f57a8*/
        v171 = _mm_unpacklo_ps(v72, v74).m128_u64[0]; /*0x1802f57b2*/
        v176 = *(_OWORD *)sub_1800D23B0(a1: (__int64)&v145, a2: v61, a3: v61, a4: v61, a5: v63); /*0x1802f57bf*/
      }
      if ( *((_DWORD *)a1 + 71) == 1 ) /*0x1802f57ca*/
      {
        v77 = *((int *)a1 + 53); /*0x1802f57cc*/
        v78 = sub_180339800(); /*0x1802f57d3*/
        *((_DWORD *)a1 + 71) = sub_1800D2820( /*0x1802f5804*/
                                 a1: (__int64)(a1 + 21),
                                 a2: *(_QWORD *)(v78 + 8 * v77),
                                 a3: 1342177280,
                                 a4: &v165,
                                 a5: &v173,
                                 a6: nullptr);
        (*(void (__fastcall **)(__int64 *))(a1[21] + 32))(a1: a1 + 21); /*0x1802f5818*/
      }
      else
      {
        v79 = 100; /*0x1802f5829*/
        if ( *(_DWORD *)(a1[1] + 48) != 1 ) /*0x1802f5832*/
          v79 = 110; /*0x1802f5832*/
        *((_DWORD *)a1 + 53) = v79; /*0x1802f5835*/
        v80 = v3 + v79 + *((_DWORD *)a1 + 16); /*0x1802f5840*/
        if ( v79 + 9 < v80 ) /*0x1802f5848*/
          v80 = v79 + 9; /*0x1802f5848*/
        if ( v80 > v79 ) /*0x1802f584d*/
          v79 = v80; /*0x1802f584d*/
        *((_DWORD *)a1 + 53) = v79; /*0x1802f5850*/
        v81 = sub_180339800(); /*0x1802f5856*/
        sub_1800D30C0(a1: a1 + 21, a2: *((unsigned int *)a1 + 71), a3: *(_QWORD *)(v81 + 8LL * v79)); /*0x1802f586f*/
        sub_1800D3060(a1: (__int64)(a1 + 21), a2: *((_DWORD *)a1 + 71), a3: (__int64)&v165); /*0x1802f5885*/
        sub_1800D2EC0(a1: (__int64)(a1 + 21), a2: *((_DWORD *)a1 + 71), a3: (float *)&v173); /*0x1802f589b*/
      }
    }
    v82 = a1 + 44; /*0x1802f58a9*/
    (*(void (__fastcall **)(__int64 *))(a1[44] + 24))(a1: a1 + 44); /*0x1802f58b6*/
    v83 = a1 + 51; /*0x1802f58b9*/
    (*(void (__fastcall **)(__int64 *))(a1[51] + 24))(a1: a1 + 51); /*0x1802f58c7*/
    v85 = v53 - *(float *)&dword_1813E4758; /*0x1802f58d2*/
    v86 = *(float *)&dword_1813668D4 / 30.0; /*0x1802f58e2*/
    v135 = v57; /*0x1802f58ee*/
    v163 = *(float *)&dword_1813668D4 / 30.0; /*0x1802f58f7*/
    v136 = v60; /*0x1802f58fc*/
    v139 = v61; /*0x1802f5903*/
    for ( k = *(float *)&v63; v85 > *(float *)&dword_1813668D4; v85 = v85 - v86 ) /*0x1802f590f*/
      ; /*0x1802f5911*/
    v87 = *a1; /*0x1802f591c*/
    if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f5922*/
    {
      v88 = *((_DWORD *)a1 + 12); /*0x1802f5928*/
    }
    else
    {
      v88 = (*(__int64 (__fastcall **)(__int64 *))(v87 + 72))(a1); /*0x1802f604b*/
      v87 = *a1; /*0x1802f604d*/
    }
    if ( (void **)v87 == &LongNotes::`vftable' ) /*0x1802f592e*/
      v89 = *((_DWORD *)a1 + 11); /*0x1802f5934*/
    else
      v89 = (*(__int64 (__fastcall **)(__int64 *))(v87 + 64))(a1); /*0x1802f6058*/
    sub_180305430( /*0x1802f597d*/
      a1: *((_DWORD *)a1 + 5),
      a2: v84,
      a3: (int *)&v134,
      a4: (int *)&v135,
      a5: nullptr,
      a6: &v136,
      a7: &v139,
      a8: &k,
      a9: v89,
      a10: v88,
      a11: nullptr,
      a12: nullptr);
    v91 = 4 /*0x1802f59e0*/
        - (int)(float)((float)((float)(v85
                                     - (float)((float)(180
                                                     * (*((_DWORD *)a1 + 6)
                                                      + *((_DWORD *)a1 + 80)
                                                      - dword_18070A610
                                                      - HIDWORD(qword_181366914)
                                                      - HIDWORD(qword_181366908)))
                                             / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300)))
                             / v86)
                     + 0.5)
        % 4;
    v162 = v91 + 37; /*0x1802f59e7*/
    if ( !__OFSUB__(v91, v91 + 37) ) /*0x1802f59ed*/
    {
      v92 = v136; /*0x1802f5a01*/
      v93 = LODWORD(v134); /*0x1802f5a08*/
      v94 = LODWORD(v135); /*0x1802f5a10*/
      v160 = v137 - 2; /*0x1802f5a18*/
      v144 = v139; /*0x1802f5a1b*/
      v140 = k; /*0x1802f5a20*/
      v161 = v137 + 2; /*0x1802f5a2e*/
      do /*0x1802f5ee9*/
      {
        v95 = *a1; /*0x1802f5a31*/
        v136 = 0.0; /*0x1802f5a38*/
        v139 = 0.0; /*0x1802f5a44*/
        v134 = 0.0; /*0x1802f5a4f*/
        v85 = fmaxf(v12, v85 - v86); /*0x1802f5a57*/
        if ( (void **)v95 == &LongNotes::`vftable' ) /*0x1802f5a5f*/
        {
          v96 = *((_DWORD *)a1 + 12); /*0x1802f5a65*/
        }
        else
        {
          v96 = (*(__int64 (__fastcall **)(__int64 *))(v95 + 72))(a1); /*0x1802f6066*/
          v95 = *a1; /*0x1802f6068*/
        }
        if ( (void **)v95 == &LongNotes::`vftable' ) /*0x1802f5a6b*/
          v97 = *((_DWORD *)a1 + 11); /*0x1802f5a71*/
        else
          v97 = (*(__int64 (__fastcall **)(__int64 *))(v95 + 64))(a1); /*0x1802f6073*/
        sub_180305430( /*0x1802f5ab9*/
          a1: *((_DWORD *)a1 + 5),
          a2: v90,
          a3: &v158,
          a4: &v159,
          a5: nullptr,
          a6: &v136,
          a7: &v139,
          a8: &v134,
          a9: v97,
          a10: v96,
          a11: nullptr,
          a12: nullptr);
        v98 = *((_DWORD *)a1 + 15); /*0x1802f5ac5*/
        v141 = *((float *)&qword_181366074 + 1); /*0x1802f5afc*/
        v99 = a1[1]; /*0x1802f5b01*/
        v142 = *(float *)&dword_18136607C; /*0x1802f5b05*/
        v149 = dword_18136607C; /*0x1802f5b17*/
        v151 = dword_181366058; /*0x1802f5b2b*/
        v152 = dword_18136605C; /*0x1802f5b34*/
        v153 = dword_181366060; /*0x1802f5b3f*/
        v154 = *(float *)&dword_181366064; /*0x1802f5b45*/
        v100 = (int)(float)((float)(v156 - 10 + v98 / 2) * v92); /*0x1802f5b4a*/
        v164 = v136; /*0x1802f5b58*/
        v145 = *(float *)&dword_181366068; /*0x1802f5b77*/
        v101 = (int)(float)((float)(v156 - 10 + v98 / 2) * v136); /*0x1802f5b7c*/
        v136 = *(float *)&dword_181366068; /*0x1802f5b88*/
        v146 = dword_18136606C; /*0x1802f5b96*/
        k = *(float *)&dword_18136606C; /*0x1802f5b9b*/
        v147 = dword_181366070; /*0x1802f5ba8*/
        v143 = qword_181366080; /*0x1802f5bad*/
        v150 = qword_181366080; /*0x1802f5bb2*/
        v148 = qword_181366074; /*0x1802f5bb7*/
        v6 = *(_DWORD *)(v99 + 48) == 1; /*0x1802f5bbc*/
        v157 = v101; /*0x1802f5bc0*/
        v135 = *(float *)&dword_181366070; /*0x1802f5bc3*/
        if ( v6 ) /*0x1802f5bc9*/
        {
          v102 = dword_181366038; /*0x1802f5bcb*/
          v103 = dword_18136603C; /*0x1802f5bd3*/
          v104 = dword_181366040; /*0x1802f5bdb*/
          v105 = dword_181366044; /*0x1802f5be4*/
          v106 = dword_181366048; /*0x1802f5bec*/
          v107 = dword_18136604C; /*0x1802f5bf5*/
          v108 = dword_181366050; /*0x1802f5bfe*/
          v109 = dword_181366054; /*0x1802f5c07*/
        }
        else
        {
          v102 = dword_181366018; /*0x1802f5c11*/
          v103 = dword_18136601C; /*0x1802f5c19*/
          v104 = dword_181366020; /*0x1802f5c21*/
          v105 = dword_181366024; /*0x1802f5c2a*/
          v106 = dword_181366028; /*0x1802f5c32*/
          v107 = dword_18136602C; /*0x1802f5c3b*/
          v108 = dword_181366030; /*0x1802f5c44*/
          v109 = dword_181366034; /*0x1802f5c4d*/
        }
        v110 = v139; /*0x1802f5c5d*/
        if ( v139 >= 1.0 ) /*0x1802f5c6a*/
        {
          v111 = *(float *)&v109 * v134; /*0x1802f5ced*/
          v115 = *(float *)&v105 * v140; /*0x1802f5cf3*/
          v122 = *(float *)&dword_181366064 * v140; /*0x1802f5cf8*/
          v141 = *(float *)&v102 * v144; /*0x1802f5d05*/
          v145 = v136 * v139; /*0x1802f5d19*/
          v142 = *(float *)&v103 * v144; /*0x1802f5d24*/
          *(float *)&v143 = *(float *)&v104 * v144; /*0x1802f5d2e*/
          *(float *)&v113 = v134; /*0x1802f5d34*/
          v114 = v135 * v139; /*0x1802f5d3b*/
          v116 = *(float *)&v106 * v139; /*0x1802f5d3f*/
          v117 = *(float *)&v107 * v139; /*0x1802f5d44*/
          v118 = *(float *)&v108 * v139; /*0x1802f5d49*/
          v123 = k * v139; /*0x1802f5d4e*/
          v112 = *(float *)&qword_181366074 * v134; /*0x1802f5d52*/
          v119 = *(float *)&dword_181366058 * v144; /*0x1802f5d57*/
          v120 = *(float *)&dword_18136605C * v144; /*0x1802f5d5c*/
          v121 = *(float *)&dword_181366060 * v144; /*0x1802f5d61*/
        }
        else
        {
          v111 = *(float *)&v109 * v140; /*0x1802f5c70*/
          v112 = *(float *)&qword_181366074 * v140; /*0x1802f5c75*/
          v141 = *(float *)&v102 * v139; /*0x1802f5c82*/
          v145 = v136 * v144; /*0x1802f5c96*/
          v142 = *(float *)&v103 * v139; /*0x1802f5ca1*/
          *(float *)&v143 = *(float *)&v104 * v139; /*0x1802f5cab*/
          *(float *)&v113 = v134; /*0x1802f5cb1*/
          v114 = v135 * v144; /*0x1802f5cb8*/
          v115 = *(float *)&v105 * v134; /*0x1802f5cbc*/
          v116 = *(float *)&v106 * v144; /*0x1802f5cc1*/
          v117 = *(float *)&v107 * v144; /*0x1802f5cc6*/
          v118 = *(float *)&v108 * v144; /*0x1802f5ccb*/
          v119 = *(float *)&dword_181366058 * v139; /*0x1802f5cd0*/
          v120 = *(float *)&dword_18136605C * v139; /*0x1802f5cd5*/
          v121 = *(float *)&dword_181366060 * v139; /*0x1802f5cda*/
          v122 = *(float *)&dword_181366064 * v134; /*0x1802f5cdf*/
          v123 = k * v144; /*0x1802f5ce4*/
        }
        v124 = *a1; /*0x1802f5d66*/
        v150 = __PAIR64__(LODWORD(v111), LODWORD(v118)); /*0x1802f5d69*/
        v149 = LODWORD(v117); /*0x1802f5d6f*/
        v154 = v122; /*0x1802f5d7b*/
        v153 = LODWORD(v121); /*0x1802f5d80*/
        v152 = LODWORD(v120); /*0x1802f5d86*/
        v151 = LODWORD(v119); /*0x1802f5d8c*/
        v148 = __PAIR64__(LODWORD(v116), LODWORD(v112)); /*0x1802f5d92*/
        v147 = LODWORD(v114); /*0x1802f5d97*/
        v146 = LODWORD(v123); /*0x1802f5d9c*/
        *((float *)&v143 + 1) = v115; /*0x1802f5da1*/
        if ( (void **)v124 == &LongNotes::`vftable' ) /*0x1802f5dae*/
        {
          v125 = *(_BYTE *)(v99 + 97); /*0x1802f5db4*/
        }
        else
        {
          v125 = (*(__int64 (__fastcall **)(__int64 *))(v124 + 192))(a1); /*0x1802f607e*/
          v115 = *((float *)&v143 + 1); /*0x1802f6084*/
          v111 = *((float *)&v150 + 1); /*0x1802f6089*/
          v122 = v154; /*0x1802f608e*/
          LODWORD(v112) = v148; /*0x1802f6093*/
          v101 = v157; /*0x1802f6098*/
        }
        if ( v125 != 0 ) /*0x1802f5dba*/
        {
          *((float *)&v143 + 1) = v115 * 0.25; /*0x1802f5dd9*/
          *((float *)&v150 + 1) = v111 * 0.25; /*0x1802f5dde*/
          v154 = v122 * 0.25; /*0x1802f5de3*/
          *(float *)&v148 = v112 * 0.25; /*0x1802f5de8*/
        }
        v126 = v158; /*0x1802f5df6*/
        v127 = v137 + v100 + v93; /*0x1802f5df9*/
        v128 = v137 + v158 - v101; /*0x1802f5e00*/
        v129 = v137 + v101 + v158; /*0x1802f5e09*/
        v130 = v93 - v100; /*0x1802f5e0c*/
        v131 = v159; /*0x1802f5e0e*/
        sub_1800D5370( /*0x1802f5e57*/
          a1: (_DWORD *)a1 + 88,
          a2: v137 + v130,
          a3: v94,
          a4: v127,
          a5: v94,
          a6: v129,
          a7: v159,
          a8: v128,
          a9: v159,
          a10: (__int64)&v148 + 4,
          a11: (__int64)&v141,
          a12: (__int64)&v148 + 4,
          a13: (__int64)&v141);
        sub_1800D5370( /*0x1802f5eb5*/
          a1: (_DWORD *)a1 + 102,
          a2: v160 + v93,
          a3: v94,
          a4: v161 + v93,
          a5: v94,
          a6: v161 + v126,
          a7: v131,
          a8: v160 + v126,
          a9: v131,
          a10: (__int64)&v145,
          a11: (__int64)&v151,
          a12: (__int64)&v145,
          a13: (__int64)&v151);
        v12 = v155; /*0x1802f5eba*/
        v93 = v126; /*0x1802f5ec0*/
        v94 = v131; /*0x1802f5ec7*/
        v144 = v110; /*0x1802f5eca*/
        v140 = *(float *)&v113; /*0x1802f5ecf*/
        if ( v155 >= v85 ) /*0x1802f5ed5*/
          break; /*0x1802f5ed5*/
        v86 = v163; /*0x1802f5ed7*/
        ++v91; /*0x1802f5edc*/
        v92 = v164; /*0x1802f5edf*/
      }
      while ( v91 < v162 ); /*0x1802f5ee9*/
      v82 = a1 + 44; /*0x1802f5ef8*/
      v83 = a1 + 51; /*0x1802f5eff*/
    }
    (*(void (__fastcall **)(__int64 *))(*v82 + 32))(a1: v82); /*0x1802f5f0c*/
    (*(void (__fastcall **)(__int64 *))(*v83 + 32))(a1: v83); /*0x1802f5f16*/
    if ( *((_DWORD *)a1 + 75) == 1 ) /*0x1802f5f8f*/
    {
      if ( *((_BYTE *)a1 + 312) == 0 && (unsigned __int8)sub_1802F4060(a1) == 0 ) /*0x1802f5fa4*/
        *((_BYTE *)a1 + 313) = 1; /*0x1802f5fa6*/
      *((_BYTE *)a1 + 312) = 0; /*0x1802f5fad*/
    }
  }
  return 0; /*0x1802f5fb6*/
}
