__int64 __fastcall sub_180368F10(__int64 a1)
{
  __int64 result; // rax
  int v3; // edx
  float *v4; // rcx
  __m128 v5; // xmm7
  float v6; // xmm6_4
  float v7; // xmm1_4
  float v8; // xmm2_4
  int v9; // eax
  char *v10; // rdi
  __int64 i; // rsi
  __m128 v12; // xmm1
  unsigned int v13; // edx
  int v14; // edx
  int v15; // r8d
  int v16; // eax
  _DWORD *v17; // r9
  float v18; // xmm0_4
  int v19; // r8d
  int v20; // edx
  int v21; // eax
  float v22; // xmm1_4
  float v23; // xmm0_4
  int v24; // edx
  float v25; // xmm0_4
  float v26[4]; // [rsp+30h] [rbp-C8h] BYREF
  _BYTE v27[16]; // [rsp+40h] [rbp-B8h] BYREF
  __int64 v28; // [rsp+50h] [rbp-A8h]
  __int128 v29; // [rsp+60h] [rbp-98h]
  char v30; // [rsp+70h] [rbp-88h] BYREF

  v28 = -2; /*0x180368f1b*/
  result = sub_180311A00(); /*0x180368f49*/
  if ( *(_BYTE *)(result + 36) != 0 ) /*0x180368f52*/
  {
    v3 = *(_DWORD *)(a1 + 8); /*0x180368f58*/
    if ( v3 != 0 ) /*0x180368f5d*/
    {
      if ( *(_BYTE *)(a1 + 61) == 0 ) /*0x180368f67*/
      {
        result = *(int *)(a1 + 12); /*0x180368f6d*/
        switch ( *(_DWORD *)(a1 + 12) ) /*0x180368f8c*/
        {
          case 1: /*0x180368f8c*/
            __wind /*0xf1c0000000000040*/
            {
              if ( (dword_181872470 & 1) == 0 ) /*0x180368f96*/
              {
                dword_181872470 |= 1u; /*0x180368f9b*/
                sub_18013D660(); /*0x180368fa1*/
                atexit(a1: sub_18038EEC0); /*0x180368fad*/
              }
              v4 = v26; /*0x180368fb3*/
            }
            __unwind
            {
              dword_181872470 &= ~1u; /*0x18038df2a*/
            }
            goto LABEL_27; /*0xf1c000000000001c*/
          case 2: /*0x180368f8c*/
            v5 = (__m128)xmmword_1813E6748; /*0x180368fbd*/
            v6 = (float)((float)v3 / 60.0) - (float)(int)(float)((float)v3 / 60.0); /*0x180368fde*/
            unknown_libname_4(a1: v26); /*0x180368fe7*/
            v26[0] = (float)((float)(v5.m128_f32[0] - v5.m128_f32[0]) * v6) + v5.m128_f32[0]; /*0x180368ffb*/
            v7 = _mm_shuffle_ps(v5, v5, 85).m128_f32[0]; /*0x180369004*/
            v26[1] = (float)((float)(v7 - v7) * v6) + v7; /*0x180369017*/
            v8 = _mm_shuffle_ps(v5, v5, 170).m128_f32[0]; /*0x180369020*/
            v26[2] = (float)((float)(v8 - v8) * v6) + v8; /*0x180369033*/
            v5.m128_f32[0] = _mm_shuffle_ps(v5, v5, 255).m128_f32[0]; /*0x180369039*/
            v26[3] = (float)((float)(v5.m128_f32[0] - v5.m128_f32[0]) * v6) + v5.m128_f32[0]; /*0x18036904c*/
            v9 = 90; /*0x180369057*/
            if ( *(_DWORD *)(a1 + 16) + 1 < 90 ) /*0x18036905e*/
              v9 = *(_DWORD *)(a1 + 16) + 1; /*0x18036905e*/
            *(_DWORD *)(a1 + 16) = v9; /*0x180369061*/
            goto LABEL_26; /*0x180369064*/
          case 3: /*0x180368f8c*/
            v29 = xmmword_1813E6748; /*0x180369070*/
            v10 = &v30; /*0x180369075*/
            for ( i = 5; i != 0; --i ) /*0x18036907a*/
            {
              unknown_libname_4(a1: v10); /*0x180369083*/
              v10 += 16; /*0x180369088*/
            }
            v12 = (__m128)*(&v29 + *(int *)(a1 + 32)); /*0x180369099*/
            sub_1800D23B0( /*0x1803690df*/
              a1: (__int64)v27,
              a2: v12.m128_f32[0] * (float)((float)(*(float *)(a1 + 28) * 0.40000001) + 0.0),
              a3: _mm_shuffle_ps(v12, v12, 85).m128_f32[0] * (float)((float)(*(float *)(a1 + 28) * 0.40000001) + 0.0),
              a4: _mm_shuffle_ps(v12, v12, 170).m128_f32[0] * (float)((float)(*(float *)(a1 + 28) * 0.40000001) + 0.0),
              a5: 1065353216);
            goto LABEL_28; /*0x1803690e4*/
          case 4: /*0x180368f8c*/
            if ( v3 <= 180 ) /*0x1803690ef*/
              goto LABEL_28; /*0x1803690ef*/
            result = sub_180311A00(); /*0x1803690f5*/
            if ( *(_BYTE *)(result + 36) == 0 || *(_DWORD *)(a1 + 12) == 1 ) /*0x180369108*/
              goto LABEL_31; /*0x180369108*/
            *(_DWORD *)(a1 + 8) = 0; /*0x180369110*/
            *(_DWORD *)(a1 + 12) = 1; /*0x180369113*/
            ++*(_DWORD *)(a1 + 8); /*0x18036911a*/
            return result; /*0x18036911d*/
          case 5: /*0x180368f8c*/
            if ( v3 > 10 ) /*0x180369125*/
            {
              v3 = 0; /*0x180369127*/
              *(_DWORD *)(a1 + 8) = 0; /*0x180369129*/
            }
            v13 = (int)((unsigned __int64)(1717986919LL * v3) >> 32) >> 1; /*0x180369133*/
            result = v13 >> 31; /*0x180369137*/
            v14 = result + v13; /*0x18036913a*/
            if ( v14 != 0 ) /*0x18036913c*/
            {
              if ( v14 != 1 ) /*0x180369141*/
                goto LABEL_31; /*0x180369141*/
              v15 = (unsigned __int8)(int)(float)(*(float *)&dword_1813E6760 * 127.0); /*0x18036915f*/
              v16 = (int)(float)(*(float *)&dword_1813E675C * 127.0); /*0x18036916f*/
              v17 = (_DWORD *)qword_1812D1E68; /*0x180369173*/
              v18 = *(float *)&dword_1813E6758; /*0x18036917a*/
LABEL_29:
              v17[634] = (unsigned __int8)(int)(float)(v18 * 127.0); /*0x1803692d3*/
              v17[635] = (unsigned __int8)v16; /*0x1803692e8*/
              v17[636] = v15; /*0x1803692ef*/
              v21 = (int)(float)(*((float *)&xmmword_1813E6748 + 2) * 127.0); /*0x180369302*/
              v22 = *((float *)&xmmword_1813E6748 + 1); /*0x180369306*/
              v23 = *(float *)&xmmword_1813E6748; /*0x18036930e*/
            }
            else
            {
              v19 = (unsigned __int8)(int)(float)(*((float *)&xmmword_1813E6748 + 2) * 127.0); /*0x18036919f*/
              v20 = (unsigned __int8)(int)(float)(*((float *)&xmmword_1813E6748 + 1) * 127.0); /*0x1803691b3*/
              v17 = (_DWORD *)qword_1812D1E68; /*0x1803691b6*/
              *(_DWORD *)(qword_1812D1E68 + 2536) = (unsigned __int8)(int)(float)(*(float *)&xmmword_1813E6748 * 127.0); /*0x1803691d0*/
              v17[635] = v20; /*0x1803691d7*/
              v17[636] = v19; /*0x1803691de*/
              v21 = (int)(float)(*(float *)&dword_1813E6760 * 127.0); /*0x1803691f1*/
              v22 = *(float *)&dword_1813E675C; /*0x1803691f5*/
              v23 = *(float *)&dword_1813E6758; /*0x1803691fd*/
            }
            v17[632] = (unsigned __int8)v21; /*0x180369329*/
            result = (unsigned int)(int)(float)(v23 * 127.0); /*0x180369330*/
            v17[631] = (unsigned __int8)(int)(float)(v22 * 127.0); /*0x180369334*/
            v17[630] = (unsigned __int8)result; /*0x18036933e*/
LABEL_31:
            ++*(_DWORD *)(a1 + 8); /*0x180369345*/
            break; /*0x180369348*/
          case 6: /*0x180368f8c*/
            result = sub_180368620(a1); /*0x18036920d*/
            ++*(_DWORD *)(a1 + 8); /*0x180369212*/
            return result; /*0x180369215*/
          case 7: /*0x180368f8c*/
            if ( (dword_181872470 & 1) == 0 ) /*0x180369222*/
            {
              __wind /*0xf1c0000000000034*/
              {
                dword_181872470 |= 1u; /*0x180369227*/
                sub_18013D660(); /*0x18036922d*/
                atexit(a1: sub_18038EEC0); /*0x180369239*/
              }
              __unwind
              {
                dword_181872470 &= ~1u; /*0x18038df40*/
              }
            }
LABEL_26:
            v4 = (float *)v27; /*0x18036923f*/
LABEL_27:
            unknown_libname_4(a1: v4); /*0x180369244*/
LABEL_28:
            v17 = (_DWORD *)qword_1812D1E68; /*0x180369249*/
            v24 = (unsigned __int8)(int)(float)(*(float *)&dword_1813E675C * 127.0); /*0x180369284*/
            v25 = *(float *)&dword_1813E6758 * 127.0; /*0x180369287*/
            *(_DWORD *)(qword_1812D1E68 + 2560) = (unsigned __int8)(int)(float)(*(float *)&dword_1813E6760 * 127.0); /*0x18036928b*/
            v17[639] = v24; /*0x180369296*/
            v17[638] = (unsigned __int8)(int)v25; /*0x1803692a0*/
            v15 = (unsigned __int8)(int)(float)(*((float *)&xmmword_1813E6748 + 2) * 127.0); /*0x1803692bf*/
            v16 = (int)(float)(*((float *)&xmmword_1813E6748 + 1) * 127.0); /*0x1803692c7*/
            v18 = *(float *)&xmmword_1813E6748; /*0x1803692cb*/
            goto LABEL_29; /*0x1803692cb*/
          default:
            goto LABEL_31;
        }
      }
    }
    else if ( *(_BYTE *)(a1 + 61) == 0 ) /*0x18036934e*/
    {
      result = 1; /*0x180369350*/
      *(_DWORD *)(a1 + 8) = 1; /*0x180369353*/
    }
  }
  return result; /*0x180369356*/
}
