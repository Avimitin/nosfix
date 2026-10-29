__int64 __fastcall sub_1802FF090(__int64 a1)
{
  __int16 v2; // si
  int v3; // ebx
  __int64 v4; // rcx
  __int64 v5; // r8
  __int64 v6; // rdx
  __int64 v7; // rax
  __int64 v8; // rax
  int i; // ecx
  __int64 v10; // r8
  __int64 v11; // rdx
  float v12; // xmm1_4
  float v13; // xmm0_4
  char v14; // bl
  __int64 v15; // rax
  __int64 v16; // rax
  __int64 result; // rax
  __int64 v18; // r9
  int v19; // r8d
  int v20; // edx
  __int64 *v21; // rcx
  __int64 v22; // [rsp+20h] [rbp-38h]
  int v23; // [rsp+60h] [rbp+8h] BYREF
  int v24; // [rsp+64h] [rbp+Ch]

  v2 = *(_WORD *)(((__int64 (*)(void))sub_180307420)() + 24); /*0x1802ff0ac*/
  sub_1802FEDC0(a1); /*0x1802ff0b0*/
  v3 = *(_DWORD *)(a1 + 1668); /*0x1802ff0b5*/
  if ( v3 < *(_DWORD *)(a1 + 1672) ) /*0x1802ff0c1*/
  {
    while ( 1 ) /*0x1802ff0f0*/
    {
      v4 = *(_QWORD *)(a1 + 1608); /*0x1802ff0f0*/
      v5 = *(_QWORD *)(v4 + 64); /*0x1802ff0fa*/
      v6 = (*(_QWORD *)(v4 + 72) - v5) / 12; /*0x1802ff112*/
      v7 = (int)v6; /*0x1802ff115*/
      if ( v3 < (int)v6 ) /*0x1802ff11a*/
        v7 = v3; /*0x1802ff11c*/
      v8 = 3 * v7; /*0x1802ff11f*/
      v22 = *(_QWORD *)(v5 + 4 * v8); /*0x1802ff12e*/
      if ( HIDWORD(v22) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908) > 0 ) /*0x1802ff151*/
        break; /*0x1802ff151*/
      v23 = *(_DWORD *)(v5 + 4 * v8 + 8); /*0x1802ff15f*/
      v24 = (int)(float)(*(float *)&dword_18070A724 * 100.0); /*0x1802ff167*/
      sub_180360060(); /*0x1802ff16b*/
      if ( *((_DWORD *)&unk_18070B870 + 4 * (int)v22) == (_DWORD)v22 ) /*0x1802ff17f*/
        ((void (__fastcall *)(int *))funcs_180197165[2 * (int)v22])(a1: &v23); /*0x1802ff186*/
      *(_DWORD *)(a1 + 1668) = ++v3; /*0x1802ff18c*/
      if ( v3 >= *(_DWORD *)(a1 + 1672) ) /*0x1802ff198*/
        goto LABEL_10; /*0x1802ff198*/
    }
    *(_DWORD *)(a1 + 1668) = v3; /*0x1802ff1a0*/
  }
LABEL_10:
  if ( *(_BYTE *)(a1 + 1632) == 0 ) /*0x1802ff1b7*/
  {
    for ( i = *(_DWORD *)(a1 + 1620); ; *(_DWORD *)(a1 + 1620) = i ) /*0x1802ff1bd*/
    {
      v10 = *(_QWORD *)(a1 + 1608); /*0x1802ff1d0*/
      if ( i >= *(_DWORD *)(v10 + 152) ) /*0x1802ff1de*/
        break; /*0x1802ff1de*/
      v11 = *(_QWORD *)(v10 + 232) + 104LL * i; /*0x1802ff1eb*/
      if ( v11 == 0 ) /*0x1802ff1f2*/
        break; /*0x1802ff1f2*/
      v12 = (float)(180 * (*(_DWORD *)(v11 + 20) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908))) /*0x1802ff22a*/
          / (float)(*(float *)&dword_18070B300 * *(float *)&dword_18070B300);
      v13 = *(float *)&dword_1813668D4 / *(float *)&qword_1813668C0; /*0x1802ff231*/
      if ( (float)(*(float *)&dword_1813668D4 / *(float *)&qword_1813668C0) <= *(float *)&dword_1813668D4 ) /*0x1802ff23c*/
        v13 = *(float *)&dword_1813668D4; /*0x1802ff23e*/
      if ( v13 <= v12 ) /*0x1802ff244*/
        goto LABEL_21; /*0x1802ff244*/
      if ( v12 > 0.5 ) /*0x1802ff249*/
      {
        sub_1802FBEF0(a1: *(_QWORD *)(a1 + 1592)); /*0x1802ff252*/
        i = *(_DWORD *)(a1 + 1620); /*0x1802ff257*/
      }
      ++i; /*0x1802ff25d*/
    }
    *(_BYTE *)(a1 + 1632) = 1; /*0x1802ff26a*/
  }
LABEL_21:
  sub_1802FBE20(a1: *(_QWORD *)(a1 + 1592)); /*0x1802ff271*/
  v14 = sub_1802F7C10(a1: *(_QWORD *)(a1 + 1592)); /*0x1802ff289*/
  v15 = sub_180300D50(); /*0x1802ff28c*/
  sub_180300E70(a1: v15); /*0x1802ff294*/
  v16 = sub_180304A70(); /*0x1802ff299*/
  result = sub_180176790(a1: v16); /*0x1802ff2a1*/
  if ( v14 != 0 || (_BYTE)word_1813E4A8C != 0 ) /*0x1802ff2bb*/
  {
    if ( dword_1813E4A90 > 1 /*0x1802ff2ee*/
      && HIDWORD(qword_181366908) + dword_18070A610 + HIDWORD(qword_181366914) >= 0
      && (int)qword_1813E4A94 <= 1 )
    {
      LOBYTE(v18) = v2 < *(_WORD *)(sub_180307420(a1: (unsigned int)(HIDWORD(qword_181366908) /*0x1802ff30d*/
                                                                   + dword_18070A610
                                                                   + HIDWORD(qword_181366914)))
                                  + 24);
      sub_1801CF380( /*0x1802ff31d*/
        a1: qword_1811CEEB8,
        a2: (unsigned int)(HIDWORD(qword_181366908) + HIDWORD(qword_181366914) + dword_18070A610),
        a3: (unsigned int)qword_181366908,
        a4: v18);
    }
    sub_1802FC9D0(a1: &dword_18070A610); /*0x1802ff329*/
    result = sub_18012B220( /*0x1802ff34a*/
               a1: *(_QWORD *)(a1 + 1688),
               a2: (unsigned int)(HIDWORD(qword_181366908) + HIDWORD(qword_181366914) + dword_18070A610),
               a3: 0);
    if ( (int)qword_1813E4A94 > 1 ) /*0x1802ff356*/
    {
      v19 = dword_1813E4A90; /*0x1802ff358*/
      v20 = 0; /*0x1802ff35f*/
      if ( dword_1813E4A90 > 0 ) /*0x1802ff364*/
      {
        v21 = &qword_1813E4AAC; /*0x1802ff366*/
        do /*0x1802ff38a*/
        {
          result = *(unsigned int *)v21; /*0x1802ff370*/
          if ( (int)result > 0 ) /*0x1802ff374*/
          {
            result = (unsigned int)(result - 1); /*0x1802ff376*/
            *(_DWORD *)v21 = result; /*0x1802ff378*/
            v19 = dword_1813E4A90; /*0x1802ff37a*/
          }
          ++v20; /*0x1802ff381*/
          v21 = (__int64 *)((char *)v21 + 4); /*0x1802ff383*/
        }
        while ( v20 < v19 ); /*0x1802ff38a*/
      }
    }
  }
  else
  {
    LODWORD(qword_18070A740) = qword_18070A740 - 1; /*0x1802ff2bd*/
  }
  return result; /*0x1802ff2c3*/
}
