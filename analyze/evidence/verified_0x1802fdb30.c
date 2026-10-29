// nosfix analysis 2026-10-29: Gameplay synchronization: drift below -5 ms accumulates toward a held update (10); drift >=10 ms accumulates and >25 ms with count>=5 runs an extra update. Executes delayed key sounds in frame units.
void __fastcall sub_1802FDB30(__int64 a1)
{
  __int64 v2; // rdx
  __int64 v3; // rcx
  __int64 v4; // r8
  __int64 v5; // r9
  CLibXact *Instance; // rax
  char *v7; // rax
  __int64 *v8; // rax
  __int64 v9; // rcx
  unsigned int v10; // eax
  float v11; // xmm0_4
  __int64 *v12; // rax
  char *v13; // rsi
  __int64 v14; // r8
  int v15; // eax
  __int64 v16; // r8
  int v17; // ebx
  __int64 v18; // rdi
  float v19; // xmm7_4
  CLibXact *v20; // rax
  __int128 *v21; // rax
  __int64 v22; // rdi
  __int64 v23; // rbx
  CLibXact *v24; // rax
  const char *v25; // rbx
  CLibXact *v26; // rax
  char v27; // al
  int v28; // eax
  int v29; // ecx
  int v30; // ecx
  int v31; // eax
  int v32; // edx
  int v33; // eax
  int v34; // ecx
  int v35; // eax
  int v36; // eax
  int v37; // ecx
  int v38; // edx
  int v39; // ecx
  _DWORD *v40; // rax
  bool v41; // bl
  char v42; // al
  bool v43; // di
  __int64 v44; // rcx
  __int64 v45; // rcx
  __int64 v46; // rbx
  const char *v47; // rax
  __int64 v48; // rax
  __int64 v49; // rdx
  __int64 v50; // rcx
  __int64 v51; // r8
  __int64 v52; // r9
  CLibXact *v53; // rax
  __int64 (__fastcall *v54)(); // rax
  __int64 v55; // rcx
  int v56; // [rsp+40h] [rbp-88h] BYREF
  const char *v57; // [rsp+48h] [rbp-80h]
  int v58; // [rsp+50h] [rbp-78h]
  int v59; // [rsp+54h] [rbp-74h]
  char v60; // [rsp+58h] [rbp-70h]
  int v61; // [rsp+5Ch] [rbp-6Ch]

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
    sub_18018B090(); /*0x1802fdc32*/
    v8 = sub_18018B090(); /*0x1802fdc37*/
    v9 = 0; /*0x1802fdc3c*/
    v10 = *((_DWORD *)v8 + 12) + 5; /*0x1802fdc42*/
    if ( v10 <= 0x5E ) /*0x1802fdc48*/
      v9 = v10; /*0x1802fdc48*/
    v11 = sub_1802C11C0(a1: v9); /*0x1802fdc4b*/
    sub_18033CF50(a1: 10, a2: 15, a3: byte_1805DDA28, v11); /*0x1802fdc6b*/
    v12 = sub_18018B090(); /*0x1802fdc70*/
    sub_18033CF50(a1: 10, a2: 30, a3: byte_1805DDA60, *((unsigned int *)v12 + 15)); /*0x1802fdc88*/
    sub_18033CF50(a1: 10, a2: 45, a3: byte_1805DDAE8, HIDWORD(qword_181366908)); /*0x1802fdca3*/
    sub_18033CF50(a1: 10, a2: 60, a3: byte_1805DDB20, HIDWORD(qword_181366914)); /*0x1802fdcbe*/
  }
  v13 = (char *)&xmmword_1813E4AD8 + 12; /*0x1802fdcca*/
  v14 = (unsigned int)(HIDWORD(qword_181366908) + HIDWORD(qword_181366914) + dword_18070A610); /*0x1802fdcdf*/
  dword_1813E56D8 = HIDWORD(qword_181366908) + HIDWORD(qword_181366914) + dword_18070A610; /*0x1802fdcfc*/
  do /*0x1802fde90*/
  {
    v15 = *((_DWORD *)v13 - 1); /*0x1802fdd12*/
    if ( v15 != -1 && v15 <= (int)v14 ) /*0x1802fdd21*/
    {
      v16 = (unsigned __int8)*(v13 - 7); /*0x1802fdd27*/
      if ( (unsigned __int8)(v16 - 1) <= 0x57u ) /*0x1802fdd32*/
      {
        v2 = 0; /*0x1802fdd34*/
        v3 = 0; /*0x1802fdd37*/
        while ( *((_BYTE *)&xmmword_1813E4AD8 + v3 + 5) != (_BYTE)v16 /*0x1802fdd61*/
             || *(_DWORD *)((char *)&xmmword_1813E4AD8 + v3 + 8) != -1
             || *(__int64 *)((char *)qword_1813E4AE8 + v3) != *(_QWORD *)(v13 + 4) )
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
      v17 = (unsigned __int8)*(v13 - 6); /*0x1802fdd93*/
      v18 = *(_QWORD *)(v13 + 4); /*0x1802fdd97*/
      sub_180360060(a1: v3, a2: v2, a3: v16, a4: v5); /*0x1802fdd9b*/
      v19 = *(float *)&dword_180654030[byte_1813E56DC]; /*0x1802fddaf*/
      v20 = (CLibXact *)CLibXact::getInstance(); /*0x1802fddb9*/
      CLibXact::SetSoundWithParam(this: v20, a2: v18, a3: (float)v17, a4: v19, a5: -20000.0, a6: 0.0, a7: false); /*0x1802fddde*/
      *((_DWORD *)v13 - 1) = -1; /*0x1802fdde4*/
      v14 = (unsigned int)dword_1813E56D8; /*0x1802fddeb*/
    }
    if ( *(_DWORD *)v13 != -1 && *(_DWORD *)v13 <= (int)v14 ) /*0x1802fde00*/
    {
      v2 = *((unsigned int *)v13 - 3); /*0x1802fde06*/
      if ( (int)v2 >= 0 ) /*0x1802fde0b*/
      {
        v3 = 0; /*0x1802fde0d*/
        v21 = &xmmword_1813E4AD8; /*0x1802fde10*/
        while ( *(_DWORD *)v21 != (_DWORD)v2 ) /*0x1802fde15*/
        {
          ++v3; /*0x1802fde17*/
          v21 = (__int128 *)((char *)v21 + 24); /*0x1802fde1a*/
          if ( (__int64)v21 >= (__int64)&dword_1813E56D8 ) /*0x1802fde21*/
            goto LABEL_39; /*0x1802fde21*/
        }
        if ( v3 != -1 ) /*0x1802fde29*/
        {
          v22 = 3 * v3; /*0x1802fde2b*/
          v23 = qword_1813E4AE8[3 * v3]; /*0x1802fde2f*/
          sub_180360060(a1: v3, a2: v2, a3: v14, a4: v5); /*0x1802fde37*/
          v24 = (CLibXact *)CLibXact::getInstance(); /*0x1802fde3c*/
          CLibXact::FadeOutSound(this: v24, a2: v23, a3: false); /*0x1802fde4b*/
          *((_DWORD *)&xmmword_1813E4AD8 + 2 * v22) = -1; /*0x1802fde51*/
          *((_WORD *)&xmmword_1813E4AD8 + 4 * v22 + 2) = 0; /*0x1802fde5d*/
          *((_BYTE *)&xmmword_1813E4AD8 + 8 * v22 + 6) = 0; /*0x1802fde66*/
          *((_QWORD *)&xmmword_1813E4AD8 + v22 + 1) = -1; /*0x1802fde6e*/
          qword_1813E4AE8[v22] = 0; /*0x1802fde7a*/
          v14 = (unsigned int)dword_1813E56D8; /*0x1802fde82*/
        }
      }
    }
LABEL_39:
    v13 += 24; /*0x1802fde89*/
  }
  while ( (__int64)v13 < (__int64)&qword_1813E56E4 ); /*0x1802fde90*/
  v25 = (const char *)(sub_180360060(a1: v3, a2: v2, a3: v14, a4: v5) + 680); /*0x1802fde9b*/
  v26 = (CLibXact *)CLibXact::getInstance(); /*0x1802fdea2*/
  if ( (int)CLibXact::GetSoundLabelLapse(this: v26, a2: v25) <= 0 ) /*0x1802fdef2*/
  {
    v27 = 0; /*0x1802fdef4*/
LABEL_48:
    *(_DWORD *)(a1 + 1644) = 0; /*0x1802fdf2d*/
    goto LABEL_49; /*0x1802fdf2d*/
  }
  v28 = qword_18070A718; /*0x1802fdef8*/
  v29 = *(_DWORD *)(a1 + 1644); /*0x1802fdefe*/
  *(_DWORD *)(a1 + 1656) = qword_18070A718; /*0x1802fdf04*/
  if ( v28 >= -5 ) /*0x1802fdf0d*/
  {
    v31 = v29 - 1; /*0x1802fdf13*/
    v30 = 0; /*0x1802fdf16*/
    if ( v31 > 0 ) /*0x1802fdf1b*/
      v30 = v31; /*0x1802fdf1b*/
  }
  else
  {
    v30 = v29 + 1; /*0x1802fdf0f*/
  }
  v27 = 0; /*0x1802fdf1e*/
  *(_DWORD *)(a1 + 1644) = v30; /*0x1802fdf20*/
  if ( v30 >= 10 ) /*0x1802fdf29*/
  {
    v27 = 1; /*0x1802fdf2b*/
    goto LABEL_48; /*0x1802fdf2b*/
  }
LABEL_49:
  *(_BYTE *)(a1 + 1652) = v27; /*0x1802fdf34*/
  v32 = qword_18070A718; /*0x1802fdf3a*/
  *(_DWORD *)(a1 + 1656) = qword_18070A718; /*0x1802fdf40*/
  if ( v32 > *(_DWORD *)(a1 + 1664) ) /*0x1802fdf4c*/
    *(_DWORD *)(a1 + 1664) = v32; /*0x1802fdf4e*/
  v33 = *(_DWORD *)(a1 + 1660); /*0x1802fdf54*/
  if ( v32 < 10 ) /*0x1802fdf5d*/
  {
    v35 = v33 - 1; /*0x1802fdf64*/
    v34 = 0; /*0x1802fdf66*/
    if ( v35 > 0 ) /*0x1802fdf6b*/
      v34 = v35; /*0x1802fdf6b*/
  }
  else
  {
    v34 = v33 + 1; /*0x1802fdf5f*/
  }
  *(_DWORD *)(a1 + 1660) = v34; /*0x1802fdf6e*/
  if ( v32 > 25 && v34 >= 5 ) /*0x1802fdf7c*/
  {
    v36 = v34 - 1; /*0x1802fdf7e*/
    v37 = 0; /*0x1802fdf81*/
    if ( v36 > 0 ) /*0x1802fdf86*/
      v37 = v36; /*0x1802fdf86*/
    ++*(_DWORD *)(a1 + 1636); /*0x1802fdf89*/
    *(_DWORD *)(a1 + 1660) = v37; /*0x1802fdf8f*/
    sub_1802FF090(a1); /*0x1802fdf98*/
    v38 = qword_1813DED30; /*0x1802fdf9d*/
    v39 = 0; /*0x1802fdfa3*/
    if ( (int)qword_1813DED30 > 0 ) /*0x1802fdfa8*/
    {
      v40 = &unk_1813DEA24; /*0x1802fdfaa*/
      do /*0x1802fdfca*/
      {
        if ( *v40 == 1 ) /*0x1802fdfb4*/
        {
          *v40 = 2; /*0x1802fdfb6*/
          v38 = qword_1813DED30; /*0x1802fdfbc*/
        }
        ++v39; /*0x1802fdfc2*/
        v40 += 7; /*0x1802fdfc4*/
      }
      while ( v39 < v38 ); /*0x1802fdfca*/
    }
  }
  if ( *(_BYTE *)(a1 + 1652) != 0 ) /*0x1802fdfd3*/
    ++*(_DWORD *)(a1 + 1640); /*0x1802fdfdf*/
  else
    sub_1802FF090(a1); /*0x1802fdfd8*/
  v41 = *(int *)(*(_QWORD *)(a1 + 1592) + 8LL) <= 0 && *(int *)(sub_180300D50() + 8) <= 0; /*0x1802fe001*/
  v42 = sub_180307B30(); /*0x1802fe003*/
  v43 = dword_18070A610 >= dword_18070A614; /*0x1802fe014*/
  if ( *(_BYTE *)(a1 + 1633) == 0 && v41 && v42 != 0 ) /*0x1802fe030*/
  {
    sub_180307420(a1: (unsigned int)dword_18070A614); /*0x1802fe036*/
    byte_1813E46F0 = 0; /*0x1802fe03b*/
    *(_BYTE *)(a1 + 1633) = 1; /*0x1802fe042*/
    if ( *(_DWORD *)(sub_180307420(a1: v44) + 16) == 1000000 ) /*0x1802fe054*/
    {
      v46 = **(_QWORD **)(a1 + 1600); /*0x1802fe05d*/
      if ( v46 != 0 ) /*0x1802fe063*/
      {
        v47 = "perfect_bottom"; /*0x1802fe069*/
LABEL_80:
        v60 = 0; /*0x1802fe0a1*/
        v56 = 242; /*0x1802fe0af*/
        v61 = 2; /*0x1802fe0b7*/
        v58 = 37; /*0x1802fe0bf*/
        v57 = v47; /*0x1802fe0c7*/
        v59 = 1; /*0x1802fe0cc*/
        sub_180336A70(a1: v46 + 16, a2: &v56); /*0x1802fe0d4*/
        sub_180082820(a1: v46 + 16); /*0x1802fe0dd*/
        sub_180360060(a1: v50, a2: v49, a3: v51, a4: v52); /*0x1802fe0e2*/
        v53 = (CLibXact *)CLibXact::getInstance(); /*0x1802fe0ee*/
        CLibXact::SetSoundWithParam( /*0x1802fe119*/
          this: v53,
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
      v48 = sub_180307420(a1: v45); /*0x1802fe072*/
      if ( (unsigned __int8)sub_180178A50(a1: v48) != 0 ) /*0x1802fe081*/
      {
        v46 = **(_QWORD **)(a1 + 1600); /*0x1802fe08e*/
        if ( v46 != 0 ) /*0x1802fe094*/
        {
          v47 = "full_bottom"; /*0x1802fe09a*/
          goto LABEL_80; /*0x1802fe09a*/
        }
      }
    }
  }
  if ( v43 && *(_BYTE *)(a1 + 1633) != 0 ) /*0x1802fe147*/
  {
    if ( dword_1813E4A90 <= 1 || (v54 = sub_1802FDAD0, (int)qword_1813E4A94 > 1) ) /*0x1802fe160*/
      v54 = sub_1802FDA20; /*0x1802fe162*/
    v55 = *(_QWORD *)(a1 + 1696); /*0x1802fe169*/
    *(_QWORD *)(v55 + 32) = v54; /*0x1802fe170*/
    *(_DWORD *)(v55 + 40) = 0; /*0x1802fe174*/
    *(_DWORD *)(v55 + 60) = 0; /*0x1802fe178*/
    *(_BYTE *)(*(int *)(v55 + 40) + v55 + 76) = 1; /*0x1802fe180*/
  }
}
