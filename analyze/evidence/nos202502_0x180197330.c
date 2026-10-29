__int64 __fastcall sub_180197330(__int64 a1)
{
  __int64 v2; // r9
  CLibXact *Instance; // rax
  int v4; // edi
  __int64 v5; // rcx
  __int64 v6; // r8
  __int64 v7; // rdx
  __int64 v8; // rax
  __int64 v9; // rax
  __int64 v10; // rcx
  int i; // ecx
  __int64 v12; // r8
  __int64 v13; // rdx
  float v14; // xmm2_4
  bool v15; // bl
  __int64 v16; // rax
  __int64 v17; // rax
  __int64 result; // rax
  __int64 v19; // [rsp+40h] [rbp-28h]
  int v20; // [rsp+70h] [rbp+8h] BYREF
  int v21; // [rsp+74h] [rbp+Ch]

  sub_180307420(a1); /*0x180197341*/
  if ( *(_BYTE *)(a1 + 1618) == 0 && dword_18070A610 >= *(_DWORD *)(*(_QWORD *)(a1 + 1608) + 172LL) - 1 ) /*0x180197364*/
  {
    Instance = (CLibXact *)CLibXact::getInstance(); /*0x180197366*/
    CLibXact::SetSoundWithParam( /*0x18019739a*/
      this: Instance,
      a2: "_backtrack",
      a3: -1.0,
      a4: -20000.0,
      a5: -20000.0,
      a6: 0.0,
      a7: false);
    *(_BYTE *)(a1 + 1618) = 1; /*0x1801973a0*/
    HIDWORD(qword_18070A718) = timeGetTime(); /*0x1801973ad*/
  }
  v4 = *(_DWORD *)(a1 + 1668); /*0x1801973b3*/
  if ( v4 < *(_DWORD *)(a1 + 1672) ) /*0x1801973c4*/
  {
    while ( 1 ) /*0x180197400*/
    {
      v5 = *(_QWORD *)(a1 + 1608); /*0x180197400*/
      v6 = *(_QWORD *)(v5 + 64); /*0x18019740a*/
      v7 = (*(_QWORD *)(v5 + 72) - v6) / 12; /*0x180197422*/
      v8 = (int)v7; /*0x180197425*/
      if ( v4 < (int)v7 ) /*0x18019742a*/
        v8 = v4; /*0x18019742c*/
      v9 = 3 * v8; /*0x18019742f*/
      v10 = *(unsigned int *)(v6 + 4 * v9 + 8); /*0x180197439*/
      v19 = *(_QWORD *)(v6 + 4 * v9); /*0x18019743e*/
      if ( HIDWORD(v19) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908) > 0 ) /*0x180197461*/
        break; /*0x180197461*/
      v20 = *(_DWORD *)(v6 + 4 * v9 + 8); /*0x18019746f*/
      v21 = (int)(float)(*(float *)&dword_18070A724 * 100.0); /*0x180197477*/
      sub_180360060(a1: v10, a2: v7, a3: v6, a4: v2); /*0x18019747b*/
      if ( *((_DWORD *)&unk_18070B870 + 4 * (int)v19) == (_DWORD)v19 ) /*0x18019748e*/
        ((void (__fastcall *)(int *))funcs_180197165[2 * (int)v19])(a1: &v20); /*0x180197495*/
      *(_DWORD *)(a1 + 1668) = ++v4; /*0x18019749b*/
      if ( v4 >= *(_DWORD *)(a1 + 1672) ) /*0x1801974a7*/
        goto LABEL_13; /*0x1801974a7*/
    }
    *(_DWORD *)(a1 + 1668) = v4; /*0x1801974af*/
  }
LABEL_13:
  if ( *(_BYTE *)(a1 + 1632) == 0 ) /*0x1801974d1*/
  {
    for ( i = *(_DWORD *)(a1 + 1620); ; *(_DWORD *)(a1 + 1620) = i ) /*0x1801974d7*/
    {
      v12 = *(_QWORD *)(a1 + 1608); /*0x1801974f0*/
      if ( i >= *(_DWORD *)(v12 + 152) ) /*0x1801974fe*/
        break; /*0x1801974fe*/
      v13 = *(_QWORD *)(v12 + 232) + 104LL * i; /*0x18019750b*/
      if ( v13 == 0 ) /*0x180197512*/
        break; /*0x180197512*/
      v14 = (float)(180 * (*(_DWORD *)(v13 + 20) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908))) /*0x18019754a*/
          / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300);
      if ( fmaxf(*(float *)&dword_1813668D4 / *(float *)&qword_1813668C0, *(float *)&dword_1813668D4) <= v14 ) /*0x180197560*/
        goto LABEL_22; /*0x180197560*/
      if ( v14 > 0.5 ) /*0x180197565*/
      {
        sub_1802FBEF0(a1: *(_QWORD *)(a1 + 1592)); /*0x18019756e*/
        i = *(_DWORD *)(a1 + 1620); /*0x180197573*/
      }
      ++i; /*0x180197579*/
    }
    *(_BYTE *)(a1 + 1632) = 1; /*0x180197586*/
  }
LABEL_22:
  sub_1802FBE20(a1: *(_QWORD *)(a1 + 1592)); /*0x18019758d*/
  v15 = sub_1802F7C10(a1: *(_QWORD *)(a1 + 1592)); /*0x1801975a5*/
  v16 = sub_180300D50(); /*0x1801975a8*/
  sub_180300E70(a1: v16); /*0x1801975b0*/
  v17 = sub_180304A70(); /*0x1801975b5*/
  result = sub_180176790(a1: v17); /*0x1801975bd*/
  if ( v15 || (_BYTE)word_1813E4A8C != 0 ) /*0x1801975d7*/
    return sub_1802FC9D0(a1: (__int64)&dword_18070A610); /*0x1801975ef*/
  LODWORD(qword_18070A740) = qword_18070A740 - 1; /*0x1801975d9*/
  return result; /*0x1801975df*/
}
