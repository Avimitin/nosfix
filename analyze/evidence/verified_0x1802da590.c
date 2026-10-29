__int64 __fastcall sub_1802DA590(__int64 a1, float *a2)
{
  int v2; // edi
  float *v3; // rbx
  float v5; // xmm3_4
  float v6; // xmm2_4
  float v7; // xmm1_4
  __int64 result; // rax
  int v9; // esi
  __int64 v10; // rax
  int v11; // ecx
  int v12; // eax
  __int16 v13; // [rsp+30h] [rbp-58h] BYREF
  char v14; // [rsp+32h] [rbp-56h]
  int v15; // [rsp+40h] [rbp-48h] BYREF
  __int64 v16; // [rsp+44h] [rbp-44h]
  __int16 v17; // [rsp+4Ch] [rbp-3Ch]
  char v18; // [rsp+4Eh] [rbp-3Ah]
  char v19; // [rsp+4Fh] [rbp-39h]
  __int16 v20; // [rsp+A0h] [rbp+18h]
  _QWORD *v21; // [rsp+A8h] [rbp+20h]
  _QWORD *v22; // [rsp+A8h] [rbp+20h]

  v2 = 0; /*0x1802da5b7*/
  v3 = a2; /*0x1802da5ba*/
  do /*0x1802da7c7*/
  {
    v5 = *v3; /*0x1802da5c0*/
    v6 = v3[1]; /*0x1802da5c6*/
    v7 = v3[2]; /*0x1802da5ce*/
    LOBYTE(v20) = (int)(float)(*v3 * 127.0); /*0x1802da5e2*/
    result = (unsigned int)(int)(float)(v6 * 127.0); /*0x1802da5e9*/
    HIBYTE(v20) = (int)(float)(v6 * 127.0); /*0x1802da5f4*/
    v9 = (int)(float)(v7 * 127.0); /*0x1802da5fb*/
    if ( v2 != 0 ) /*0x1802da601*/
    {
      switch ( v2 ) /*0x1802da60a*/
      {
        case 1: /*0x1802da60a*/
          v13 = v20; /*0x1802da6fe*/
          v14 = (int)(float)(v7 * 127.0); /*0x1802da703*/
          v12 = sub_180353AF0(a1: 0, a2); /*0x1802da708*/
          result = sub_1803516E0(a1: v12 + 5536, a2: 0, a3: (unsigned int)&v13, a4: 0, a5: 1, a6: 0); /*0x1802da728*/
          break;
        case 2: /*0x1802da60a*/
          if ( qword_1812D1E70 == 0 ) /*0x1802da668*/
          {
            v21 = operator new(Size: 0x40u); /*0x1802da674*/
            qword_1812D1E70 = (__int64)v21; /*0x1802da67c*/
            *v21 = &CDeviceDriver::`vftable'; /*0x1802da683*/
            v21[1] = 0; /*0x1802da686*/
            v21[2] = 0; /*0x1802da68a*/
            v21[3] = 0; /*0x1802da68e*/
            v21[4] = 0; /*0x1802da692*/
            v21[5] = 0; /*0x1802da696*/
            v21[6] = 0; /*0x1802da69a*/
            *((_DWORD *)v21 + 14) = 0; /*0x1802da69e*/
            *((_WORD *)v21 + 30) = 0; /*0x1802da6a2*/
            v7 = v3[2]; /*0x1802da6a7*/
            v6 = v3[1]; /*0x1802da6ac*/
            v5 = *v3; /*0x1802da6b1*/
          }
          a2 = (float *)(unsigned __int8)(int)(float)(v6 * 127.0); /*0x1802da6cd*/
          result = qword_1812D1E68; /*0x1802da6d7*/
          *(_DWORD *)(qword_1812D1E68 + 2552) = (unsigned __int8)(int)(float)(v5 * 127.0); /*0x1802da6de*/
          *(_DWORD *)(result + 2556) = (_DWORD)a2; /*0x1802da6e4*/
          *(_DWORD *)(result + 2560) = (unsigned __int8)(int)(float)(v7 * 127.0); /*0x1802da6ea*/
          break;
        case 3: /*0x1802da60a*/
          v10 = sub_180353AF0(a1: (unsigned int)(v2 - 2), a2); /*0x1802da61e*/
          v11 = *(_DWORD *)(a1 + 224) - 1; /*0x1802da62e*/
          v19 = 0; /*0x1802da630*/
          v15 = v11; /*0x1802da635*/
          v17 = v20; /*0x1802da641*/
          v16 = 0; /*0x1802da64d*/
          v18 = v9; /*0x1802da652*/
          result = sub_180351450(a1: v10 + 5536, a2: &v15); /*0x1802da657*/
          break;
        default:
          break; /*0x1802da60a*/
      }
    }
    else
    {
      if ( qword_1812D1E70 == 0 ) /*0x1802da739*/
      {
        v22 = operator new(Size: 0x40u); /*0x1802da745*/
        qword_1812D1E70 = (__int64)v22; /*0x1802da74d*/
        *v22 = &CDeviceDriver::`vftable'; /*0x1802da754*/
        v22[1] = 0; /*0x1802da757*/
        v22[2] = 0; /*0x1802da75b*/
        v22[3] = 0; /*0x1802da75f*/
        v22[4] = 0; /*0x1802da763*/
        v22[5] = 0; /*0x1802da767*/
        v22[6] = 0; /*0x1802da76b*/
        *((_DWORD *)v22 + 14) = 0; /*0x1802da76f*/
        *((_WORD *)v22 + 30) = 0; /*0x1802da773*/
        v7 = v3[2]; /*0x1802da778*/
        v6 = v3[1]; /*0x1802da77d*/
        v5 = *v3; /*0x1802da782*/
      }
      a2 = (float *)(unsigned __int8)(int)(float)(v6 * 127.0); /*0x1802da79e*/
      result = qword_1812D1E68; /*0x1802da7a8*/
      *(_DWORD *)(qword_1812D1E68 + 2520) = (unsigned __int8)(int)(float)(v5 * 127.0); /*0x1802da7af*/
      *(_DWORD *)(result + 2524) = (_DWORD)a2; /*0x1802da7b5*/
      *(_DWORD *)(result + 2528) = (unsigned __int8)(int)(float)(v7 * 127.0); /*0x1802da7bb*/
    }
    ++v2; /*0x1802da7c2*/
  }
  while ( v2 < 4 ); /*0x1802da7c7*/
  return result; /*0x1802da7cd*/
}
