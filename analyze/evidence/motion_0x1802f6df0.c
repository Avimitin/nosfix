char __fastcall sub_1802F6DF0(__int64 a1)
{
  int v1; // eax
  unsigned int v3; // r14d
  int v4; // r15d
  int v5; // edx
  int v6; // ecx
  __int64 v7; // rbx
  unsigned int v8; // r12d
  int v9; // eax
  int v10; // edx
  int v11; // r8d
  int v12; // r9d
  int v13; // ecx
  __int64 v14; // rdx
  void **v15; // rax
  int v16; // edi
  int v17; // eax
  __int64 v18; // rcx
  char v19; // al
  float v20; // xmm7_4
  _QWORD *v21; // rdi
  __int64 v22; // rcx
  int v23; // r12d
  __int64 v24; // rax
  int v25; // edx
  int v26; // r8d
  int v27; // r9d
  int v28; // edi
  int v29; // eax
  __int128 *v30; // rax
  __int128 v31; // xmm0
  int v32; // ecx
  int v33; // edx
  int v34; // r8d
  int v35; // r9d
  int v36; // ecx
  __int128 *v37; // rax
  __int128 v38; // xmm0
  int v39; // edx
  int v40; // r8d
  int v41; // r9d
  int v42; // ecx
  _OWORD *v43; // rax
  __m128 v44; // xmm2
  int v45; // edx
  int v46; // r8d
  int v47; // r9d
  _OWORD *v48; // rax
  __int64 v49; // rbx
  __int64 v50; // rax
  int v51; // eax
  int v52; // eax
  int v53; // edi
  int v54; // edx
  __int64 v55; // rax
  int v57; // [rsp+60h] [rbp-A0h]
  float v58; // [rsp+64h] [rbp-9Ch] BYREF
  float v59; // [rsp+68h] [rbp-98h] BYREF
  int v60; // [rsp+6Ch] [rbp-94h] BYREF
  char v61; // [rsp+70h] [rbp-90h] BYREF
  int v62; // [rsp+78h] [rbp-88h]
  unsigned __int64 v63; // [rsp+80h] [rbp-80h] BYREF
  int v64; // [rsp+88h] [rbp-78h]
  unsigned __int64 v65; // [rsp+8Ch] [rbp-74h]
  int v66; // [rsp+94h] [rbp-6Ch]
  unsigned __int64 v67; // [rsp+98h] [rbp-68h]
  int v68; // [rsp+A0h] [rbp-60h]
  unsigned __int64 v69; // [rsp+A4h] [rbp-5Ch]
  int v70; // [rsp+ACh] [rbp-54h]
  _QWORD v71[8]; // [rsp+B0h] [rbp-50h] BYREF

  v1 = *(_DWORD *)(a1 + 104); /*0x1802f6e17*/
  if ( v1 > 0 ) /*0x1802f6e1f*/
    *(_DWORD *)(a1 + 104) = v1 - 1; /*0x1802f6e23*/
  if ( HIBYTE(word_1813E5720) != 0 ) /*0x1802f6e2d*/
  {
    v3 = 0; /*0x1802f6e65*/
    v4 = 0; /*0x1802f6e68*/
  }
  else
  {
    v3 = 0; /*0x1802f6e37*/
    v4 = -2; /*0x1802f6e3a*/
    v5 = *(_DWORD *)(sub_18018B090(a1: *(unsigned int *)(a1 + 64)) + 88); /*0x1802f6e43*/
    if ( v6 < 4 ) /*0x1802f6e46*/
      v4 = 0; /*0x1802f6e46*/
    if ( v6 == 3 ) /*0x1802f6e52*/
      v4 = -1; /*0x1802f6e52*/
    if ( v5 > 0 ) /*0x1802f6e58*/
      v5 = 0; /*0x1802f6e58*/
    if ( v5 > v4 ) /*0x1802f6e5f*/
      v4 = v5; /*0x1802f6e5f*/
  }
  v57 = 0; /*0x1802f6e6b*/
  v7 = 4; /*0x1802f6e70*/
  v8 = 2; /*0x1802f6e75*/
  if ( v4 < 0 ) /*0x1802f6e7c*/
  {
    v9 = *(_DWORD *)(a1 + 64); /*0x1802f6e7e*/
    if ( v9 > 4 && v4 + v9 <= 4 ) /*0x1802f6e8a*/
      v8 = 4; /*0x1802f6e8a*/
  }
  if ( (unsigned int)(*(_DWORD *)(a1 + 64) % 2) <= 1 && *(int *)(a1 + 64) >= 3 && v4 == -1 ) /*0x1802f6eaf*/
    v57 = dword_1813E476C / -2; /*0x1802f6ebe*/
  v10 = *(_DWORD *)(a1 + 24); /*0x1802f6ec2*/
  v11 = dword_18070A610; /*0x1802f6ec7*/
  v12 = HIDWORD(qword_181366908); /*0x1802f6ed1*/
  v13 = v10 - dword_18070A610 - HIDWORD(qword_181366908); /*0x1802f6ed8*/
  if ( dword_1813668F8 >= v13 && v13 >= dword_1813668FC /*0x1802f6f0d*/
    || dword_1813668F0 >= v13 && v13 >= dword_1813668F4
    || dword_1813668E8 >= v13 && v13 >= dword_1813668EC
    || v13 < 0 )
  {
    *(_BYTE *)(a1 + 17) = 1; /*0x1802f6f0f*/
    v12 = HIDWORD(qword_181366908); /*0x1802f6f13*/
    v11 = dword_18070A610; /*0x1802f6f1a*/
  }
  if ( *(_DWORD *)(*(_QWORD *)(a1 + 8) + 48LL) != 2 )
  {
    v14 = (unsigned int)(v10 - v11 - HIDWORD(qword_181366914) - v12); /*0x1802f6f40*/
    v59 = 0.0; /*0x1802f6f6c*/
    v60 = 0; /*0x1802f6f74*/
    v58 = 0.0; /*0x1802f6f7c*/
    v15 = *(void ***)a1; /*0x1802f6f88*/
    if ( *(void ***)a1 == &NormalNotes::`vftable' ) /*0x1802f6f95*/
    {
      v16 = *(_DWORD *)(a1 + 48); /*0x1802f6f9b*/
    }
    else
    {
      v16 = ((__int64 (__fastcall *)(__int64))v15[9])(a1); /*0x1802f7354*/
      v15 = *(void ***)a1; /*0x1802f7356*/
    }
    v17 = v15 == &NormalNotes::`vftable' ? *(_DWORD *)(a1 + 44) : ((__int64 (__fastcall *)(__int64))v15[8])(a1);
    sub_180305430( /*0x1802f6fed*/
      a1: *(_DWORD *)(a1 + 20),
      a2: v14,
      a3: (int *)(a1 + 28),
      a4: (int *)(a1 + 32),
      a5: nullptr,
      a6: &v59,
      a7: (float *)&v60,
      a8: &v58,
      a9: v17,
      a10: v16,
      a11: nullptr,
      a12: nullptr);
    v18 = *(_QWORD *)(a1 + 8); /*0x1802f6ff5*/
    *(_DWORD *)(a1 + 36) = *(_DWORD *)(a1 + 28); /*0x1802f6ff9*/
    *(_DWORD *)(a1 + 40) = *(_DWORD *)(a1 + 32); /*0x1802f6fff*/
    if ( *(_DWORD *)(v18 + 48) != 2 && BYTE1(dword_1813E473C) != 0 ) /*0x1802f7013*/
    {
      if ( *(void ***)a1 == &NormalNotes::`vftable' ) /*0x1802f7026*/
        v19 = *(_BYTE *)(v18 + 97); /*0x1802f702c*/
      else
        v19 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)a1 + 192LL))(a1); /*0x1802f736c*/
      v20 = v58; /*0x1802f7038*/
      if ( v19 != 0 ) /*0x1802f7040*/
        v20 = v58 * 0.25; /*0x1802f7042*/
      v21 = v71; /*0x1802f704a*/
      do /*0x1802f7060*/
      {
        unknown_libname_4(a1: v21); /*0x1802f7053*/
        v21 += 2; /*0x1802f7058*/
        --v7; /*0x1802f705c*/
      }
      while ( v7 != 0 ); /*0x1802f7060*/
      v22 = *(_DWORD *)(a1 + 196) / v8; /*0x1802f7070*/
      v23 = (int)(float)((float)(int)v22 * v59); /*0x1802f707d*/
      v24 = sub_18018B090(a1: v22); /*0x1802f7082*/
      v28 = (int)(float)((float)((float)(*(_DWORD *)(a1 + 200) >> 1) * v59) * (float)((float)*(int *)(v24 + 84) / 10.0)); /*0x1802f70b1*/
      if ( v23 > 0 && v28 > 0 ) /*0x1802f70c0*/
      {
        v29 = *(_DWORD *)(a1 + 28) + v57 - v23; /*0x1802f70e1*/
        v62 = 0; /*0x1802f70e7*/
        v64 = 0; /*0x1802f70eb*/
        v63 = _mm_unpacklo_ps( /*0x1802f7109*/
                (__m128)COERCE_UNSIGNED_INT((float)v29),
                (__m128)COERCE_UNSIGNED_INT((float)(*(_DWORD *)(a1 + 32) - v28))).m128_u64[0];
        sub_1800D23B0(a1: (unsigned int)&v61, a2: v25, a3: v26, a4: v27, a5: LODWORD(v20)); /*0x1802f710e*/
        v31 = *v30; /*0x1802f7126*/
        LODWORD(v30) = *(_DWORD *)(a1 + 28) - 1; /*0x1802f712c*/
        v62 = 0; /*0x1802f712e*/
        v32 = (_DWORD)v30 + v23 + v57; /*0x1802f7132*/
        *(_OWORD *)v71 = v31; /*0x1802f7134*/
        LODWORD(v30) = *(_DWORD *)(a1 + 32) - v28; /*0x1802f713b*/
        v66 = 0; /*0x1802f713d*/
        v65 = _mm_unpacklo_ps((__m128)COERCE_UNSIGNED_INT((float)v32), (__m128)COERCE_UNSIGNED_INT((float)(int)v30)).m128_u64[0]; /*0x1802f715b*/
        sub_1800D23B0(a1: (unsigned int)&v61, a2: v33, a3: v34, a4: v35, a5: LODWORD(v20)); /*0x1802f7160*/
        v36 = *(_DWORD *)(a1 + 32) - 1; /*0x1802f716b*/
        v38 = *v37; /*0x1802f7178*/
        v62 = 0; /*0x1802f7182*/
        LODWORD(v37) = *(_DWORD *)(a1 + 28) + v57 - v23; /*0x1802f7186*/
        *(_OWORD *)&v71[2] = v38; /*0x1802f7189*/
        v68 = 0; /*0x1802f7199*/
        v67 = _mm_unpacklo_ps( /*0x1802f71ab*/
                (__m128)COERCE_UNSIGNED_INT((float)(int)v37),
                (__m128)COERCE_UNSIGNED_INT((float)(v28 + v36))).m128_u64[0];
        sub_1800D23B0(a1: (unsigned int)&v61, a2: v39, a3: v40, a4: v41, a5: LODWORD(v20)); /*0x1802f71b0*/
        v42 = *(_DWORD *)(a1 + 28) - 1 + v23 + v57; /*0x1802f71c4*/
        *(_OWORD *)&v71[4] = *v43; /*0x1802f71c6*/
        v44 = (__m128)COERCE_UNSIGNED_INT((float)(v28 + *(_DWORD *)(a1 + 32) - 1)); /*0x1802f71dc*/
        v62 = 0; /*0x1802f71e6*/
        v70 = 0; /*0x1802f71ea*/
        v69 = _mm_unpacklo_ps((__m128)COERCE_UNSIGNED_INT((float)v42), v44).m128_u64[0]; /*0x1802f71ff*/
        sub_1800D23B0(a1: (unsigned int)&v61, a2: v45, a3: v46, a4: v47, a5: LODWORD(v20)); /*0x1802f7204*/
        *(_OWORD *)&v71[6] = *v48; /*0x1802f720c*/
      }
      if ( *(_DWORD *)(a1 + 204) == 1 ) /*0x1802f721f*/
      {
        v49 = *(int *)(a1 + 168); /*0x1802f7221*/
        v50 = sub_180339800(); /*0x1802f7228*/
        *(_DWORD *)(a1 + 204) = sub_1800D2820( /*0x1802f7255*/
                                  a1: a1 + 128,
                                  a2: *(_QWORD *)(v50 + 8 * v49),
                                  a3: 1342177280,
                                  a4: &v63,
                                  a5: v71,
                                  a6: nullptr);
        (*(void (__fastcall **)(__int64))(*(_QWORD *)(a1 + 128) + 32LL))(a1: a1 + 128); /*0x1802f7269*/
      }
      else
      {
        if ( HIDWORD(qword_1813E4730) == 1 ) /*0x1802f7278*/
        {
          v51 = *(_DWORD *)(a1 + 88); /*0x1802f727a*/
          if ( v51 == 1 ) /*0x1802f7280*/
          {
            v3 = 6; /*0x1802f7282*/
          }
          else if ( v51 == 3 ) /*0x1802f7290*/
          {
            v3 = 7; /*0x1802f7290*/
          }
        }
        v52 = sub_180301CB0(a1: v3, a2: *(unsigned int *)(*(_QWORD *)(a1 + 8) + 48LL)); /*0x1802f729e*/
        *(_DWORD *)(a1 + 168) = v52; /*0x1802f72a3*/
        v53 = v52; /*0x1802f72a9*/
        v54 = v4 + v52 + *(_DWORD *)(a1 + 64); /*0x1802f72b0*/
        if ( v52 + 9 < v54 ) /*0x1802f72b8*/
          v54 = v52 + 9; /*0x1802f72b8*/
        if ( v54 > v52 ) /*0x1802f72bd*/
          v53 = v54; /*0x1802f72bd*/
        *(_DWORD *)(a1 + 168) = v53; /*0x1802f72c0*/
        v55 = sub_180339800(); /*0x1802f72c6*/
        sub_1800D30C0(a1: a1 + 128, a2: *(unsigned int *)(a1 + 204), a3: *(_QWORD *)(v55 + 8LL * v53)); /*0x1802f72df*/
        sub_1800D3060(a1: a1 + 128, a2: *(_DWORD *)(a1 + 204), a3: (__int64)&v63); /*0x1802f72f5*/
        sub_1800D2EC0(a1: a1 + 128, a2: *(unsigned int *)(a1 + 204), a3: v71); /*0x1802f730b*/
      }
    }
  }
  return 0; /*0x1802f732a*/
}
