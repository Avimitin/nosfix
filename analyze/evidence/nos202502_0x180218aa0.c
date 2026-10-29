__int64 __fastcall sub_180218AA0(__int64 a1)
{
  __int64 v1; // r9
  __int64 result; // rax
  int v4; // ecx
  double v5; // xmm0_8
  double v6; // xmm2_8
  double v7; // xmm1_8
  int v8; // edx
  int v9; // ecx
  double v10; // xmm1_8
  float v11; // xmm2_4
  __int128 v12; // [rsp+0h] [rbp-18h]

  v1 = *(int *)(a1 + 224); /*0x180218aa4*/
  result = *(int *)(a1 + 4 * v1 + 228); /*0x180218aae*/
  switch ( *(_DWORD *)(a1 + 4 * v1 + 228) ) /*0x180218ad1*/
  {
    case 0: /*0x180218ad1*/
      if ( *(_DWORD *)(a1 + 4 * v1 + 272) == 300 ) /*0x180218adf*/
      {
        *(_DWORD *)(a1 + 4 * v1 + 244) = 1; /*0x180218ae5*/
        result = *(int *)(a1 + 224); /*0x180218af1*/
        *(_BYTE *)(result + a1 + 260) = 1; /*0x180218af8*/
      }
      break; /*0x180218b04*/
    case 1: /*0x180218ad1*/
    case 3: /*0x180218ad1*/
      *(_DWORD *)(a1 + 4 * v1 + 244) = 2; /*0x180218b05*/
      result = *(int *)(a1 + 224); /*0x180218b11*/
      *(_BYTE *)(result + a1 + 260) = 1; /*0x180218b18*/
      break; /*0x180218b24*/
    case 2: /*0x180218ad1*/
      v4 = *(_DWORD *)(a1 + 4 * v1 + 272) + 1; /*0x180218b3a*/
      v5 = *(double *)&qword_1813E7FE8 + *(double *)(a1 + 120); /*0x180218b3c*/
      *(double *)(a1 + 120) = v5; /*0x180218b53*/
      v6 = (double)v4 / v5; /*0x180218b5c*/
      result = (unsigned int)(180 * (v4 / 180)); /*0x180218b62*/
      *(double *)(a1 + 128) = v6; /*0x180218b68*/
      if ( v4 == (_DWORD)result ) /*0x180218b73*/
      {
        v7 = v6 - *(double *)(a1 + 136); /*0x180218b83*/
        v8 = *(_DWORD *)(a1 + 144) + 1; /*0x180218b94*/
        *(_DWORD *)(a1 + 144) = v8; /*0x180218b98*/
        if ( fabs(v7) >= 0.002 ) /*0x180218baa*/
        {
          *(_DWORD *)(a1 + 148) = 0; /*0x180218bbc*/
          v9 = 0; /*0x180218bc3*/
        }
        else
        {
          v9 = ++*(_DWORD *)(a1 + 148); /*0x180218bb3*/
        }
        *(double *)(a1 + 136) = v6; /*0x180218bc5*/
        if ( v9 < 3 ) /*0x180218bd1*/
        {
          if ( v8 < 30 ) /*0x180218beb*/
          {
            *(_QWORD *)(a1 + 120) = 0; /*0x180218c02*/
            *(_DWORD *)(a1 + 4LL * *(int *)(a1 + 224) + 244) = 3; /*0x180218c0d*/
          }
          else
          {
            *(_DWORD *)(a1 + 4LL * *(int *)(a1 + 224) + 244) = 5; /*0x180218bf4*/
          }
        }
        else
        {
          *(_DWORD *)(a1 + 4LL * *(int *)(a1 + 224) + 244) = 4; /*0x180218bda*/
        }
        result = *(int *)(a1 + 224); /*0x180218c19*/
        *(_BYTE *)(result + a1 + 260) = 1; /*0x180218c20*/
      }
      break; /*0x180218c2d*/
    case 4: /*0x180218ad1*/
      v10 = *(double *)(a1 + 128); /*0x180218c2e*/
      v11 = v10; /*0x180218c36*/
      if ( v11 >= 59.0 && v11 <= 60.0 ) /*0x180218c4e*/
      {
        qword_18070B2F8 = *(_QWORD *)(a1 + 128); /*0x180218c54*/
        dword_18070B304 = (int)v10; /*0x180218c5c*/
        *(float *)&dword_18070B300 = v10; /*0x180218c62*/
        LODWORD(v1) = *(_DWORD *)(a1 + 224); /*0x180218c6a*/
      }
      *(_DWORD *)(a1 + 4LL * (int)v1 + 244) = 5; /*0x180218c74*/
      result = *(int *)(a1 + 224); /*0x180218c7f*/
      *(_BYTE *)(result + a1 + 260) = 1; /*0x180218c86*/
      break; /*0x180218c92*/
    case 5: /*0x180218ad1*/
      *(_QWORD *)&v12 = sub_180218A90; /*0x180218c9a*/
      *(_DWORD *)(a1 + 224) = 0; /*0x180218ca0*/
      DWORD2(v12) = 0; /*0x180218ca6*/
      *(_OWORD *)(a1 + 208) = v12; /*0x180218cae*/
      *(_DWORD *)(a1 + 244) = 0; /*0x180218cb5*/
      result = *(int *)(a1 + 224); /*0x180218cbb*/
      *(_BYTE *)(result + a1 + 260) = 1; /*0x180218cc2*/
      break; /*0x180218cc2*/
    default:
      return result;
  }
  return result; /*0x180218b00*/
}
