int __fastcall sub_1802FEDC0(__int64 a1)
{
  _UNKNOWN **v1; // rax
  int v3; // r15d
  int v4; // r12d
  __int64 v5; // rax
  __int64 v6; // r8
  __int64 v7; // r9
  int v8; // edx
  int v9; // ecx
  unsigned int v10; // ebp
  __int64 v11; // r13
  __int64 v12; // rdx
  __int64 v13; // rcx
  __int64 v14; // r8
  __int64 v15; // r9
  __int64 v16; // rsi
  unsigned int *v17; // rbx
  __int64 v18; // rcx
  unsigned int v19; // edi
  _QWORD *v20; // rdi
  _QWORD *v21; // rsi
  __int64 v22; // rdx
  __int64 v23; // rcx
  __int64 v24; // r8
  __int64 v25; // r9
  __int64 v26; // rdx
  __int64 v27; // rcx
  __int64 v28; // r8
  __int64 v29; // r9
  __int64 v30; // rcx
  __int64 v31; // rax
  __int64 v32; // rax
  char v33; // cl
  char v34; // bl
  __int64 v35; // rcx
  __int64 v36; // rcx
  __int64 v37; // r8
  __int64 v38; // r9
  __int64 v39; // rdx
  char v40; // bl
  __int64 v41; // rdi
  float v42; // xmm6_4
  CLibXact *Instance; // rax
  __int64 v45; // [rsp+48h] [rbp-90h] BYREF
  _QWORD v46[4]; // [rsp+50h] [rbp-88h] BYREF
  _QWORD v47[4]; // [rsp+70h] [rbp-68h] BYREF
  _UNKNOWN *retaddr; // [rsp+D8h] [rbp+0h] BYREF

  v1 = &retaddr; /*0x1802fedc0*/
  if ( *(_BYTE *)(a1 + 1618) == 0 ) /*0x1802fee08*/
  {
    v3 = dword_18070A610; /*0x1802fee0e*/
    v4 = *(_DWORD *)(*(_QWORD *)(a1 + 1608) + 172LL) - 1; /*0x1802fee23*/
    v5 = sub_18019FF80(); /*0x1802fee26*/
    v8 = *(_DWORD *)(v5 + 200); /*0x1802fee2b*/
    if ( v8 < 0 ) /*0x1802fee35*/
      v8 = 0; /*0x1802fee35*/
    v9 = 4; /*0x1802fee38*/
    if ( v8 < 4 ) /*0x1802fee3f*/
      v9 = v8; /*0x1802fee3f*/
    v10 = *(_DWORD *)(v5 + 40LL * v9); /*0x1802fee49*/
    v11 = sub_180360060(a1: v9, a2: 5LL * v9, a3: v6, a4: v7); /*0x1802fee51*/
    v16 = sub_180360060(a1: v13, a2: v12, a3: v14, a4: v15); /*0x1802fee59*/
    v17 = (unsigned int *)(v16 + 424); /*0x1802fee5c*/
    v45 = v16 + 424; /*0x1802fee63*/
    avs2_core_16(a1: *(unsigned int *)(v16 + 424)); /*0x1802fee6a*/
    __wind /*0xf1c000000000003c*/
    {
      avs2_core_16(a1: *v17); /*0x1802fee73*/
      v18 = *v17; /*0x1802fee79*/
      if ( *(_DWORD *)(v16 + 428) == v10 ) /*0x1802fee81*/
      {
        avs2_core_16(a1: v18); /*0x1802fee8d*/
        v19 = *(_DWORD *)(v16 + 428); /*0x1802fee93*/
        if ( (__int64)(*(_QWORD *)(v16 + 408) - *(_QWORD *)(v16 + 400)) >> 2 != 0 ) /*0x1802feeb1*/
          v19 = *(_DWORD *)(*(_QWORD *)(v16 + 408) - 4LL); /*0x1802feeb3*/
        avs2_core_17(a1: *v17); /*0x1802feeb8*/
        avs2_core_17(a1: *v17); /*0x1802feec0*/
      }
      else
      {
        avs2_core_17(a1: v18); /*0x1802fee83*/
        v19 = v10; /*0x1802fee89*/
      }
      (*(void (__fastcall **)(__int64, _QWORD *, _QWORD))(*(_QWORD *)(v16 + 440) + 24LL))( /*0x1802feed8*/
        a1: v16 + 440,
        a2: v46,
        a3: v19);
      __wind /*0xf1c0000000000030*/
      {
        v20 = v47; /*0x1802feedc*/
        if ( v47[3] >= 0x10u ) /*0x1802feeea*/
          v20 = (_QWORD *)v47[0]; /*0x1802feeea*/
        v21 = v46; /*0x1802feef0*/
        if ( v46[3] >= 0x10u ) /*0x1802feefb*/
          v21 = (_QWORD *)v46[0]; /*0x1802feefb*/
        while ( (unsigned __int8)sub_180359270(a1: v21, a2: v20) == 0 ) /*0x1802fef0e*/
          ; /*0x1802fef01*/
      }
      __unwind
      {
        sub_18009D860(a1: v46); /*0x18038c891*/
      }
      sub_1800773D0(a1: v47); /*0x1802fef15*/
      sub_1800773D0(a1: v46); /*0x1802fef1f*/
      avs2_core_17(a1: *v17); /*0x1802fef27*/
    }
    __unwind
    {
      sub_1800931C0(a1: &v45); /*0x18038c885*/
    }
    sub_180360060(a1: v23, a2: v22, a3: v24, a4: v25); /*0x1802fef2d*/
    swprintf(Buffer: word_18070A618, BufferCount: 0x100u, Format: &word_1803FE99C, "_backtrack"); /*0x1802fef4c*/
    sub_180360060(a1: v27, a2: v26, a3: v28, a4: v29); /*0x1802fef51*/
    LODWORD(v1) = swprintf( /*0x1802fef69*/
                    Buffer: (wchar_t *const)(v11 + 680),
                    BufferCount: 0x100u,
                    Format: (const wchar_t *const)"_backtrack");
    if ( v3 >= v4 ) /*0x1802fef71*/
    {
      sub_180173BB0(); /*0x1802fef77*/
      v31 = sub_180173AD0(a1: v30, a2: v10); /*0x1802fef7e*/
      v32 = sub_18018B090(a1: *(unsigned __int8 *)(v31 + 264)); /*0x1802fef8a*/
      v34 = v33 + *(_BYTE *)(v32 + 105); /*0x1802fef93*/
      sub_180173BB0(); /*0x1802fef95*/
      v36 = *(unsigned __int8 *)(sub_180173AD0(a1: v35, a2: v10) + 264); /*0x1802fefa1*/
      v39 = (unsigned __int8)v34; /*0x1802fefa8*/
      if ( (char)v36 < v34 ) /*0x1802fefad*/
        v39 = (unsigned int)v36; /*0x1802fefad*/
      v40 = -20; /*0x1802fefb0*/
      if ( (char)v39 > -20 ) /*0x1802fefbb*/
        v40 = v39; /*0x1802fefbb*/
      v41 = sub_180360060(a1: v36, a2: v39, a3: v37, a4: v38); /*0x1802fefc3*/
      if ( *(_DWORD *)(v41 + 940) != 0 ) /*0x1802fefcd*/
      {
        off_18070B840(); /*0x1802fefd6*/
        *(_DWORD *)(v41 + 940) = 0; /*0x1802fefdc*/
      }
      v42 = *(float *)&dword_180654030[v40]; /*0x1802feff1*/
      if ( v10 != 0 ) /*0x1802feff8*/
      {
        Instance = (CLibXact *)CLibXact::getInstance(); /*0x1802ff001*/
        CLibXact::SetSoundWithParam( /*0x1802ff034*/
          this: Instance,
          a2: "_backtrack",
          a3: -1.0,
          a4: v42,
          a5: -20000.0,
          a6: 0.0,
          a7: true);
        *(_DWORD *)(v41 + 940) = v10; /*0x1802ff03a*/
      }
      *(_BYTE *)(a1 + 1618) = 1; /*0x1802ff040*/
      LODWORD(v1) = timeGetTime(); /*0x1802ff048*/
      HIDWORD(qword_18070A718) = (_DWORD)v1; /*0x1802ff04e*/
    }
  }
  return (int)v1; /*0x1802ff054*/
}
