__int64 __fastcall CLibXact::nosync_nowait(
        CLibXact *this,
        int a2,
        int a3,
        int a4,
        float a5,
        float a6,
        float a7,
        float a8,
        __int64 a9,
        const char *a10,
        const char *a11)
{
  __int64 v15; // r8
  _BYTE *v16; // rdx
  _BYTE *v17; // rcx
  _BYTE *v18; // r9
  __int64 i; // rax
  __int128 v20; // xmm0
  __int128 v21; // xmm1
  __int128 v22; // xmm0
  __int128 v23; // xmm1
  __int128 v24; // xmm0
  __int128 v25; // xmm1
  __int128 v26; // xmm0
  __int128 v27; // xmm1
  __int128 v28; // xmm1
  __int64 v29; // rax
  __int128 v30; // xmm0
  __int64 v31; // r8
  _BYTE v33[576]; // [rsp+20h] [rbp-4C8h] BYREF
  _BYTE Src[4]; // [rsp+260h] [rbp-288h] BYREF
  int v35; // [rsp+264h] [rbp-284h]
  int v36; // [rsp+268h] [rbp-280h]
  int v37; // [rsp+26Ch] [rbp-27Ch]
  float v38; // [rsp+270h] [rbp-278h]
  float v39; // [rsp+274h] [rbp-274h]
  float v40; // [rsp+278h] [rbp-270h]
  float v41; // [rsp+27Ch] [rbp-26Ch]
  __int64 v42; // [rsp+280h] [rbp-268h]
  _BYTE v43[260]; // [rsp+288h] [rbp-260h] BYREF
  _BYTE v44[276]; // [rsp+38Ch] [rbp-15Ch] BYREF

  avs2_core_244(a1: Src, a2: 0, a3: 568); /*0x18000a9fd*/
  Src[0] = 0; /*0x18000aa0b*/
  v38 = a5; /*0x18000aa25*/
  v39 = a6; /*0x18000aa37*/
  v35 = a2; /*0x18000aa40*/
  v36 = a3; /*0x18000aa47*/
  v37 = a4; /*0x18000aa4e*/
  v40 = a7; /*0x18000aa5e*/
  v42 = a9; /*0x18000aa67*/
  v41 = a8; /*0x18000aa6f*/
  if ( a10 != nullptr ) /*0x18000aa7b*/
    avs2_core_223(a1: v43, a2: a10, a3: 260); /*0x18000aa8e*/
  if ( a11 != nullptr ) /*0x18000aa97*/
    avs2_core_223(a1: v44, a2: a11, a3: 260); /*0x18000aaaa*/
  v16 = Src; /*0x18000aab0*/
  v17 = v33; /*0x18000aab8*/
  v18 = v33; /*0x18000aabd*/
  if ( (((unsigned __int8)v33 | (unsigned __int8)Src) & 0xF) != 0 ) /*0x18000aad3*/
  {
    memmove(a1: v33, Src, Size: 0x238u); /*0x18000ab5e*/
  }
  else
  {
    for ( i = 4; i != 0; --i ) /*0x18000aad5*/
    {
      v20 = *(_OWORD *)v16; /*0x18000aae0*/
      v21 = *((_OWORD *)v16 + 1); /*0x18000aae3*/
      v17 += 128; /*0x18000aae7*/
      v16 += 128; /*0x18000aaee*/
      *((_OWORD *)v17 - 8) = v20; /*0x18000aaf5*/
      v22 = *((_OWORD *)v16 - 6); /*0x18000aaf9*/
      *((_OWORD *)v17 - 7) = v21; /*0x18000aafd*/
      v23 = *((_OWORD *)v16 - 5); /*0x18000ab01*/
      *((_OWORD *)v17 - 6) = v22; /*0x18000ab05*/
      v24 = *((_OWORD *)v16 - 4); /*0x18000ab09*/
      *((_OWORD *)v17 - 5) = v23; /*0x18000ab0d*/
      v25 = *((_OWORD *)v16 - 3); /*0x18000ab11*/
      *((_OWORD *)v17 - 4) = v24; /*0x18000ab15*/
      v26 = *((_OWORD *)v16 - 2); /*0x18000ab19*/
      *((_OWORD *)v17 - 3) = v25; /*0x18000ab1d*/
      v27 = *((_OWORD *)v16 - 1); /*0x18000ab21*/
      *((_OWORD *)v17 - 2) = v26; /*0x18000ab25*/
      *((_OWORD *)v17 - 1) = v27; /*0x18000ab29*/
    }
    v28 = *((_OWORD *)v16 + 1); /*0x18000ab35*/
    v29 = *((_QWORD *)v16 + 6); /*0x18000ab39*/
    *(_OWORD *)v17 = *(_OWORD *)v16; /*0x18000ab3d*/
    v30 = *((_OWORD *)v16 + 2); /*0x18000ab40*/
    *((_OWORD *)v17 + 1) = v28; /*0x18000ab44*/
    *((_OWORD *)v17 + 2) = v30; /*0x18000ab48*/
    *((_QWORD *)v17 + 6) = v29; /*0x18000ab4c*/
  }
  if ( (unsigned int)CLibXact::add_cue(a1: this, a2: v33, a3: v15, a4: v18) == 0 ) /*0x18000ab72*/
    avs2_core_380(a1: "Sound::nosync_nowait", a2: &unk_18001C150, a3: v31); /*0x18000ab82*/
  return 0; /*0x18000ab8a*/
}
