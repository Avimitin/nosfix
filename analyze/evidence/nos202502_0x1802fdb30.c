void __fastcall sub_1802FDB30(__int64 a1)
{
  __int64 v2; // rdx
  __int64 v3; // rcx
  __int64 v4; // r8
  __int64 v5; // r9
  CLibXact *Instance; // rax
  char *v7; // rax
  __int64 v8; // rcx
  __int64 v9; // rax
  __int64 v10; // rcx
  unsigned int v11; // eax
  float v12; // xmm0_4
  __int64 v13; // rcx
  __int64 v14; // rax
  char *v15; // rsi
  __int64 v16; // r8
  int v17; // eax
  __int64 v18; // r8
  int v19; // ebx
  __int64 v20; // rdi
  float v21; // xmm7_4
  CLibXact *v22; // rax
  __int128 *v23; // rax
  __int64 v24; // rdi
  __int64 v25; // rbx
  CLibXact *v26; // rax
  const char *v27; // rbx
  CLibXact *v28; // rax
  char v29; // al
  int v30; // eax
  int v31; // ecx
  int v32; // ecx
  int v33; // eax
  int v34; // edx
  int v35; // eax
  int v36; // ecx
  int v37; // eax
  int v38; // eax
  int v39; // ecx
  int v40; // edx
  int v41; // ecx
  _DWORD *v42; // rax
  bool v43; // bl
  char v44; // al
  bool v45; // di
  __int64 v46; // rcx
  __int64 v47; // rcx
  __int64 v48; // rbx
  const char *v49; // rax
  __int64 v50; // rax
  __int64 v51; // rdx
  __int64 v52; // rcx
  __int64 v53; // r8
  __int64 v54; // r9
  CLibXact *v55; // rax
  __int64 (__fastcall *v56)(); // rax
  __int64 v57; // rcx
  int v58; // [rsp+40h] [rbp-88h] BYREF
  const char *v59; // [rsp+48h] [rbp-80h]
  int v60; // [rsp+50h] [rbp-78h]
  int v61; // [rsp+54h] [rbp-74h]
  char v62; // [rsp+58h] [rbp-70h]
  int v63; // [rsp+5Ch] [rbp-6Ch]

  if ( *(_BYTE *)(sub_180369760() + 54) != 0 ) /*0x1802fdb6f*/
    ++*(_DWORD *)(a1 + 1676); /*0x1802fdb71*/
  if ( *(_BYTE *)(sub_180369760() + 55) != 0 ) /*0x1802fdb80*/
    ++*(_DWORD *)(a1 + 1680); /*0x1802fdb82*/
  if ( *(_BYTE *)(a1 + 1653) == 0 ) /*0x1802fdb9c*/
  {
    v2 = (unsigned int)(HIDWORD(qword_181366908) + HIDWORD(qword_181366914) + dword_18070A610); /*0x1802fdbaa*/
    if ( (int)v2 >= -160 ) /*0x1802fdbb6*/
    {
      sub_180360060(a1: v3, a2: v2, a3: v4, a4: v5); /*0x1802fdbb8*/
      Instance = (CLibXact *)CLibXact::getInstance(); /*0x1802fdbc4*/
      CLibXact::SetSoundWithParam( /*0x1802fdbef*/
        this: Instance,
        a2: "se020_stg_start",
        a3: -1.0,
        a4: -20000.0,
        a5: -20000.0,
        a6: 0.0,
        a7: false);
      *(_BYTE *)(a1 + 1653) = 1; /*0x1802fdbf5*/
    }
  }
  v7 = (char *)qword_1812CF918; /*0x1802fdbfc*/
  if ( qword_1812CF918 != nullptr ) /*0x1802fdc09*/
    *(_QWORD *)((char *)qword_1812CF918 + 4) = 0; /*0x1802fdc0b*/
  if ( dword_1813E475C != 0 ) /*0x1802fdc16*/
  {
    if ( v7 != nullptr ) /*0x1802fdc1f*/
    {
      *(_DWORD *)v7 = 6; /*0x1802fdc21*/
      *(_OWORD *)(v7 + 12) = xmmword_181366178; /*0x1802fdc2e*/
    }
    sub_18018B090(a1: v3); /*0x1802fdc32*/
    v9 = sub_18018B090(a1: v8); /*0x1802fdc37*/
    v10 = 0; /*0x1802fdc3c*/
    v11 = *(_DWORD *)(v9 + 48) + 5; /*0x1802fdc42*/
    if ( v11 <= 0x5E ) /*0x1802fdc48*/
      v10 = v11; /*0x1802fdc48*/
    v12 = sub_1802C11C0(a1: v10); /*0x1802fdc4b*/
    sub_18033CF50(a1: 10, a2: 15, a3: byte_1805DDA28, v12); /*0x1802fdc6b*/
    v14 = sub_18018B090(a1: v13); /*0x1802fdc70*/
    sub_18033CF50(a1: 10, a2: 30, a3: byte_1805DDA60, *(unsigned int *)(v14 + 60)); /*0x1802fdc88*/
    sub_18033CF50(a1: 10, a2: 45, a3: byte_1805DDAE8, HIDWORD(qword_181366908)); /*0x1802fdca3*/
    sub_18033CF50(a1: 10, a2: 60, a3: byte_1805DDB20, HIDWORD(qword_181366914)); /*0x1802fdcbe*/
  }
  v15 = (char *)&xmmword_1813E4AD8 + 12; /*0x1802fdcca*/
  v16 = (unsigned int)(HIDWORD(qword_181366908) + HIDWORD(qword_181366914) + dword_18070A610); /*0x1802fdcdf*/
  dword_1813E56D8 = HIDWORD(qword_181366908) + HIDWORD(qword_181366914) + dword_18070A610; /*0x1802fdcfc*/
  do /*0x1802fde90*/
  {
    v17 = *((_DWORD *)v15 - 1); /*0x1802fdd12*/
    if ( v17 != -1 && v17 <= (int)v16 ) /*0x1802fdd21*/
    {
      v18 = (unsigned __int8)*(v15 - 7); /*0x1802fdd27*/
      if ( (unsigned __int8)(v18 - 1) <= 0x57u ) /*0x1802fdd32*/
      {
        v2 = 0; /*0x1802fdd34*/
        v3 = 0; /*0x1802fdd37*/
        while ( *((_BYTE *)&xmmword_1813E4AD8 + v3 + 5) != (_BYTE)v18 /*0x1802fdd61*/
             || *(_DWORD *)((char *)&xmmword_1813E4AD8 + v3 + 8) != -1
             || *(__int64 *)((char *)qword_1813E4AE8 + v3) != *(_QWORD *)(v15 + 4) )
        {
          ++v2; /*0x1802fdd63*/
          v3 += 24; /*0x1802fdd66*/
          if ( v3 >= 3072 ) /*0x1802fdd71*/
            goto LABEL_29; /*0x1802fdd71*/
        }
        if ( v2 != -1 ) /*0x1802fdd79*/
        {
          v2 = *((unsigned int *)&xmmword_1813E4AD8 + 6 * v2); /*0x1802fdd7f*/
          if ( (int)v2 >= 0 ) /*0x1802fdd89*/
            sub_1802BCAD0(a1: &xmmword_1813E4AD8); /*0x1802fdd8e*/
        }
      }
LABEL_29:
      v19 = (unsigned __int8)*(v15 - 6); /*0x1802fdd93*/
      v20 = *(_QWORD *)(v15 + 4); /*0x1802fdd97*/
      sub_180360060(a1: v3, a2: v2, a3: v18, a4: v5); /*0x1802fdd9b*/
      v21 = *(float *)&dword_180654030[byte_1813E56DC]; /*0x1802fddaf*/
      v22 = (CLibXact *)CLibXact::getInstance(); /*0x1802fddb9*/
      CLibXact::SetSoundWithParam(this: v22, a2: v20, a3: (float)v19, a4: v21, a5: -20000.0, a6: 0.0, a7: false); /*0x1802fddde*/
      *((_DWORD *)v15 - 1) = -1; /*0x1802fdde4*/
      v16 = (unsigned int)dword_1813E56D8; /*0x1802fddeb*/
    }
    if ( *(_DWORD *)v15 != -1 && *(_DWORD *)v15 <= (int)v16 ) /*0x1802fde00*/
    {
      v2 = *((unsigned int *)v15 - 3); /*0x1802fde06*/
      if ( (int)v2 >= 0 ) /*0x1802fde0b*/
      {
        v3 = 0; /*0x1802fde0d*/
        v23 = &xmmword_1813E4AD8; /*0x1802fde10*/
        while ( *(_DWORD *)v23 != (_DWORD)v2 ) /*0x1802fde15*/
        {
          ++v3; /*0x1802fde17*/
          v23 = (__int128 *)((char *)v23 + 24); /*0x1802fde1a*/
          if ( (__int64)v23 >= (__int64)&dword_1813E56D8 ) /*0x1802fde21*/
            goto LABEL_39; /*0x1802fde21*/
        }
        if ( v3 != -1 ) /*0x1802fde29*/
        {
          v24 = 3 * v3; /*0x1802fde2b*/
          v25 = qword_1813E4AE8[3 * v3]; /*0x1802fde2f*/
          sub_180360060(a1: v3, a2: v2, a3: v16, a4: v5); /*0x1802fde37*/
          v26 = (CLibXact *)CLibXact::getInstance(); /*0x1802fde3c*/
          CLibXact::FadeOutSound(this: v26, a2: v25, a3: false); /*0x1802fde4b*/
          *((_DWORD *)&xmmword_1813E4AD8 + 2 * v24) = -1; /*0x1802fde51*/
          *((_WORD *)&xmmword_1813E4AD8 + 4 * v24 + 2) = 0; /*0x1802fde5d*/
          *((_BYTE *)&xmmword_1813E4AD8 + 8 * v24 + 6) = 0; /*0x1802fde66*/
          *((_QWORD *)&xmmword_1813E4AD8 + v24 + 1) = -1; /*0x1802fde6e*/
          qword_1813E4AE8[v24] = 0; /*0x1802fde7a*/
          v16 = (unsigned int)dword_1813E56D8; /*0x1802fde82*/
        }
      }
    }
LABEL_39:
    v15 += 24; /*0x1802fde89*/
  }
  while ( (__int64)v15 < (__int64)&qword_1813E56E4 ); /*0x1802fde90*/
  v27 = (const char *)(sub_180360060(a1: v3, a2: v2, a3: v16, a4: v5) + 680); /*0x1802fde9b*/
  v28 = (CLibXact *)CLibXact::getInstance(); /*0x1802fdea2*/
  if ( (int)CLibXact::GetSoundLabelLapse(this: v28, a2: v27) <= 0 ) /*0x1802fdef2*/
  {
    v29 = 0; /*0x1802fdef4*/
LABEL_48:
    *(_DWORD *)(a1 + 1644) = 0; /*0x1802fdf2d*/
    goto LABEL_49; /*0x1802fdf2d*/
  }
  v30 = qword_18070A718; /*0x1802fdef8*/
  v31 = *(_DWORD *)(a1 + 1644); /*0x1802fdefe*/
  *(_DWORD *)(a1 + 1656) = qword_18070A718; /*0x1802fdf04*/
  if ( v30 >= -5 ) /*0x1802fdf0d*/
  {
    v33 = v31 - 1; /*0x1802fdf13*/
    v32 = 0; /*0x1802fdf16*/
    if ( v33 > 0 ) /*0x1802fdf1b*/
      v32 = v33; /*0x1802fdf1b*/
  }
  else
  {
    v32 = v31 + 1; /*0x1802fdf0f*/
  }
  v29 = 0; /*0x1802fdf1e*/
  *(_DWORD *)(a1 + 1644) = v32; /*0x1802fdf20*/
  if ( v32 >= 10 ) /*0x1802fdf29*/
  {
    v29 = 1; /*0x1802fdf2b*/
    goto LABEL_48; /*0x1802fdf2b*/
  }
LABEL_49:
  *(_BYTE *)(a1 + 1652) = v29; /*0x1802fdf34*/
  v34 = qword_18070A718; /*0x1802fdf3a*/
  *(_DWORD *)(a1 + 1656) = qword_18070A718; /*0x1802fdf40*/
  if ( v34 > *(_DWORD *)(a1 + 1664) ) /*0x1802fdf4c*/
    *(_DWORD *)(a1 + 1664) = v34; /*0x1802fdf4e*/
  v35 = *(_DWORD *)(a1 + 1660); /*0x1802fdf54*/
  if ( v34 < 10 ) /*0x1802fdf5d*/
  {
    v37 = v35 - 1; /*0x1802fdf64*/
    v36 = 0; /*0x1802fdf66*/
    if ( v37 > 0 ) /*0x1802fdf6b*/
      v36 = v37; /*0x1802fdf6b*/
  }
  else
  {
    v36 = v35 + 1; /*0x1802fdf5f*/
  }
  *(_DWORD *)(a1 + 1660) = v36; /*0x1802fdf6e*/
  if ( v34 > 25 && v36 >= 5 ) /*0x1802fdf7c*/
  {
    v38 = v36 - 1; /*0x1802fdf7e*/
    v39 = 0; /*0x1802fdf81*/
    if ( v38 > 0 ) /*0x1802fdf86*/
      v39 = v38; /*0x1802fdf86*/
    ++*(_DWORD *)(a1 + 1636); /*0x1802fdf89*/
    *(_DWORD *)(a1 + 1660) = v39; /*0x1802fdf8f*/
    sub_1802FF090(a1); /*0x1802fdf98*/
    v40 = qword_1813DED30; /*0x1802fdf9d*/
    v41 = 0; /*0x1802fdfa3*/
    if ( (int)qword_1813DED30 > 0 ) /*0x1802fdfa8*/
    {
      v42 = &unk_1813DEA24; /*0x1802fdfaa*/
      do /*0x1802fdfca*/
      {
        if ( *v42 == 1 ) /*0x1802fdfb4*/
        {
          *v42 = 2; /*0x1802fdfb6*/
          v40 = qword_1813DED30; /*0x1802fdfbc*/
        }
        ++v41; /*0x1802fdfc2*/
        v42 += 7; /*0x1802fdfc4*/
      }
      while ( v41 < v40 ); /*0x1802fdfca*/
    }
  }
  if ( *(_BYTE *)(a1 + 1652) != 0 ) /*0x1802fdfd3*/
    ++*(_DWORD *)(a1 + 1640); /*0x1802fdfdf*/
  else
    sub_1802FF090(a1); /*0x1802fdfd8*/
  v43 = *(int *)(*(_QWORD *)(a1 + 1592) + 8LL) <= 0 && *(int *)(sub_180300D50() + 8) <= 0; /*0x1802fe001*/
  v44 = sub_180307B30(); /*0x1802fe003*/
  v45 = dword_18070A610 >= dword_18070A614; /*0x1802fe014*/
  if ( *(_BYTE *)(a1 + 1633) == 0 && v43 && v44 != 0 ) /*0x1802fe030*/
  {
    sub_180307420(a1: (unsigned int)dword_18070A614); /*0x1802fe036*/
    byte_1813E46F0 = 0; /*0x1802fe03b*/
    *(_BYTE *)(a1 + 1633) = 1; /*0x1802fe042*/
    if ( *(_DWORD *)(sub_180307420(a1: v46) + 16) == 1000000 ) /*0x1802fe054*/
    {
      v48 = **(_QWORD **)(a1 + 1600); /*0x1802fe05d*/
      if ( v48 != 0 ) /*0x1802fe063*/
      {
        v49 = "perfect_bottom"; /*0x1802fe069*/
LABEL_80:
        v62 = 0; /*0x1802fe0a1*/
        v58 = 242; /*0x1802fe0af*/
        v63 = 2; /*0x1802fe0b7*/
        v60 = 37; /*0x1802fe0bf*/
        v59 = v49; /*0x1802fe0c7*/
        v61 = 1; /*0x1802fe0cc*/
        sub_180336A70(a1: v48 + 16, a2: &v58); /*0x1802fe0d4*/
        sub_180082820(a1: v48 + 16); /*0x1802fe0dd*/
        sub_180360060(a1: v52, a2: v51, a3: v53, a4: v54); /*0x1802fe0e2*/
        v55 = (CLibXact *)CLibXact::getInstance(); /*0x1802fe0ee*/
        CLibXact::SetSoundWithParam( /*0x1802fe119*/
          this: v55,
          a2: "se021_stg_clear",
          a3: -1.0,
          a4: -20000.0,
          a5: -20000.0,
          a6: 0.0,
          a7: false);
      }
    }
    else
    {
      v50 = sub_180307420(a1: v47); /*0x1802fe072*/
      if ( (unsigned __int8)sub_180178A50(a1: v50) != 0 ) /*0x1802fe081*/
      {
        v48 = **(_QWORD **)(a1 + 1600); /*0x1802fe08e*/
        if ( v48 != 0 ) /*0x1802fe094*/
        {
          v49 = "full_bottom"; /*0x1802fe09a*/
          goto LABEL_80; /*0x1802fe09a*/
        }
      }
    }
  }
  if ( v45 && *(_BYTE *)(a1 + 1633) != 0 ) /*0x1802fe147*/
  {
    if ( dword_1813E4A90 <= 1 || (v56 = sub_1802FDAD0, (int)qword_1813E4A94 > 1) ) /*0x1802fe160*/
      v56 = sub_1802FDA20; /*0x1802fe162*/
    v57 = *(_QWORD *)(a1 + 1696); /*0x1802fe169*/
    *(_QWORD *)(v57 + 32) = v56; /*0x1802fe170*/
    *(_DWORD *)(v57 + 40) = 0; /*0x1802fe174*/
    *(_DWORD *)(v57 + 60) = 0; /*0x1802fe178*/
    *(_BYTE *)(*(int *)(v57 + 40) + v57 + 76) = 1; /*0x1802fe180*/
  }
}
