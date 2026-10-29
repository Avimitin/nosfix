void __fastcall sub_1803078B0(__int64 a1, __int64 a2, __int64 a3, __int64 a4)
{
  char *v4; // rsi
  __int64 v5; // r8
  int v6; // eax
  __int64 v7; // r8
  int v8; // ebx
  __int64 v9; // rdi
  float v10; // xmm7_4
  CLibXact *Instance; // rax
  __int128 *v12; // rax
  __int64 v13; // rdi
  __int64 v14; // rbx
  CLibXact *v15; // rax

  v4 = (char *)&xmmword_1813E4AD8 + 12; /*0x1803078cc*/
  v5 = (unsigned int)a1; /*0x180307909*/
  dword_1813E56D8 = a1; /*0x180307913*/
  do /*0x180307a9c*/
  {
    v6 = *((_DWORD *)v4 - 1); /*0x180307920*/
    if ( v6 != -1 && v6 <= (int)v5 ) /*0x18030792f*/
    {
      v7 = (unsigned __int8)*(v4 - 7); /*0x180307935*/
      if ( (unsigned __int8)(v7 - 1) <= 0x57u ) /*0x180307940*/
      {
        a2 = 0; /*0x180307942*/
        a1 = 0; /*0x180307945*/
        while ( *((_BYTE *)&xmmword_1813E4AD8 + a1 + 5) != (_BYTE)v7 /*0x180307970*/
             || *(_DWORD *)((char *)&xmmword_1813E4AD8 + a1 + 8) != -1
             || *(__int64 *)((char *)qword_1813E4AE8 + a1) != *(_QWORD *)(v4 + 4) )
        {
          ++a2; /*0x180307972*/
          a1 += 24; /*0x180307975*/
          if ( a1 >= 3072 ) /*0x180307980*/
            goto LABEL_14; /*0x180307980*/
        }
        if ( a2 != -1 ) /*0x180307988*/
        {
          a2 = *((unsigned int *)&xmmword_1813E4AD8 + 6 * a2); /*0x18030798e*/
          if ( (int)a2 >= 0 ) /*0x180307997*/
            sub_1802BCAD0(a1: &xmmword_1813E4AD8); /*0x18030799c*/
        }
      }
LABEL_14:
      v8 = (unsigned __int8)*(v4 - 6); /*0x1803079a1*/
      v9 = *(_QWORD *)(v4 + 4); /*0x1803079a5*/
      sub_180360060(a1, a2, a3: v7, a4); /*0x1803079a9*/
      v10 = *(float *)&dword_180654030[byte_1813E56DC]; /*0x1803079bd*/
      Instance = (CLibXact *)CLibXact::getInstance(); /*0x1803079c6*/
      CLibXact::SetSoundWithParam(this: Instance, a2: v9, a3: (float)v8, a4: v10, a5: -20000.0, a6: 0.0, a7: false); /*0x1803079eb*/
      *((_DWORD *)v4 - 1) = -1; /*0x1803079f1*/
      v5 = (unsigned int)dword_1813E56D8; /*0x1803079f8*/
    }
    if ( *(_DWORD *)v4 != -1 && *(_DWORD *)v4 <= (int)v5 ) /*0x180307a0d*/
    {
      a2 = *((unsigned int *)v4 - 3); /*0x180307a13*/
      if ( (int)a2 >= 0 ) /*0x180307a18*/
      {
        a1 = 0; /*0x180307a1a*/
        v12 = &xmmword_1813E4AD8; /*0x180307a1d*/
        while ( *(_DWORD *)v12 != (_DWORD)a2 ) /*0x180307a22*/
        {
          ++a1; /*0x180307a24*/
          v12 = (__int128 *)((char *)v12 + 24); /*0x180307a27*/
          if ( (__int64)v12 >= (__int64)&dword_1813E56D8 ) /*0x180307a2e*/
            goto LABEL_24; /*0x180307a2e*/
        }
        if ( a1 != -1 ) /*0x180307a36*/
        {
          v13 = 3 * a1; /*0x180307a38*/
          v14 = qword_1813E4AE8[3 * a1]; /*0x180307a3c*/
          sub_180360060(a1, a2, a3: v5, a4); /*0x180307a44*/
          v15 = (CLibXact *)CLibXact::getInstance(); /*0x180307a49*/
          CLibXact::FadeOutSound(this: v15, a2: v14, a3: false); /*0x180307a58*/
          *((_DWORD *)&xmmword_1813E4AD8 + 2 * v13) = -1; /*0x180307a5e*/
          *((_WORD *)&xmmword_1813E4AD8 + 4 * v13 + 2) = 0; /*0x180307a69*/
          *((_BYTE *)&xmmword_1813E4AD8 + 8 * v13 + 6) = 0; /*0x180307a72*/
          *((_QWORD *)&xmmword_1813E4AD8 + v13 + 1) = -1; /*0x180307a7a*/
          qword_1813E4AE8[v13] = 0; /*0x180307a86*/
          v5 = (unsigned int)dword_1813E56D8; /*0x180307a8e*/
        }
      }
    }
LABEL_24:
    v4 += 24; /*0x180307a95*/
  }
  while ( (__int64)v4 < (__int64)&qword_1813E56E4 ); /*0x180307a9c*/
}
