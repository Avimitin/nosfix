__int64 __fastcall CLibXact::sync_wait(
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
  __int64 v31; // rcx
  unsigned int v32; // ebx
  _BYTE v34[576]; // [rsp+30h] [rbp-4D8h] BYREF
  _BYTE v35[16]; // [rsp+270h] [rbp-298h] BYREF
  _BYTE Src[4]; // [rsp+280h] [rbp-288h] BYREF
  int v37; // [rsp+284h] [rbp-284h]
  int v38; // [rsp+288h] [rbp-280h]
  int v39; // [rsp+28Ch] [rbp-27Ch]
  float v40; // [rsp+290h] [rbp-278h]
  float v41; // [rsp+294h] [rbp-274h]
  float v42; // [rsp+298h] [rbp-270h]
  float v43; // [rsp+29Ch] [rbp-26Ch]
  __int64 v44; // [rsp+2A0h] [rbp-268h]
  _BYTE v45[260]; // [rsp+2A8h] [rbp-260h] BYREF
  _BYTE v46[276]; // [rsp+3ACh] [rbp-15Ch] BYREF

  avs2_core_244(a1: Src, a2: 0, a3: 568); /*0x18000ac0d*/
  Src[0] = 1; /*0x18000ac1b*/
  v40 = a5; /*0x18000ac35*/
  v41 = a6; /*0x18000ac47*/
  v37 = a2; /*0x18000ac50*/
  v38 = a3; /*0x18000ac57*/
  v39 = a4; /*0x18000ac5e*/
  v42 = a7; /*0x18000ac6e*/
  v44 = a9; /*0x18000ac77*/
  v43 = a8; /*0x18000ac7f*/
  if ( a10 != nullptr ) /*0x18000ac8b*/
    avs2_core_223(a1: v45, a2: a10, a3: 260); /*0x18000ac9e*/
  if ( a11 != nullptr ) /*0x18000aca7*/
    avs2_core_223(a1: v46, a2: a11, a3: 260); /*0x18000acba*/
  v16 = Src; /*0x18000acc0*/
  v17 = v34; /*0x18000acc8*/
  v18 = v34; /*0x18000accd*/
  if ( (((unsigned __int8)v34 | (unsigned __int8)Src) & 0xF) != 0 ) /*0x18000ace3*/
  {
    memmove(a1: v34, Src, Size: 0x238u); /*0x18000ad6e*/
  }
  else
  {
    for ( i = 4; i != 0; --i ) /*0x18000ace5*/
    {
      v20 = *(_OWORD *)v16; /*0x18000acf0*/
      v21 = *((_OWORD *)v16 + 1); /*0x18000acf3*/
      v17 += 128; /*0x18000acf7*/
      v16 += 128; /*0x18000acfe*/
      *((_OWORD *)v17 - 8) = v20; /*0x18000ad05*/
      v22 = *((_OWORD *)v16 - 6); /*0x18000ad09*/
      *((_OWORD *)v17 - 7) = v21; /*0x18000ad0d*/
      v23 = *((_OWORD *)v16 - 5); /*0x18000ad11*/
      *((_OWORD *)v17 - 6) = v22; /*0x18000ad15*/
      v24 = *((_OWORD *)v16 - 4); /*0x18000ad19*/
      *((_OWORD *)v17 - 5) = v23; /*0x18000ad1d*/
      v25 = *((_OWORD *)v16 - 3); /*0x18000ad21*/
      *((_OWORD *)v17 - 4) = v24; /*0x18000ad25*/
      v26 = *((_OWORD *)v16 - 2); /*0x18000ad29*/
      *((_OWORD *)v17 - 3) = v25; /*0x18000ad2d*/
      v27 = *((_OWORD *)v16 - 1); /*0x18000ad31*/
      *((_OWORD *)v17 - 2) = v26; /*0x18000ad35*/
      *((_OWORD *)v17 - 1) = v27; /*0x18000ad39*/
    }
    v28 = *((_OWORD *)v16 + 1); /*0x18000ad45*/
    v29 = *((_QWORD *)v16 + 6); /*0x18000ad49*/
    *(_OWORD *)v17 = *(_OWORD *)v16; /*0x18000ad4d*/
    v30 = *((_OWORD *)v16 + 2); /*0x18000ad50*/
    *((_OWORD *)v17 + 1) = v28; /*0x18000ad54*/
    *((_OWORD *)v17 + 2) = v30; /*0x18000ad58*/
    *((_QWORD *)v17 + 6) = v29; /*0x18000ad5c*/
  }
  if ( (unsigned int)CLibXact::add_cue(a1: this, a2: v34, a3: v15, a4: v18) != 0 ) /*0x18000ad82*/
  {
    while ( (unsigned int)avs2_core_26(a1: *((unsigned int *)this + 145411), a2: 1, a3: 16, a4: v35, a5: 60000) != 0 ) /*0x18000adb0*/
    {
      avs2_core_380(a1: "Sound::sync_wait", a2: "Event timeout (%dtimes)\n", ++dword_189646954); /*0x18000add1*/
      CLibXact::OutputCueLog(this, a2: "Sound::sync_wait"); /*0x18000ade1*/
    }
    v31 = *((unsigned int *)this + 145413); /*0x18000ae10*/
    dword_189646954 = 0; /*0x18000ae16*/
    avs2_core_16(a1: v31); /*0x18000ae20*/
    v32 = *((_DWORD *)this + 142); /*0x18000ae2c*/
    avs2_core_17(a1: *((unsigned int *)this + 145413)); /*0x18000ae32*/
    CLibXact::del_cue(this); /*0x18000ae3b*/
    avs2_core_25(a1: *((unsigned int *)this + 145412), a2: 1); /*0x18000ae4b*/
    return v32; /*0x18000ae51*/
  }
  else
  {
    avs2_core_380(a1: "Sound::sync_wait", a2: byte_18001C150); /*0x18000ae63*/
    return 0xFFFFFFFFLL; /*0x18000ae69*/
  }
}
