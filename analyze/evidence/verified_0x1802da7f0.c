__int64 __fastcall sub_1802DA7F0(__int64 a1, int a2, float *a3)
{
  float v3; // xmm3_4
  float v5; // xmm2_4
  float v6; // xmm1_4
  __int64 result; // rax
  int v9; // edi
  __int64 v10; // rdx
  __int64 v11; // rdx
  __int64 v12; // rax
  int v13; // ecx
  _QWORD *v14; // rax
  int v15; // eax
  _QWORD *v16; // rax
  int v17; // [rsp+30h] [rbp-28h] BYREF
  __int64 v18; // [rsp+34h] [rbp-24h]
  __int16 v19; // [rsp+3Ch] [rbp-1Ch]
  char v20; // [rsp+3Eh] [rbp-1Ah]
  char v21; // [rsp+3Fh] [rbp-19h]
  __int16 v22; // [rsp+68h] [rbp+10h]
  _QWORD *v23; // [rsp+70h] [rbp+18h] BYREF

  v3 = *a3; /*0x1802da7ff*/
  v5 = a3[1]; /*0x1802da807*/
  v6 = a3[2]; /*0x1802da810*/
  LOBYTE(v22) = (int)(float)(*a3 * 127.0); /*0x1802da835*/
  result = (unsigned int)(int)(float)(v5 * 127.0); /*0x1802da839*/
  HIBYTE(v22) = (int)(float)(v5 * 127.0); /*0x1802da844*/
  v9 = (int)(float)(v6 * 127.0); /*0x1802da848*/
  if ( a2 != 0 ) /*0x1802da84e*/
  {
    v10 = (unsigned int)(a2 - 1); /*0x1802da854*/
    if ( (_DWORD)v10 != 0 ) /*0x1802da857*/
    {
      v11 = (unsigned int)(v10 - 1); /*0x1802da85d*/
      if ( (_DWORD)v11 != 0 ) /*0x1802da860*/
      {
        if ( (_DWORD)v11 == 1 ) /*0x1802da865*/
        {
          v12 = sub_180353AF0(a1, a2: v11); /*0x1802da86b*/
          v13 = *(_DWORD *)(a1 + 224) - 1; /*0x1802da878*/
          v18 = 0; /*0x1802da87a*/
          v17 = v13; /*0x1802da87f*/
          v19 = v22; /*0x1802da88d*/
          v21 = 0; /*0x1802da899*/
          v20 = v9; /*0x1802da89e*/
          return sub_180351450(a1: v12 + 5536, a2: &v17); /*0x1802da8a3*/
        }
      }
      else
      {
        if ( qword_1812D1E70 == 0 ) /*0x1802da8b5*/
        {
          v14 = operator new(Size: 0x40u); /*0x1802da8bc*/
          v23 = v14; /*0x1802da8c1*/
          qword_1812D1E70 = (__int64)v14; /*0x1802da8cf*/
          *v14 = &CDeviceDriver::`vftable'; /*0x1802da8d6*/
          v14[1] = 0; /*0x1802da8d9*/
          v14[2] = 0; /*0x1802da8dd*/
          v14[3] = 0; /*0x1802da8e1*/
          v14[4] = 0; /*0x1802da8e5*/
          v14[5] = 0; /*0x1802da8e9*/
          v14[6] = 0; /*0x1802da8ed*/
          *((_DWORD *)v14 + 14) = 0; /*0x1802da8f1*/
          *((_WORD *)v14 + 30) = 0; /*0x1802da8f4*/
          v6 = a3[2]; /*0x1802da8f8*/
          v5 = a3[1]; /*0x1802da8fd*/
          v3 = *a3; /*0x1802da902*/
        }
        result = qword_1812D1E68; /*0x1802da928*/
        *(_DWORD *)(qword_1812D1E68 + 2552) = (unsigned __int8)(int)(float)(v3 * 127.0); /*0x1802da92f*/
        *(_DWORD *)(result + 2556) = (unsigned __int8)(int)(float)(v5 * 127.0); /*0x1802da935*/
        *(_DWORD *)(result + 2560) = (unsigned __int8)(int)(float)(v6 * 127.0); /*0x1802da93b*/
      }
    }
    else
    {
      LOWORD(v23) = v22; /*0x1802da94c*/
      BYTE2(v23) = (int)(float)(v6 * 127.0); /*0x1802da951*/
      v15 = sub_180353AF0(a1, a2: v10); /*0x1802da956*/
      return sub_1803516E0(a1: v15 + 5536, a2: 0, a3: (unsigned int)&v23, a4: 0, a5: 1, a6: 0); /*0x1802da975*/
    }
  }
  else
  {
    if ( qword_1812D1E70 == 0 ) /*0x1802da987*/
    {
      v16 = operator new(Size: 0x40u); /*0x1802da98e*/
      v23 = v16; /*0x1802da993*/
      qword_1812D1E70 = (__int64)v16; /*0x1802da9a1*/
      *v16 = &CDeviceDriver::`vftable'; /*0x1802da9a8*/
      v16[1] = 0; /*0x1802da9ab*/
      v16[2] = 0; /*0x1802da9af*/
      v16[3] = 0; /*0x1802da9b3*/
      v16[4] = 0; /*0x1802da9b7*/
      v16[5] = 0; /*0x1802da9bb*/
      v16[6] = 0; /*0x1802da9bf*/
      *((_DWORD *)v16 + 14) = 0; /*0x1802da9c3*/
      *((_WORD *)v16 + 30) = 0; /*0x1802da9c6*/
      v6 = a3[2]; /*0x1802da9ca*/
      v5 = a3[1]; /*0x1802da9cf*/
      v3 = *a3; /*0x1802da9d4*/
    }
    result = qword_1812D1E68; /*0x1802da9fa*/
    *(_DWORD *)(qword_1812D1E68 + 2520) = (unsigned __int8)(int)(float)(v3 * 127.0); /*0x1802daa01*/
    *(_DWORD *)(result + 2524) = (unsigned __int8)(int)(float)(v5 * 127.0); /*0x1802daa07*/
    *(_DWORD *)(result + 2528) = (unsigned __int8)(int)(float)(v6 * 127.0); /*0x1802daa0d*/
  }
  return result; /*0x1802daa14*/
}
