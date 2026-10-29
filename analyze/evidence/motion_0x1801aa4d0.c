char __fastcall sub_1801AA4D0(__int64 a1)
{
  int v1; // eax
  int v3; // ecx
  _DWORD *v4; // r13
  void **v5; // rax
  float v6; // xmm9_4
  int v7; // edi
  int v8; // r8d
  void **v9; // rax
  int v10; // edi
  int v11; // r8d
  int v12; // r9d
  __int64 v13; // rcx
  _QWORD *v14; // rbx
  __int64 i; // rdi
  int v16; // r8d
  int v17; // r9d
  float *v18; // rdi
  float v19; // xmm1_4
  int v20; // eax
  int v21; // edx
  int v22; // ecx
  unsigned int v23; // eax
  __m128i v24; // xmm0
  int v25; // eax
  int v26; // ecx
  __int128 *v27; // rax
  __int128 v28; // xmm0
  int v29; // edx
  int v30; // r8d
  int v31; // r9d
  int v32; // ecx
  __int128 *v33; // rax
  __int128 v34; // xmm0
  int v35; // edx
  int v36; // r8d
  int v37; // r9d
  int v38; // ecx
  _OWORD *v39; // rax
  int v40; // edx
  int v41; // r8d
  int v42; // r9d
  _OWORD *v43; // rax
  unsigned int v44; // edx
  __int64 v45; // rdx
  unsigned int v46; // eax
  __m128i v47; // xmm10
  void **v48; // rax
  float v49; // xmm10_4
  int v50; // ebx
  int v51; // eax
  float v52; // eax
  float v53; // r13d
  float v54; // eax
  _QWORD *v55; // rbx
  __int64 j; // rdi
  int v57; // edx
  int v58; // r8d
  int v59; // r9d
  float v60; // xmm11_4
  int v61; // xmm12_4
  float v62; // xmm13_4
  float v63; // edi
  int v64; // eax
  int v65; // ecx
  int v66; // ebx
  __m128 v67; // xmm8
  __m128 v68; // xmm6
  _OWORD *v69; // rax
  __m128 v70; // xmm7
  int v71; // edx
  int v72; // r8d
  int v73; // r9d
  _OWORD *v74; // rax
  __m128 v75; // xmm6
  int v76; // edx
  int v77; // r8d
  int v78; // r9d
  __int128 *v79; // rax
  __int128 v80; // xmm0
  int v81; // edx
  int v82; // r8d
  int v83; // r9d
  _OWORD *v84; // rax
  unsigned int v85; // edx
  __int64 v86; // rdx
  float v87; // xmm10_4
  float v88; // xmm15_4
  void **v89; // rax
  int v90; // ebx
  int v91; // eax
  __int64 v92; // rdx
  int v93; // r15d
  char v94; // di
  float v95; // xmm12_4
  float v96; // xmm6_4
  float v97; // r13d
  void **v98; // rax
  float v99; // xmm14_4
  int v100; // ebx
  int v101; // eax
  float v102; // xmm11_4
  __m128i v103; // xmm1
  __m128 *v104; // rax
  float v105; // xmm1_4
  __m128 v106; // xmm4
  __int128 *v107; // rax
  float v108; // xmm5_4
  float v109; // xmm7_4
  int v110; // ebx
  float v111; // xmm8_4
  float v112; // xmm6_4
  float v113; // xmm2_4
  float v114; // xmm9_4
  float v115; // r9d
  __int128 v116; // xmm0
  int v117; // eax
  float v118; // xmm3_4
  float v119; // xmm11_4
  float v120; // xmm13_4
  float v121; // xmm2_4
  float v122; // xmm7_4
  float v123; // xmm8_4
  float v124; // xmm9_4
  float v125; // xmm3_4
  float v126; // xmm4_4
  float v127; // xmm5_4
  float v128; // xmm6_4
  void **v129; // rax
  char v130; // al
  float v131; // edi
  int v132; // eax
  int v133; // ecx
  int v134; // r9d
  int v135; // r13d
  int v136; // ebx
  int v138; // [rsp+28h] [rbp-E0h]
  int v139; // [rsp+28h] [rbp-E0h]
  float v140; // [rsp+78h] [rbp-90h] BYREF
  float v141; // [rsp+7Ch] [rbp-8Ch] BYREF
  char v142; // [rsp+80h] [rbp-88h]
  float v143; // [rsp+84h] [rbp-84h] BYREF
  float v144; // [rsp+88h] [rbp-80h] BYREF
  float v145; // [rsp+8Ch] [rbp-7Ch] BYREF
  float k; // [rsp+90h] [rbp-78h] BYREF
  __int128 v147; // [rsp+98h] [rbp-70h]
  _DWORD v148[3]; // [rsp+A8h] [rbp-60h] BYREF
  float v149; // [rsp+B4h] [rbp-54h]
  float v150; // [rsp+B8h] [rbp-50h]
  _DWORD v151[3]; // [rsp+C0h] [rbp-48h] BYREF
  float v152; // [rsp+CCh] [rbp-3Ch]
  int v153; // [rsp+D0h] [rbp-38h] BYREF
  int v154; // [rsp+D4h] [rbp-34h]
  float v155; // [rsp+D8h] [rbp-30h]
  unsigned __int64 v156; // [rsp+E0h] [rbp-28h] BYREF
  int v157; // [rsp+E8h] [rbp-20h]
  unsigned __int64 v158; // [rsp+ECh] [rbp-1Ch]
  int v159; // [rsp+F4h] [rbp-14h]
  unsigned __int64 v160; // [rsp+F8h] [rbp-10h]
  int v161; // [rsp+100h] [rbp-8h]
  unsigned __int64 v162; // [rsp+104h] [rbp-4h]
  int v163; // [rsp+10Ch] [rbp+4h]
  unsigned __int64 v164; // [rsp+110h] [rbp+8h] BYREF
  int v165; // [rsp+118h] [rbp+10h]
  unsigned __int64 v166; // [rsp+11Ch] [rbp+14h]
  int v167; // [rsp+124h] [rbp+1Ch]
  unsigned __int64 v168; // [rsp+128h] [rbp+20h]
  int v169; // [rsp+130h] [rbp+28h]
  unsigned __int64 v170; // [rsp+134h] [rbp+2Ch]
  int v171; // [rsp+13Ch] [rbp+34h]
  _QWORD v172[8]; // [rsp+148h] [rbp+40h] BYREF
  _QWORD v173[8]; // [rsp+188h] [rbp+80h] BYREF

  v1 = *(_DWORD *)(a1 + 104); /*0x1801aa4f4*/
  if ( v1 > 0 ) /*0x1801aa539*/
    *(_DWORD *)(a1 + 104) = v1 - 1; /*0x1801aa53d*/
  v3 = *(_DWORD *)(a1 + 24); /*0x1801aa540*/
  if ( dword_1813668E0 >= v3 - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908) ) /*0x1801aa55d*/
    *(_BYTE *)(a1 + 17) = 1; /*0x1801aa55f*/
  if ( *(_DWORD *)(a1 + 300) != 0 ) /*0x1801aa57a*/
  {
    v9 = *(void ***)a1; /*0x1801aa66a*/
    v6 = (float)(180 * *(_DWORD *)(a1 + 316)) / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300); /*0x1801aa671*/
    v150 = v6; /*0x1801aa676*/
    if ( v9 == &VelocityZoneNotes::`vftable' ) /*0x1801aa67f*/
    {
      v10 = *(_DWORD *)(a1 + 48); /*0x1801aa685*/
    }
    else
    {
      v10 = ((__int64 (__fastcall *)(__int64))v9[9])(a1); /*0x1801ab2c8*/
      v9 = *(void ***)a1; /*0x1801ab2ca*/
    }
    if ( v9 == &VelocityZoneNotes::`vftable' ) /*0x1801aa68b*/
      v11 = *(_DWORD *)(a1 + 44); /*0x1801aa691*/
    else
      v11 = ((__int64 (__fastcall *)(__int64))v9[8])(a1); /*0x1801ab2d8*/
    v4 = (_DWORD *)(a1 + 32); /*0x1801aa6bb*/
    sub_180305430( /*0x1801aa6e6*/
      a1: *(_DWORD *)(a1 + 20),
      a2: a1 + 336,
      a3: (int *)(a1 + 28),
      a4: (int *)(a1 + 32),
      a5: nullptr,
      a6: (float *)(a1 + 332),
      a7: (float *)(a1 + 336),
      a8: (float *)(a1 + 340),
      a9: v11,
      a10: v10,
      a11: nullptr,
      a12: nullptr);
    v13 = *(_QWORD *)(a1 + 288); /*0x1801aa6eb*/
    if ( v13 != 0 ) /*0x1801aa6f5*/
      sub_1802B9B90(a1: v13, a2: *(_DWORD *)(a1 + 28), a3: *v4, a4: v12, a5: v138); /*0x1801aa6fe*/
  }
  else
  {
    v4 = (_DWORD *)(a1 + 32); /*0x1801aa583*/
    *(_DWORD *)(a1 + 324) = *(_DWORD *)(a1 + 28); /*0x1801aa587*/
    *(_DWORD *)(a1 + 328) = *(_DWORD *)(a1 + 32); /*0x1801aa591*/
    v5 = *(void ***)a1; /*0x1801aa5c0*/
    v6 = (float)(180 * (v3 - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908))) /*0x1801aa5c7*/
       / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300);
    v150 = v6; /*0x1801aa5cc*/
    if ( v5 == &VelocityZoneNotes::`vftable' ) /*0x1801aa5d5*/
    {
      v7 = *(_DWORD *)(a1 + 48); /*0x1801aa5db*/
    }
    else
    {
      v7 = ((__int64 (__fastcall *)(__int64))v5[9])(a1); /*0x1801ab2aa*/
      v5 = *(void ***)a1; /*0x1801ab2ac*/
    }
    if ( v5 == &VelocityZoneNotes::`vftable' ) /*0x1801aa5e1*/
      v8 = *(_DWORD *)(a1 + 44); /*0x1801aa5e7*/
    else
      v8 = ((__int64 (__fastcall *)(__int64))v5[8])(a1); /*0x1801ab2ba*/
    sub_180305430( /*0x1801aa638*/
      a1: *(_DWORD *)(a1 + 20),
      a2: a1 + 336,
      a3: (int *)(a1 + 28),
      a4: (int *)(a1 + 32),
      a5: nullptr,
      a6: (float *)(a1 + 332),
      a7: (float *)(a1 + 336),
      a8: (float *)(a1 + 340),
      a9: v8,
      a10: v7,
      a11: nullptr,
      a12: nullptr);
    *(_DWORD *)(a1 + 36) = *(_DWORD *)(a1 + 28); /*0x1801aa640*/
    *(_DWORD *)(a1 + 40) = *v4; /*0x1801aa647*/
  }
  v14 = v172; /*0x1801aa709*/
  for ( i = 4; i != 0; --i ) /*0x1801aa70d*/
  {
    unknown_libname_4(a1: v14); /*0x1801aa713*/
    v14 += 2; /*0x1801aa718*/
  }
  v18 = (float *)(a1 + 28); /*0x1801aa728*/
  v19 = *(float *)(a1 + 332); /*0x1801aa72c*/
  v20 = *(_DWORD *)(a1 + 244) >> 1; /*0x1801aa744*/
  v21 = (int)(float)((float)(*(_DWORD *)(a1 + 240) >> 1) * v19); /*0x1801aa74a*/
  v140 = *(float *)&v21; /*0x1801aa751*/
  v22 = (int)(float)((float)v20 * v19); /*0x1801aa75e*/
  v141 = *(float *)&v22; /*0x1801aa762*/
  if ( v21 > 0 && v22 > 0 ) /*0x1801aa770*/
  {
    v23 = *(_DWORD *)v18 - v21; /*0x1801aa778*/
    DWORD2(v147) = 0; /*0x1801aa77a*/
    v24 = _mm_cvtsi32_si128(v23); /*0x1801aa781*/
    v25 = *v4 - v22; /*0x1801aa789*/
    v157 = 0; /*0x1801aa79c*/
    v156 = _mm_unpacklo_ps(_mm_cvtepi32_ps(v24), (__m128)COERCE_UNSIGNED_INT((float)v25)).m128_u64[0]; /*0x1801aa7aa*/
    sub_1800D23B0(a1: (unsigned int)v148, a2: v21, a3: v16, a4: v17, a5: *(_DWORD *)(a1 + 340)); /*0x1801aa7c3*/
    DWORD2(v147) = 0; /*0x1801aa7ce*/
    v26 = *(_DWORD *)v18 + LODWORD(v140) - 1; /*0x1801aa7d5*/
    v28 = *v27; /*0x1801aa7d7*/
    LODWORD(v27) = *v4 - LODWORD(v141); /*0x1801aa7de*/
    *(_OWORD *)v172 = v28; /*0x1801aa7e2*/
    v159 = 0; /*0x1801aa7fb*/
    v158 = _mm_unpacklo_ps((__m128)COERCE_UNSIGNED_INT((float)v26), (__m128)COERCE_UNSIGNED_INT((float)(int)v27)).m128_u64[0]; /*0x1801aa809*/
    sub_1800D23B0(a1: (unsigned int)v148, a2: v29, a3: v30, a4: v31, a5: *(_DWORD *)(a1 + 340)); /*0x1801aa822*/
    DWORD2(v147) = 0; /*0x1801aa82d*/
    v32 = *v4 + LODWORD(v141) - 1; /*0x1801aa834*/
    v34 = *v33; /*0x1801aa838*/
    LODWORD(v33) = *(_DWORD *)v18 - LODWORD(v140); /*0x1801aa83d*/
    *(_OWORD *)&v172[2] = v34; /*0x1801aa841*/
    v161 = 0; /*0x1801aa85a*/
    v160 = _mm_unpacklo_ps((__m128)COERCE_UNSIGNED_INT((float)(int)v33), (__m128)COERCE_UNSIGNED_INT((float)v32)).m128_u64[0]; /*0x1801aa868*/
    sub_1800D23B0(a1: (unsigned int)v148, a2: v35, a3: v36, a4: v37, a5: *(_DWORD *)(a1 + 340)); /*0x1801aa881*/
    DWORD2(v147) = 0; /*0x1801aa88c*/
    v38 = *(_DWORD *)v18 + LODWORD(v140) - 1; /*0x1801aa893*/
    *(_OWORD *)&v172[4] = *v39; /*0x1801aa898*/
    v162 = _mm_unpacklo_ps( /*0x1801aa8b7*/
             (__m128)COERCE_UNSIGNED_INT((float)v38),
             (__m128)COERCE_UNSIGNED_INT((float)(*v4 + LODWORD(v141) - 1))).m128_u64[0];
    v139 = *(_DWORD *)(a1 + 340); /*0x1801aa8d9*/
    v163 = 0; /*0x1801aa8df*/
    sub_1800D23B0(a1: (unsigned int)v148, a2: v40, a3: v41, a4: v42, a5: v139); /*0x1801aa8e2*/
    *(_OWORD *)&v172[6] = *v43; /*0x1801aa8ea*/
  }
  v44 = *(_DWORD *)(a1 + 280); /*0x1801aa8ee*/
  if ( v44 == 1 ) /*0x1801aa8fe*/
  {
    if ( (dword_184AEF6C0 & 1) == 0 ) /*0x1801aa90c*/
    {
      dword_184AEF6C0 |= 1u; /*0x1801aa918*/
      sub_180102B80(a1: qword_184AEF6D0, a2: 0, a3: 1984); /*0x1801aa91e*/
      atexit(a1: sub_1803A4DF0); /*0x1801aa92a*/
    }
    *(_DWORD *)(a1 + 280) = sub_1800D2820( /*0x1801aa965*/
                              a1: a1 + 128,
                              a2: qword_184AEF6D0[*(int *)(a1 + 208)],
                              a3: 1342177280,
                              a4: &v156,
                              a5: v172,
                              a6: nullptr);
    (*(void (__fastcall **)(__int64))(*(_QWORD *)(a1 + 128) + 32LL))(a1: a1 + 128); /*0x1801aa979*/
  }
  else
  {
    sub_1800D3060(a1: a1 + 128, a2: v44, a3: (__int64)&v156); /*0x1801aa989*/
    sub_1800D2EC0(a1: a1 + 128, a2: *(unsigned int *)(a1 + 280), a3: v172); /*0x1801aa99f*/
  }
  v46 = 180 * (*(_DWORD *)(a1 + 296) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908)); /*0x1801aa9c4*/
  k = 1.0; /*0x1801aa9ce*/
  v144 = 0.0; /*0x1801aa9d5*/
  v145 = 0.0; /*0x1801aa9dc*/
  v47 = _mm_cvtsi32_si128(v46); /*0x1801aa9e3*/
  v48 = *(void ***)a1; /*0x1801aa9e8*/
  v49 = _mm_cvtepi32_ps(v47).m128_f32[0] / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300); /*0x1801aa9ef*/
  if ( *(void ***)a1 == &VelocityZoneNotes::`vftable' ) /*0x1801aa9f7*/
  {
    v50 = *(_DWORD *)(a1 + 48); /*0x1801aa9fd*/
  }
  else
  {
    v50 = ((__int64 (__fastcall *)(__int64))v48[9])(a1); /*0x1801ab2e6*/
    v48 = *(void ***)a1; /*0x1801ab2e8*/
  }
  if ( v48 == &VelocityZoneNotes::`vftable' ) /*0x1801aaa03*/
    v51 = *(_DWORD *)(a1 + 44); /*0x1801aaa09*/
  else
    v51 = ((__int64 (__fastcall *)(__int64))v48[8])(a1); /*0x1801ab2f3*/
  sub_180305430( /*0x1801aaa57*/
    a1: *(_DWORD *)(a1 + 20),
    a2: v45,
    a3: (int *)&v140,
    a4: (int *)&v141,
    a5: nullptr,
    a6: &k,
    a7: &v144,
    a8: &v145,
    a9: v51,
    a10: v50,
    a11: nullptr,
    a12: nullptr);
  if ( *(_DWORD *)(a1 + 300) == 0 ) /*0x1801aaa63*/
  {
    v53 = v141; /*0x1801aaa7a*/
    goto LABEL_35; /*0x1801aaa7a*/
  }
  v52 = *(float *)v4; /*0x1801aaa65*/
  v53 = v141; /*0x1801aaa69*/
  if ( SLODWORD(v141) <= SLODWORD(v52) ) /*0x1801aaa71*/
  {
LABEL_35:
    v54 = v140; /*0x1801aaa7f*/
    goto LABEL_36; /*0x1801aaa7f*/
  }
  v53 = v52; /*0x1801aaa73*/
  v54 = *v18; /*0x1801aaa76*/
LABEL_36:
  v140 = v54; /*0x1801aaa83*/
  v55 = v173; /*0x1801aaa87*/
  for ( j = 4; j != 0; --j ) /*0x1801aaa8e*/
  {
    unknown_libname_4(a1: v55); /*0x1801aaa94*/
    v55 += 2; /*0x1801aaa99*/
  }
  v60 = k; /*0x1801aaaac*/
  v61 = LODWORD(v144); /*0x1801aaab2*/
  v62 = v145; /*0x1801aaab8*/
  v63 = v140; /*0x1801aaabe*/
  v64 = *(_DWORD *)(a1 + 276) >> 1; /*0x1801aaae8*/
  v65 = (int)(float)((float)(*(_DWORD *)(a1 + 272) >> 1) * k); /*0x1801aaaef*/
  v143 = *(float *)&v65; /*0x1801aaaf6*/
  v66 = (int)(float)((float)v64 * k); /*0x1801aab04*/
  if ( v65 > 0 && v66 > 0 ) /*0x1801aab12*/
  {
    DWORD2(v147) = 0; /*0x1801aab1a*/
    v67 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v140) - v65)); /*0x1801aab44*/
    v68 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v53) - v66)); /*0x1801aab53*/
    v165 = 0; /*0x1801aab56*/
    v164 = _mm_unpacklo_ps(v67, v68).m128_u64[0]; /*0x1801aab5c*/
    sub_1800D23B0(a1: (unsigned int)v148, a2: v57, a3: v58, a4: v59, a5: LODWORD(v145)); /*0x1801aab61*/
    DWORD2(v147) = 0; /*0x1801aab66*/
    *(_OWORD *)v173 = *v69; /*0x1801aab8f*/
    v70 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v63) + LODWORD(v143) - 1)); /*0x1801aab9d*/
    v167 = 0; /*0x1801aaba0*/
    v166 = _mm_unpacklo_ps(v70, v68).m128_u64[0]; /*0x1801aaba9*/
    sub_1800D23B0(a1: (unsigned int)v148, a2: v71, a3: v72, a4: v73, a5: LODWORD(v62)); /*0x1801aabae*/
    DWORD2(v147) = 0; /*0x1801aabb3*/
    *(_OWORD *)&v173[2] = *v74; /*0x1801aabda*/
    v75 = (__m128)COERCE_UNSIGNED_INT((float)(LODWORD(v53) + v66 - 1)); /*0x1801aabec*/
    v169 = 0; /*0x1801aabef*/
    v168 = _mm_unpacklo_ps(v67, v75).m128_u64[0]; /*0x1801aabf5*/
    sub_1800D23B0(a1: (unsigned int)v148, a2: v76, a3: v77, a4: v78, a5: LODWORD(v62)); /*0x1801aabfa*/
    DWORD2(v147) = 0; /*0x1801aabff*/
    v80 = *v79; /*0x1801aac1d*/
    v171 = 0; /*0x1801aac23*/
    *(_OWORD *)&v173[4] = v80; /*0x1801aac26*/
    v170 = _mm_unpacklo_ps(v70, v75).m128_u64[0]; /*0x1801aac33*/
    sub_1800D23B0(a1: (unsigned int)v148, a2: v81, a3: v82, a4: v83, a5: LODWORD(v62)); /*0x1801aac38*/
    *(_OWORD *)&v173[6] = *v84; /*0x1801aac40*/
  }
  v85 = *(_DWORD *)(a1 + 284); /*0x1801aac47*/
  if ( v85 == 1 ) /*0x1801aac50*/
  {
    if ( (dword_184AEF6C0 & 1) == 0 ) /*0x1801aac5e*/
    {
      dword_184AEF6C0 |= 1u; /*0x1801aac6b*/
      sub_180102B80(a1: qword_184AEF6D0, a2: 0, a3: 1984); /*0x1801aac77*/
      atexit(a1: sub_1803A4DF0); /*0x1801aac83*/
    }
    *(_DWORD *)(a1 + 284) = sub_1800D2820( /*0x1801aacc1*/
                              a1: a1 + 168,
                              a2: qword_184AEF6D0[*(int *)(a1 + 212)],
                              a3: 1342177280,
                              a4: &v164,
                              a5: v173,
                              a6: nullptr);
    (*(void (__fastcall **)(__int64))(*(_QWORD *)(a1 + 168) + 32LL))(a1: a1 + 168); /*0x1801aacd5*/
  }
  else
  {
    sub_1800D3060(a1: a1 + 168, a2: v85, a3: (__int64)&v164); /*0x1801aace5*/
    sub_1800D2EC0(a1: a1 + 168, a2: *(unsigned int *)(a1 + 284), a3: v173); /*0x1801aacfe*/
  }
  (*(void (__fastcall **)(__int64))(*(_QWORD *)(a1 + 352) + 24LL))(a1: a1 + 352); /*0x1801aad0d*/
  v87 = v49 - *(float *)&dword_1813E4758; /*0x1801aad18*/
  v145 = v60; /*0x1801aad25*/
  v88 = *(float *)&dword_1813668D4 / 30.0; /*0x1801aad2b*/
  v141 = v63; /*0x1801aad34*/
  v143 = v53; /*0x1801aad38*/
  v144 = *(float *)&v61; /*0x1801aad41*/
  for ( k = v62; v87 > *(float *)&dword_1813668D4; v87 = v87 - v88 ) /*0x1801aad4d*/
    ; /*0x1801aad50*/
  v89 = *(void ***)a1; /*0x1801aad5b*/
  if ( *(void ***)a1 == &VelocityZoneNotes::`vftable' ) /*0x1801aad61*/
  {
    v90 = *(_DWORD *)(a1 + 48); /*0x1801aad67*/
  }
  else
  {
    v90 = ((__int64 (__fastcall *)(__int64))v89[9])(a1); /*0x1801ab301*/
    v89 = *(void ***)a1; /*0x1801ab303*/
  }
  if ( v89 == &VelocityZoneNotes::`vftable' ) /*0x1801aad6d*/
    v91 = *(_DWORD *)(a1 + 44); /*0x1801aad73*/
  else
    v91 = ((__int64 (__fastcall *)(__int64))v89[8])(a1); /*0x1801ab30e*/
  sub_180305430( /*0x1801aadb9*/
    a1: *(_DWORD *)(a1 + 20),
    a2: v86,
    a3: (int *)&v141,
    a4: (int *)&v143,
    a5: nullptr,
    a6: &v145,
    a7: &v144,
    a8: &k,
    a9: v91,
    a10: v90,
    a11: nullptr,
    a12: nullptr);
  v93 = 4 /*0x1801aae1d*/
      - (int)(float)((float)((float)(v87
                                   - (float)((float)(180
                                                   * (*(_DWORD *)(a1 + 24)
                                                    + *(_DWORD *)(a1 + 316)
                                                    - dword_18070A610
                                                    - HIDWORD(qword_181366914)
                                                    - HIDWORD(qword_181366908)))
                                           / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300)))
                           / v88)
                   + 0.5)
      % 4;
  v94 = 1; /*0x1801aae20*/
  v142 = 1; /*0x1801aae23*/
  v154 = v93 + 37; /*0x1801aae2c*/
  if ( !__OFSUB__(v93, v93 + 37) ) /*0x1801aae32*/
  {
    v95 = v144; /*0x1801aae3d*/
    v96 = v145; /*0x1801aae47*/
    v97 = v141; /*0x1801aae4c*/
    v140 = k; /*0x1801aae51*/
    v144 = v143; /*0x1801aae60*/
    do /*0x1801ab1b1*/
    {
      v98 = *(void ***)a1; /*0x1801aae70*/
      v143 = 0.0; /*0x1801aae77*/
      v145 = 0.0; /*0x1801aae84*/
      v141 = 0.0; /*0x1801aae8f*/
      v99 = fmaxf(v6, v87 - v88); /*0x1801aae97*/
      if ( v98 == &VelocityZoneNotes::`vftable' ) /*0x1801aae9f*/
      {
        v100 = *(_DWORD *)(a1 + 48); /*0x1801aaea5*/
      }
      else
      {
        v100 = ((__int64 (__fastcall *)(__int64))v98[9])(a1); /*0x1801ab31c*/
        v98 = *(void ***)a1; /*0x1801ab31e*/
      }
      if ( v98 == &VelocityZoneNotes::`vftable' ) /*0x1801aaeab*/
        v101 = *(_DWORD *)(a1 + 44); /*0x1801aaeb1*/
      else
        v101 = ((__int64 (__fastcall *)(__int64))v98[8])(a1); /*0x1801ab329*/
      sub_180305430( /*0x1801aaef7*/
        a1: *(_DWORD *)(a1 + 20),
        a2: v92,
        a3: (int *)&k,
        a4: &v153,
        a5: nullptr,
        a6: &v143,
        a7: &v145,
        a8: &v141,
        a9: v101,
        a10: v100,
        a11: nullptr,
        a12: nullptr);
      v102 = *(float *)&dword_18070B300 * *(float *)&dword_18070B300; /*0x1801aaf25*/
      v103 = _mm_cvtsi32_si128(*(_DWORD *)(a1 + 60) / 2 - 10); /*0x1801aaf31*/
      v104 = (__m128 *)&unk_1812D33C8; /*0x1801aaf35*/
      if ( *(_DWORD *)(a1 + 344) == 1 ) /*0x1801aaf3c*/
        v104 = (__m128 *)&unk_1812D3398; /*0x1801aaf3c*/
      v105 = _mm_cvtepi32_ps(v103).m128_f32[0]; /*0x1801aaf46*/
      v106 = *v104; /*0x1801aaf49*/
      v107 = (__int128 *)&unk_1812D33D8; /*0x1801aaf4c*/
      if ( *(_DWORD *)(a1 + 344) == 1 ) /*0x1801aaf53*/
        v107 = (__int128 *)&unk_1812D33A8; /*0x1801aaf53*/
      v108 = _mm_shuffle_ps(v106, v106, 85).m128_f32[0]; /*0x1801aaf67*/
      v109 = v106.m128_f32[0]; /*0x1801aaf6b*/
      v110 = (int)(float)(v96 * v105); /*0x1801aaf6e*/
      v111 = v108; /*0x1801aaf78*/
      v155 = v143; /*0x1801aaf7f*/
      v112 = _mm_shuffle_ps(v106, v106, 170).m128_f32[0]; /*0x1801aaf8b*/
      v113 = _mm_shuffle_ps(v106, v106, 255).m128_f32[0]; /*0x1801aaf8f*/
      v114 = v112; /*0x1801aaf93*/
      LODWORD(v115) = (int)(float)(v143 * v105); /*0x1801aaf97*/
      v116 = *v107; /*0x1801aaf9c*/
      v117 = *(_DWORD *)(a1 + 24); /*0x1801aaf9f*/
      v143 = v115; /*0x1801aafa2*/
      v118 = v113; /*0x1801aafa7*/
      v147 = v116; /*0x1801aafb7*/
      if ( v88 <= (float)(v99 /*0x1801aafd3*/
                        - (float)((float)(180
                                        * (v117 - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908)))
                                / v102)) )
      {
        if ( v88 > (float)((float)((float)(180 /*0x1801ab02d*/
                                         * (*(_DWORD *)(a1 + 296)
                                          - dword_18070A610
                                          - HIDWORD(qword_181366914)
                                          - HIDWORD(qword_181366908)))
                                 / v102)
                         - v87) )
        {
          v109 = v106.m128_f32[0]; /*0x1801ab02f*/
          v111 = v108; /*0x1801ab032*/
          v106.m128_i32[0] = v147; /*0x1801ab036*/
          v114 = v112; /*0x1801ab03b*/
          v108 = *((float *)&v147 + 1); /*0x1801ab03f*/
          v118 = v113; /*0x1801ab044*/
          v112 = *((float *)&v147 + 2); /*0x1801ab047*/
          v113 = *((float *)&v147 + 3); /*0x1801ab04c*/
        }
      }
      else
      {
        v109 = *(float *)&v147; /*0x1801aafd5*/
        v111 = *((float *)&v147 + 1); /*0x1801aafda*/
        v114 = *((float *)&v147 + 2); /*0x1801aafe0*/
        v118 = *((float *)&v147 + 3); /*0x1801aafe6*/
        if ( v94 == 0 ) /*0x1801aafee*/
        {
          v106.m128_i32[0] = v147; /*0x1801aaff0*/
          v108 = *((float *)&v147 + 1); /*0x1801aaff3*/
          v112 = *((float *)&v147 + 2); /*0x1801aaff7*/
          v113 = *((float *)&v147 + 3); /*0x1801aaffb*/
        }
        v142 = 0; /*0x1801aaffe*/
      }
      v119 = v145; /*0x1801ab051*/
      v120 = v141; /*0x1801ab063*/
      if ( v145 >= 1.0 ) /*0x1801ab06a*/
      {
        v125 = v118 * v140; /*0x1801ab097*/
        v126 = v106.m128_f32[0] * v145; /*0x1801ab09d*/
        v127 = v108 * v145; /*0x1801ab0a2*/
        v128 = v112 * v145; /*0x1801ab0a7*/
        v121 = v113 * v141; /*0x1801ab0ac*/
        v122 = v109 * v95; /*0x1801ab0b1*/
        v123 = v111 * v95; /*0x1801ab0b6*/
        v124 = v114 * v95; /*0x1801ab0bb*/
      }
      else
      {
        v121 = v113 * v140; /*0x1801ab06c*/
        v122 = v109 * v145; /*0x1801ab072*/
        v123 = v111 * v145; /*0x1801ab077*/
        v124 = v114 * v145; /*0x1801ab07c*/
        v125 = v118 * v141; /*0x1801ab081*/
        v126 = v106.m128_f32[0] * v95; /*0x1801ab086*/
        v127 = v108 * v95; /*0x1801ab08b*/
        v128 = v112 * v95; /*0x1801ab090*/
      }
      v129 = *(void ***)a1; /*0x1801ab0c0*/
      v152 = v125; /*0x1801ab0c3*/
      *(float *)&v151[2] = v124; /*0x1801ab0c8*/
      *(float *)&v151[1] = v123; /*0x1801ab0ce*/
      *(float *)v151 = v122; /*0x1801ab0d4*/
      v149 = v121; /*0x1801ab0d9*/
      *(float *)&v148[2] = v128; /*0x1801ab0de*/
      *(float *)&v148[1] = v127; /*0x1801ab0e3*/
      *(float *)v148 = v126; /*0x1801ab0e8*/
      if ( v129 == &VelocityZoneNotes::`vftable' ) /*0x1801ab0f0*/
      {
        v130 = *(_BYTE *)(*(_QWORD *)(a1 + 8) + 97LL); /*0x1801ab0fa*/
      }
      else
      {
        v130 = ((__int64 (__fastcall *)(__int64))v129[24])(a1); /*0x1801ab28a*/
        v125 = v152; /*0x1801ab290*/
        v121 = v149; /*0x1801ab295*/
        v115 = v143; /*0x1801ab29a*/
      }
      if ( v130 != 0 ) /*0x1801ab100*/
      {
        v152 = v125 * 0.25; /*0x1801ab112*/
        v149 = v121 * 0.25; /*0x1801ab117*/
      }
      v131 = k; /*0x1801ab11c*/
      v132 = LODWORD(k) - LODWORD(v115); /*0x1801ab12a*/
      v133 = LODWORD(v115) + LODWORD(k); /*0x1801ab13f*/
      v134 = v110 + LODWORD(v97); /*0x1801ab14c*/
      v135 = LODWORD(v97) - v110; /*0x1801ab150*/
      v136 = v153; /*0x1801ab153*/
      sub_1800D5370( /*0x1801ab17a*/
        a1: (_DWORD *)(a1 + 352),
        a2: v135,
        a3: SLODWORD(v144),
        a4: v134,
        a5: SLODWORD(v144),
        a6: v133,
        a7: v153,
        a8: v132,
        a9: v153,
        a10: (__int64)v148,
        a11: (__int64)v151,
        a12: (__int64)v148,
        a13: (__int64)v151);
      v6 = v150; /*0x1801ab17f*/
      v87 = v99; /*0x1801ab185*/
      v97 = v131; /*0x1801ab18d*/
      v144 = *(float *)&v136; /*0x1801ab190*/
      v140 = v120; /*0x1801ab193*/
      v95 = v119; /*0x1801ab19a*/
      if ( v150 >= v99 ) /*0x1801ab19e*/
        break; /*0x1801ab19e*/
      v96 = v155; /*0x1801ab1a0*/
      ++v93; /*0x1801ab1a5*/
      v94 = v142; /*0x1801ab1a8*/
    }
    while ( v93 < v154 ); /*0x1801ab1b1*/
  }
  (*(void (__fastcall **)(__int64))(*(_QWORD *)(a1 + 352) + 32LL))(a1: a1 + 352); /*0x1801ab1ca*/
  if ( *(_DWORD *)(a1 + 300) == 1 ) /*0x1801ab253*/
  {
    if ( *(_BYTE *)(a1 + 308) == 0 ) /*0x1801ab25c*/
      *(_BYTE *)(a1 + 309) = 1; /*0x1801ab25e*/
    *(_BYTE *)(a1 + 308) = 0; /*0x1801ab265*/
  }
  return 0; /*0x1801ab26e*/
}
