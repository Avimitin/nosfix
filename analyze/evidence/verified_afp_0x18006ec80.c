__int64 __fastcall sub_18006EC80(unsigned __int64 a1, int a2)
{
  __int64 v2; // rbx
  __int64 v3; // rbp
  __int64 v4; // r15
  unsigned __int64 v5; // r14
  int v6; // r12d
  __int64 v8; // rax
  __int64 v9; // rdx
  int v10; // ecx
  int v11; // ecx
  __int64 v12; // r13
  __int64 v13; // rdi
  int v14; // ecx
  int v15; // r8d
  int v16; // r9d
  int v17; // r8d
  int v18; // r9d
  __int64 v19; // rsi
  int v20; // r8d
  int v21; // r9d
  int v22; // eax
  int v23; // r8d
  int v24; // r9d
  float v25; // xmm1_4
  __m128 v26; // xmm0
  int v27; // eax
  __m128 v28; // xmm8
  int v29; // esi
  float v30; // xmm1_4
  float v31; // xmm0_4
  __int64 v32; // r8
  char v33; // al
  char v34; // al
  unsigned int v35; // edx
  __int64 v36; // r9
  unsigned int v37; // ecx
  unsigned int v38; // esi
  __m128 v39; // xmm0
  __m128 v40; // xmm0
  __int64 v41; // rsi
  __int64 v42; // rcx
  __int64 v43; // rdx
  __int64 v44; // rbx
  _BYTE *v45; // rbp
  __int64 v46; // r15
  __int64 v47; // rbp
  __int16 v48; // r8
  __int64 v49; // r15
  __int64 v50; // rbp
  __int16 v51; // r8
  __int64 v52; // rax
  __int64 v53; // [rsp+30h] [rbp-118h] BYREF
  int v54; // [rsp+38h] [rbp-110h]
  _DWORD v55[3]; // [rsp+3Ch] [rbp-10Ch] BYREF
  _DWORD v56[2]; // [rsp+48h] [rbp-100h] BYREF
  _BYTE v57[64]; // [rsp+50h] [rbp-F8h] BYREF
  __m128 v58; // [rsp+90h] [rbp-B8h]
  _DWORD v59[8]; // [rsp+A0h] [rbp-A8h] BYREF
  __int64 v60; // [rsp+C0h] [rbp-88h]
  __int64 v61; // [rsp+C8h] [rbp-80h]
  __int64 v62; // [rsp+D0h] [rbp-78h]

  v5 = a1; /*0x18006ec8f*/
  v6 = a2; /*0x18006ec9b*/
  if ( a2 != *(__int16 *)(a1 + 8) )
  {
    v8 = *(_QWORD *)(a1 + 56); /*0x18006ed1b*/
    if ( v8 != 0 ) /*0x18006ed22*/
    {
      v9 = (unsigned int)*(__int16 *)(v8 + 14); /*0x18006ed2a*/
      v10 = *(__int16 *)(v8 + 30); /*0x18006ed2e*/
      if ( (int)v9 >= v10 ) /*0x18006ed34*/
        v10 = *(__int16 *)(v8 + 14); /*0x18006ed34*/
      *(_WORD *)(v8 + 30) = v10; /*0x18006ed37*/
      v11 = v10 - 1; /*0x18006ed3b*/
      if ( v11 < v6 ) /*0x18006ed40*/
        v6 = v11; /*0x18006ed40*/
      if ( v6 < 0 ) /*0x18006ed47*/
        v6 = 0; /*0x18006ed47*/
      *(_WORD *)(v5 + 8) = v6; /*0x18006ed4b*/
    }
    else
    {
      v52 = *(_QWORD *)(a1 + 64); /*0x18006f53f*/
      if ( v52 == 0 || (v8 = sub_18004D490(a1: **(_QWORD **)(v52 + 32), a2: 155), *(_WORD *)(v5 + 8) = v6, v8 == 0) ) /*0x18006f561*/
      {
        v53 = 0; /*0x18006f584*/
        v54 = 0; /*0x18006f58d*/
        strcpy((char *)v55, "T\a"); /*0x18006f591*/
        sub_1801285D0(a1: 155, a2: &v53); /*0x18006f59b*/
        return 4294966287LL; /*0x18006f5b9*/
      }
      v6 -= *(__int16 *)(v8 + 28); /*0x18006f567*/
      v9 = (unsigned int)*(__int16 *)(v8 + 14); /*0x18006f56a*/
    }
    v12 = v6 >= (int)v9 ? 0LL : *(_QWORD *)(*(_QWORD *)(v8 + 56) + 8LL * v6);
    if ( *(__int16 *)(v12 + 4) != *(__int16 *)(v5 + 10) )
    {
      v13 = *(_QWORD *)(v5 + 72); /*0x18006ed77*/
      if ( (*(_BYTE *)(v12 + 9) & 1) != 0 ) /*0x18006ed80*/
      {
        *(_DWORD *)(v13 + 32) |= 0x20000u; /*0x18006f533*/
        goto LABEL_92; /*0x18006f53a*/
      }
      if ( *(_BYTE *)(v5 + 4) != 0 )
      {
        v14 = *(_DWORD *)(v8 + 32); /*0x18006ed93*/
        if ( (v14 & 0x20) != 0 ) /*0x18006ed99*/
        {
          v15 = *(_DWORD *)(v8 + 4); /*0x18006ed9d*/
          v16 = *(_DWORD *)(v8 + 8); /*0x18006eda1*/
          *(_DWORD *)(v5 + 16) = *(_DWORD *)v8; /*0x18006eda5*/
          *(_DWORD *)(v5 + 20) = v15; /*0x18006eda9*/
          *(_DWORD *)(v5 + 24) = v16; /*0x18006edad*/
        }
        if ( (v14 & 0x10) != 0 ) /*0x18006edb4*/
        {
          v17 = *(_DWORD *)(v8 + 20); /*0x18006edb9*/
          v18 = *(_DWORD *)(v8 + 24); /*0x18006edbd*/
          *(_DWORD *)(v5 + 32) = *(_DWORD *)(v8 + 16); /*0x18006edc1*/
          *(_DWORD *)(v5 + 36) = v17; /*0x18006edc5*/
          *(_DWORD *)(v5 + 40) = v18; /*0x18006edc9*/
        }
        if ( v6 >= (int)v9 ) /*0x18006edd0*/
          v19 = 0; /*0x18006eddf*/
        else
          v19 = *(_QWORD *)(*(_QWORD *)(v8 + 56) + 8LL * v6); /*0x18006edd9*/
        if ( (*(_BYTE *)(v19 + 6) & 1) != 0 && (v9 = v19 + 64, v19 != -64) )
        {
          sub_1800A7160(a1: v13, a2: v9); /*0x18006f021*/
        }
        else if ( (v14 & 8) != 0 )
        {
          sub_1800459D0(a1: v57, a2: v9); /*0x18006ee02*/
          v22 = *(_BYTE *)(v19 + 7) & 7; /*0x18006ee0b*/
          if ( (*(_BYTE *)(v19 + 7) & 7) != 0 ) /*0x18006ee0e*/
          {
            switch ( v22 ) /*0x18006ee13*/
            {
              case 1: /*0x18006ee13*/
                sub_18002DE20(a1: v57); /*0x18006ee6b*/
                break;
              case 2: /*0x18006ee13*/
                sub_18002DE90(a1: v57); /*0x18006ee24*/
                break;
              case 4: /*0x18006ee13*/
                sub_18002DF00(a1: v57); /*0x18006ee3a*/
                break;
              default:
                sub_18002DF70(a1: (unsigned int)v57, a2: (unsigned int)v57, a3: v20, a4: v21, a5: *(_DWORD *)(v19 + 56)); /*0x18006ee5a*/
                break;
            }
          }
          sub_18002E6E0( /*0x18006ee9f*/
            a1: (unsigned int)v57,
            a2: (unsigned int)v57,
            a3: v20,
            a4: v21,
            a5: *(float *)(v5 + 40) + *(float *)(v19 + 24));
          sub_18002E650(a1: (unsigned int)v57, a2: (unsigned int)v57, a3: v23, a4: v24, a5: 0.0 - *(float *)(v5 + 24)); /*0x18006eed0*/
          if ( (*(_BYTE *)(v19 + 7) & 0x78) != 0 )
          {
            v26 = (__m128)(unsigned int)dword_180184424; /*0x18006eedf*/
            v25 = *(float *)&dword_180184424 - *(float *)(v19 + 32); /*0x18006eef6*/
            v26.m128_f32[0] = *(float *)&dword_180184424 - *(float *)(v19 + 36); /*0x18006ef04*/
            v27 = v25 == 0.0 ? -_mm_cvtsi128_si32((__m128i)_mm_cmpeq_ss(v26, (__m128)0LL)) : 0;
            if ( *(float *)(v19 + 40) == 0.0 ) /*0x18006ef39*/
            {
              v28.m128_i32[0] = *(_DWORD *)(v19 + 44); /*0x18006ef2d*/
              v29 = -_mm_cvtsi128_si32((__m128i)_mm_cmpeq_ss((__m128)0LL, v28)); /*0x18006ef4d*/
            }
            else
            {
              v29 = 0; /*0x18006ef3b*/
            }
            if ( v27 == 0 || v29 == 0 ) /*0x18006ef55*/
            {
              v30 = v25 * *(float *)(v5 + 16); /*0x18006ef71*/
              v31 = v26.m128_f32[0] * *(float *)(v5 + 20); /*0x18006ef75*/
              v54 = 0; /*0x18006ef7d*/
              v55[1] = 0; /*0x18006ef81*/
              v55[2] = 0; /*0x18006ef85*/
              v53 = 1065353216; /*0x18006ef8c*/
              v55[0] = 1065353216; /*0x18006ef94*/
              *(float *)v56 = v30; /*0x18006ef9c*/
              *(float *)&v56[1] = v31; /*0x18006efa2*/
              XCd229cc00004d(a1: &v53, a2: &v53, a3: v56); /*0x18006efa8*/
              if ( v29 == 0 ) /*0x18006efaf*/
                sub_18002A860(a1: &v53, a2: &v53); /*0x18006efe5*/
              sub_18002A250(a1: &v53); /*0x18006eff7*/
              sub_18002CCF0(a1: v57, a2: v57, a3: &v53); /*0x18006f009*/
            }
          }
          sub_1800A7160(a1: *(_QWORD *)(v5 + 72), a2: v57); /*0x18006f017*/
        }
      }
      sub_180029560(a1: v59); /*0x18006f02e*/
      v33 = *(_BYTE *)(v12 + 6); /*0x18006f033*/
      if ( (v33 & 6) != 0 ) /*0x18006f03a*/
      {
        sub_180029580(a1: v59, a2: *(unsigned int *)(v12 + 132)); /*0x18006f528*/
LABEL_70:
        *(_DWORD *)(v13 + 36) |= 0x4000000u; /*0x18006f229*/
        sub_180029650(a1: v13 + 208, a2: v59, a3: v32, a4: v36); /*0x18006f23f*/
        *(_DWORD *)(v13 + 32) |= 0x40000u; /*0x18006f244*/
        goto LABEL_71; /*0x18006f244*/
      }
      if ( (v33 & 8) != 0 ) /*0x18006f042*/
      {
        if ( *(float *)(v12 + 60) >= 0.0 ) /*0x18006f4dd*/
          sub_180029580(a1: v59, a2: 0xFFFFFF); /*0x18006f508*/
        else
          sub_180029580(a1: v59, a2: 0); /*0x18006f4ed*/
        goto LABEL_70; /*0x18006f4f3*/
      }
      v34 = *(_BYTE *)(v12 + 8); /*0x18006f048*/
      if ( v34 != 0 ) /*0x18006f04f*/
      {
        v35 = *(_DWORD *)(v12 + 128); /*0x18006f055*/
        v36 = *(unsigned int *)(v12 + 132); /*0x18006f061*/
        if ( (*(_BYTE *)(v12 + 8) & 0xF0) != 0 ) /*0x18006f068*/
        {
          v37 = (unsigned __int8)v36; /*0x18006f07b*/
          v38 = BYTE1(v36); /*0x18006f07f*/
          v32 = BYTE2(v36); /*0x18006f083*/
          v36 = BYTE3(v36); /*0x18006f087*/
          v58.m128_u64[0] = __PAIR64__(v38, v37); /*0x18006f08b*/
          v58.m128_u64[1] = __PAIR64__(v36, v32); /*0x18006f09b*/
          v39 = _mm_mul_ps( /*0x18006f0db*/
                  _mm_cvtepi32_ps((__m128i)_mm_unpacklo_ps(
                                             _mm_unpacklo_ps((__m128)v37, (__m128)(unsigned int)v32),
                                             _mm_unpacklo_ps((__m128)v38, (__m128)(unsigned int)v36))),
                  (__m128)xmmword_1801843B0);
          if ( (v34 & 0x40) != 0 ) /*0x18006f0e2*/
          {
            v58 = v39; /*0x18006f0e4*/
            v59[4] = v39.m128_i32[2]; /*0x18006f0f3*/
          }
          if ( (v34 & 0x20) != 0 ) /*0x18006f0fc*/
          {
            v58 = v39; /*0x18006f0fe*/
            v59[5] = v39.m128_i32[1]; /*0x18006f10d*/
          }
          if ( (v34 & 0x10) != 0 ) /*0x18006f116*/
          {
            v58 = v39; /*0x18006f118*/
            v59[6] = v39.m128_i32[0]; /*0x18006f127*/
          }
          if ( v34 < 0 ) /*0x18006f133*/
          {
            v58 = v39; /*0x18006f135*/
            v59[7] = v39.m128_i32[3]; /*0x18006f144*/
          }
        }
        if ( (v34 & 0xF) != 0 ) /*0x18006f14d*/
        {
          v32 = BYTE2(v35); /*0x18006f166*/
          v58.m128_i32[0] = (unsigned __int8)v35; /*0x18006f16d*/
          v58.m128_i32[1] = BYTE1(v35); /*0x18006f176*/
          v58.m128_i32[2] = BYTE2(v35); /*0x18006f17d*/
          v58.m128_i32[3] = HIBYTE(v35); /*0x18006f185*/
          v40 = _mm_mul_ps( /*0x18006f1bc*/
                  _mm_cvtepi32_ps((__m128i)_mm_unpacklo_ps(
                                             _mm_unpacklo_ps((__m128)(unsigned __int8)v35, (__m128)BYTE2(v35)),
                                             _mm_unpacklo_ps((__m128)BYTE1(v35), (__m128)HIBYTE(v35)))),
                  (__m128)xmmword_1801843B0);
          if ( (v34 & 4) != 0 ) /*0x18006f1c3*/
          {
            v58 = v40; /*0x18006f1c5*/
            v59[0] = v40.m128_i32[2]; /*0x18006f1d4*/
          }
          if ( (v34 & 2) != 0 ) /*0x18006f1dd*/
          {
            v58 = v40; /*0x18006f1df*/
            v59[1] = v40.m128_i32[1]; /*0x18006f1ee*/
          }
          if ( (v34 & 1) != 0 ) /*0x18006f1f7*/
          {
            v58 = v40; /*0x18006f1f9*/
            v59[2] = v40.m128_i32[0]; /*0x18006f208*/
          }
          if ( (v34 & 8) != 0 ) /*0x18006f211*/
          {
            v58 = v40; /*0x18006f213*/
            v59[3] = v40.m128_i32[3]; /*0x18006f222*/
          }
        }
        goto LABEL_70; /*0x18006f222*/
      }
LABEL_71:
      if ( *(_BYTE *)(v12 + 10) == 0 ) /*0x18006f252*/
        goto LABEL_91; /*0x18006f252*/
      v41 = sub_1800E6180(); /*0x18006f25d*/
      v42 = *(unsigned __int8 *)(v12 + 10); /*0x18006f260*/
      v43 = 0; /*0x18006f265*/
      if ( *(_BYTE *)(v12 + 10) == 0 ) /*0x18006f26b*/
      {
LABEL_89:
        sub_180125C90(a1: v13, a2: v41); /*0x18006f412*/
        sub_180048E10(a1: v41); /*0x18006f420*/
        goto LABEL_91; /*0x18006f426*/
      }
      v58.m128_u64[0] = v5; /*0x18006f271*/
      v5 = 0; /*0x18006f279*/
      v60 = v2; /*0x18006f27c*/
      v44 = 0; /*0x18006f284*/
      v61 = v4; /*0x18006f287*/
      v62 = v3; /*0x18006f28f*/
      while ( 1 ) /*0x18006f29e*/
      {
        v45 = *(_BYTE **)(*(_QWORD *)(v12 + 136) + 8 * v44); /*0x18006f29e*/
        if ( v45 != nullptr ) /*0x18006f2a6*/
          break; /*0x18006f2a6*/
LABEL_87:
        v5 = (unsigned int)(v5 + 1); /*0x18006f3e3*/
        ++v44; /*0x18006f3e6*/
        if ( (int)v5 >= (int)v42 ) /*0x18006f3ec*/
        {
          v5 = v58.m128_u64[0]; /*0x18006f3f2*/
          goto LABEL_89; /*0x18006f3f2*/
        }
      }
      if ( *v45 == 0 ) /*0x18006f2b4*/
      {
        v46 = sub_1801222B0(a1: v42, a2: v43); /*0x18006f2bf*/
        sub_180122660(a1: v46, a2: v45); /*0x18006f2c8*/
        v47 = *(_QWORD *)(*(_QWORD *)(v41 + 32) /*0x18006f2fe*/
                        + ((int)(v5 + ((unsigned int)((int)v5 >> 2) >> 29)) & 0xFFFFFFFFFFFFFFF8uLL))
            + 16LL * ((int)v5 % 8);
        if ( ((0x12080u >> *(_WORD *)v47) & 1) != 0 ) /*0x18006f316*/
          sub_18005FF50(a1: v47); /*0x18006f4b7*/
        v48 = 13; /*0x18006f321*/
        if ( v46 == 0 ) /*0x18006f32a*/
          v48 = 12; /*0x18006f32a*/
        *(_WORD *)v47 = v48; /*0x18006f334*/
        *(_WORD *)(v47 + 2) = 74; /*0x18006f339*/
        *(_QWORD *)(v47 + 8) = v46; /*0x18006f33e*/
        v42 = *(unsigned __int8 *)(v12 + 10); /*0x18006f342*/
        goto LABEL_87; /*0x18006f347*/
      }
      if ( *v45 == 99 ) /*0x18006f350*/
      {
        v49 = sub_180121740(a1: v42, a2: v43); /*0x18006f35b*/
        sub_180121E70(a1: v49, a2: v45); /*0x18006f364*/
        v50 = *(_QWORD *)(*(_QWORD *)(v41 + 32) /*0x18006f39a*/
                        + ((int)(v5 + ((unsigned int)((int)v5 >> 2) >> 29)) & 0xFFFFFFFFFFFFFFF8uLL))
            + 16LL * ((int)v5 % 8);
        if ( ((0x12080u >> *(_WORD *)v50) & 1) != 0 ) /*0x18006f3b2*/
          sub_18005FF50(a1: v50); /*0x18006f4c5*/
        v51 = 13; /*0x18006f3bd*/
        if ( v49 == 0 ) /*0x18006f3c6*/
          v51 = 12; /*0x18006f3c6*/
        *(_WORD *)v50 = v51; /*0x18006f3d0*/
        *(_WORD *)(v50 + 2) = 40; /*0x18006f3d5*/
        *(_QWORD *)(v50 + 8) = v49; /*0x18006f3da*/
        v42 = *(unsigned __int8 *)(v12 + 10); /*0x18006f3de*/
        goto LABEL_87; /*0x18006f3de*/
      }
      sub_180042450(a1: "afp-motion", a2: "%s unknown filter type(%d)", a3: "afp_keyframe_base_get_filters_array"); /*0x18006f43d*/
LABEL_91:
      sub_1800AD820(a1: v13, a2: *(unsigned __int16 *)(v12 + 2)); /*0x18006f443*/
      *(_DWORD *)(v13 + 32) = ((*(_BYTE *)(v12 + 9) & 4) << 15) | *(_DWORD *)(v13 + 32) & 0xFFFDFFFF; /*0x18006f466*/
LABEL_92:
      *(_WORD *)(v5 + 10) = v6; /*0x18006f469*/
      return 0; /*0x18006ecd5*/
    }
  }
  return 0; /*0x18006ed1a*/
}
