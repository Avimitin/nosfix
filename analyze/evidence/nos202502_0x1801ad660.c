unsigned __int64 __fastcall sub_1801AD660(__int64 a1)
{
  __int64 i; // rcx
  __int64 v3; // r9
  CLibXact *Instance; // rax
  int v5; // edi
  __int64 v6; // rcx
  __int64 v7; // r8
  __int64 v8; // rdx
  __int64 v9; // rax
  __int64 v10; // rax
  __int64 v11; // r8
  __int64 v12; // rdx
  float v13; // xmm2_4
  __int64 v14; // rax
  int v15; // r9d
  __int64 v16; // rdx
  int v17; // r8d
  bool v18; // cc
  __int64 v19; // rcx
  int v20; // edx
  int v21; // edx
  bool v22; // zf
  bool v23; // bl
  __int64 v24; // rax
  unsigned __int64 result; // rax
  __int64 v26; // rbx
  __int64 v27; // [rsp+40h] [rbp-28h]
  int v28; // [rsp+70h] [rbp+8h] BYREF
  int v29; // [rsp+74h] [rbp+Ch]

  sub_180307420(a1); /*0x1801ad671*/
  if ( *(_BYTE *)(a1 + 1618) == 0 && dword_18070A610 >= *(_DWORD *)(*(_QWORD *)(a1 + 1608) + 172LL) - 1 ) /*0x1801ad694*/
  {
    Instance = (CLibXact *)CLibXact::getInstance(); /*0x1801ad696*/
    CLibXact::SetSoundWithParam( /*0x1801ad6ca*/
      this: Instance,
      a2: "_backtrack",
      a3: -1.0,
      a4: -20000.0,
      a5: -20000.0,
      a6: 0.0,
      a7: false);
    *(_BYTE *)(a1 + 1618) = 1; /*0x1801ad6d0*/
    HIDWORD(qword_18070A718) = timeGetTime(); /*0x1801ad6dd*/
  }
  v5 = *(_DWORD *)(a1 + 1668); /*0x1801ad6e3*/
  if ( v5 < *(_DWORD *)(a1 + 1672) ) /*0x1801ad6f4*/
  {
    while ( 1 ) /*0x1801ad720*/
    {
      v6 = *(_QWORD *)(a1 + 1608); /*0x1801ad720*/
      v7 = *(_QWORD *)(v6 + 64); /*0x1801ad72a*/
      v8 = (*(_QWORD *)(v6 + 72) - v7) / 12; /*0x1801ad742*/
      v9 = (int)v8; /*0x1801ad745*/
      if ( v5 < (int)v8 ) /*0x1801ad74a*/
        v9 = v5; /*0x1801ad74c*/
      v10 = 3 * v9; /*0x1801ad74f*/
      i = *(unsigned int *)(v7 + 4 * v10 + 8); /*0x1801ad759*/
      v27 = *(_QWORD *)(v7 + 4 * v10); /*0x1801ad75e*/
      if ( HIDWORD(v27) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908) > 0 ) /*0x1801ad781*/
        break; /*0x1801ad781*/
      v28 = *(_DWORD *)(v7 + 4 * v10 + 8); /*0x1801ad78f*/
      v29 = (int)(float)(*(float *)&dword_18070A724 * 100.0); /*0x1801ad797*/
      sub_180360060(a1: i, a2: v8, a3: v7, a4: v3); /*0x1801ad79b*/
      if ( *((_DWORD *)&unk_18070B870 + 4 * (int)v27) == (_DWORD)v27 ) /*0x1801ad7ae*/
        ((void (__fastcall *)(int *))funcs_180197165[2 * (int)v27])(a1: &v28); /*0x1801ad7b5*/
      *(_DWORD *)(a1 + 1668) = ++v5; /*0x1801ad7bb*/
      if ( v5 >= *(_DWORD *)(a1 + 1672) ) /*0x1801ad7c7*/
        goto LABEL_13; /*0x1801ad7c7*/
    }
    *(_DWORD *)(a1 + 1668) = v5; /*0x1801ad7cf*/
  }
LABEL_13:
  if ( *(_BYTE *)(a1 + 1632) == 0 ) /*0x1801ad7f1*/
  {
    for ( i = *(unsigned int *)(a1 + 1620); ; *(_DWORD *)(a1 + 1620) = i ) /*0x1801ad7f7*/
    {
      v11 = *(_QWORD *)(a1 + 1608); /*0x1801ad810*/
      if ( (int)i >= *(_DWORD *)(v11 + 152) ) /*0x1801ad81e*/
        break; /*0x1801ad81e*/
      v12 = *(_QWORD *)(v11 + 232) + 104LL * (int)i; /*0x1801ad82b*/
      if ( v12 == 0 ) /*0x1801ad832*/
        break; /*0x1801ad832*/
      v13 = (float)(180 * (*(_DWORD *)(v12 + 20) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908))) /*0x1801ad86a*/
          / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300);
      if ( fmaxf(*(float *)&dword_1813668D4 / *(float *)&qword_1813668C0, *(float *)&dword_1813668D4) <= v13 ) /*0x1801ad880*/
        goto LABEL_22; /*0x1801ad880*/
      if ( v13 > 0.5 ) /*0x1801ad885*/
      {
        sub_1802FBEF0(a1: *(_QWORD *)(a1 + 1592)); /*0x1801ad88e*/
        LODWORD(i) = *(_DWORD *)(a1 + 1620); /*0x1801ad893*/
      }
      i = (unsigned int)(i + 1); /*0x1801ad899*/
    }
    *(_BYTE *)(a1 + 1632) = 1; /*0x1801ad8a6*/
  }
LABEL_22:
  v14 = sub_18018B090(a1: i); /*0x1801ad8ad*/
  v15 = HIDWORD(qword_1813E471C); /*0x1801ad8b2*/
  byte_1813E56DC = *(_BYTE *)(v14 + 104); /*0x1801ad8c2*/
  if ( *(_BYTE *)(a1 + 1676) != 0 ) /*0x1801ad8cf*/
  {
    v16 = *(_QWORD *)(a1 + 1608); /*0x1801ad8d1*/
    v17 = 0; /*0x1801ad8d8*/
    if ( *(int *)(v16 + 152) >= 0 ) /*0x1801ad8e2*/
    {
      v18 = *(_DWORD *)(v16 + 152) > 0; /*0x1801ad8e4*/
      do /*0x1801ad8eb*/
      {
        if ( !v18 ) /*0x1801ad8eb*/
          goto LABEL_35; /*0x1801ad8eb*/
        v19 = *(_QWORD *)(v16 + 232) + 104LL * v17; /*0x1801ad8f4*/
        if ( v19 == 0 ) /*0x1801ad8fb*/
          goto LABEL_35; /*0x1801ad8fb*/
        v20 = *(_DWORD *)(v19 + 48); /*0x1801ad8fd*/
        *(_BYTE *)(v19 + 97) = 0; /*0x1801ad900*/
        if ( v20 != 0 ) /*0x1801ad906*/
        {
          v21 = v20 - 1; /*0x1801ad908*/
          if ( v21 != 0 ) /*0x1801ad90b*/
          {
            if ( v21 != 1 ) /*0x1801ad910*/
              goto LABEL_35; /*0x1801ad910*/
LABEL_34:
            *(_BYTE *)(v19 + 97) = 1; /*0x1801ad920*/
            goto LABEL_35; /*0x1801ad920*/
          }
          v22 = v15 == 2; /*0x1801ad914*/
        }
        else
        {
          v22 = v15 == 1; /*0x1801ad91a*/
        }
        if ( v22 ) /*0x1801ad91e*/
          goto LABEL_34; /*0x1801ad91e*/
LABEL_35:
        v16 = *(_QWORD *)(a1 + 1608); /*0x1801ad924*/
        v18 = ++v17 < *(_DWORD *)(v16 + 152); /*0x1801ad92e*/
      }
      while ( v17 <= *(_DWORD *)(v16 + 152) ); /*0x1801ad8eb*/
    }
    *(_BYTE *)(a1 + 1676) = 0; /*0x1801ad937*/
  }
  sub_1802FBE20(a1: *(_QWORD *)(a1 + 1592)); /*0x1801ad945*/
  v23 = sub_1802F7C10(a1: *(_QWORD *)(a1 + 1592)); /*0x1801ad956*/
  v24 = sub_180300D50(); /*0x1801ad959*/
  sub_180300E70(a1: v24); /*0x1801ad961*/
  if ( v23 || (_BYTE)word_1813E4A8C != 0 ) /*0x1801ad970*/
    sub_1802FC9D0(a1: (__int64)&dword_18070A610); /*0x1801ad981*/
  else
    LODWORD(qword_18070A740) = qword_18070A740 - 1; /*0x1801ad972*/
  result = avs2_core_44(a1: 0); /*0x1801ad988*/
  v26 = result; /*0x1801ad995*/
  if ( result > qword_1813EC318 + 1200 ) /*0x1801ad9a2*/
  {
    result = sub_1801CDDE0(a1: qword_1813EC318 + 1200, a2: 0); /*0x1801ad9a6*/
    qword_1813EC318 = v26; /*0x1801ad9ab*/
  }
  return result; /*0x1801ad9b2*/
}
