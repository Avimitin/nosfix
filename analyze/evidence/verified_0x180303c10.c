__int64 __fastcall sub_180303C10(int a1, int a2)
{
  int v2; // r9d
  int v4; // r8d
  int *v5; // rax
  int v6; // r8d
  __int64 result; // rax
  __int64 v8; // rcx

  v2 = dword_1813E4A60; /*0x180303c15*/
  v4 = a2; /*0x180303c26*/
  if ( a2 <= dword_1813E4A60 + a2 ) /*0x180303c2f*/
  {
    v5 = &dword_1813668B0[2 * a2 + 129075]; /*0x180303c43*/
    do /*0x180303c6c*/
    {
      if ( (__int64)v5 > (__int64)&dword_1813E4A5C ) /*0x180303c49*/
        break; /*0x180303c49*/
      ++v4; /*0x180303c51*/
      *(v5 - 1) = dword_1813E4968; /*0x180303c54*/
      *v5 = a1; /*0x180303c57*/
      v5 += 2; /*0x180303c5a*/
      v2 = dword_1813E4A60; /*0x180303c5e*/
    }
    while ( v4 <= dword_1813E4A60 + a2 ); /*0x180303c6c*/
  }
  v6 = a2; /*0x180303c70*/
  result = (unsigned int)(a2 - v2); /*0x180303c73*/
  if ( a2 >= (int)result ) /*0x180303c78*/
  {
    v8 = a2; /*0x180303c7a*/
    do /*0x180303cab*/
    {
      if ( v8 < 0 ) /*0x180303c83*/
        break; /*0x180303c83*/
      --v6; /*0x180303c8b*/
      dword_1813668B0[2 * v8 + 129074] = dword_1813E4968; /*0x180303c8e*/
      dword_1813668B0[2 * v8-- + 129075] = a1; /*0x180303c97*/
      result = (unsigned int)(a2 - dword_1813E4A60); /*0x180303ca2*/
    }
    while ( v6 >= (int)result ); /*0x180303cab*/
  }
  return result; /*0x180303cad*/
}
