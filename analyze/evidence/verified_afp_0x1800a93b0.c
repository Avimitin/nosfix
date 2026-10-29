void __fastcall sub_1800A93B0(__int64 a1, unsigned int a2, int a3, char a4, char a5)
{
  __int64 v9; // r8
  __int64 v10; // r8
  __int64 v11; // rax
  signed int v12; // r14d
  __int64 v13; // r8
  int v14; // eax
  __int64 v15; // rcx
  signed int v16; // eax
  __int64 v17; // r15
  signed int v18; // ebp
  int v19; // r9d
  int v20; // ebx
  const char *v21; // rax
  __int64 v22; // r14
  int v23; // eax
  int v24; // r13d
  const char *v25; // rax
  int v26; // [rsp+48h] [rbp-40h]

  if ( (*(_DWORD *)(a1 + 32) & 0x1000000) != 0 ) /*0x1800a93d0*/
  {
    v9 = *(_QWORD *)(a1 + 320); /*0x1800a93d6*/
    if ( v9 == 0 ) /*0x1800a93e0*/
      return; /*0x1800a93e0*/
    v10 = v9 + 56; /*0x1800a93e6*/
  }
  else
  {
    v22 = *(_QWORD *)(a1 + 48); /*0x1800a95df*/
    if ( v22 == 0 ) /*0x1800a95e6*/
      goto LABEL_30; /*0x1800a95e6*/
    v23 = (unsigned __int16)(*(_WORD *)v22 & 0x7FE) >> 1; /*0x1800a95f1*/
    if ( v23 == 121 ) /*0x1800a95f6*/
    {
      v10 = *(_QWORD *)(v22 + 16) + 24LL; /*0x1800a9667*/
      goto LABEL_4; /*0x1800a966b*/
    }
    if ( v23 == 123 || v23 == 126 ) /*0x1800a9600*/
    {
LABEL_30:
      v10 = 0; /*0x1800a9602*/
    }
    else
    {
      v24 = sub_1800293D0(a1: *(_QWORD *)(a1 + 48)); /*0x1800a9612*/
      v25 = (const char *)sub_1801211B0(a1: (unsigned __int16)(*(_WORD *)v22 & 0x7FE) >> 1); /*0x1800a9621*/
      sub_180041A90( /*0x1800a9656*/
        a1: "afp-play",
        a2: "%s unknown tag(id=%d group=%d(%s)).",
        "afp_tag_get_tags",
        v24,
        (unsigned __int16)(*(_WORD *)v22 & 0x7FE) >> 1,
        v25);
      v10 = 0; /*0x1800a965b*/
    }
  }
LABEL_4:
  if ( v10 != 0 ) /*0x1800a93ed*/
  {
    v11 = *(_QWORD *)(v10 + 16); /*0x1800a93f3*/
    if ( *(int *)(v11 + 4) > 0 ) /*0x1800a93fb*/
    {
      v12 = 0; /*0x1800a9404*/
      v13 = *(_QWORD *)(v10 + 8); /*0x1800a9407*/
      v14 = *(_DWORD *)(*(unsigned int *)(v11 + 16) + v11 + 4LL * (int)a2); /*0x1800a9416*/
      v15 = v14 & 0xFFFFF; /*0x1800a941b*/
      v16 = (v14 & 0xFFF00000) >> 20; /*0x1800a9426*/
      if ( v16 > 0 ) /*0x1800a9433*/
      {
        v26 = a3; /*0x1800a9439*/
        v17 = v13 + 24 * v15; /*0x1800a944d*/
        v18 = v16; /*0x1800a9455*/
        do /*0x1800a9575*/
        {
          switch ( (unsigned __int16)(*(_WORD *)v17 & 0x7FE) >> 1 ) /*0x1800a9472*/
          {
            case 0x78: /*0x1800a9472*/
            case 0x79: /*0x1800a9472*/
            case 0x7B: /*0x1800a9472*/
            case 0x7C: /*0x1800a9472*/
            case 0x7D: /*0x1800a9472*/
            case 0x7E: /*0x1800a9472*/
            case 0x82: /*0x1800a9472*/
            case 0x83: /*0x1800a9472*/
            case 0x84: /*0x1800a9472*/
            case 0x85: /*0x1800a9472*/
            case 0x89: /*0x1800a9472*/
              break;
            case 0x7A: /*0x1800a9472*/
              if ( a4 != 0 ) /*0x1800a94f4*/
                *(_WORD *)(sub_1800CCFC0(a1, a2: *(_QWORD *)(v17 + 8), a3: *(_QWORD *)(a1 + 320), a4: 7, a5: 1) + 42) = a2; /*0x1800a9514*/
              break; /*0x1800a9519*/
            case 0x7F: /*0x1800a9472*/
              v19 = *(unsigned __int16 *)(*(_QWORD *)(v17 + 8) + 6LL); /*0x1800a94c9*/
              if ( v19 > v26 ) /*0x1800a94d3*/
                sub_180126680(a1, a2: v17, a3: a2, a4: v19, a5: a4); /*0x1800a94ea*/
              break; /*0x1800a94ef*/
            case 0x80: /*0x1800a9472*/
              sub_180127F00(a1, a2, a3: *(unsigned __int16 *)(*(_QWORD *)(v17 + 8) + 2LL), a4: 0xFFFF); /*0x1800a94bb*/
              break; /*0x1800a94c0*/
            case 0x81: /*0x1800a9472*/
              if ( a4 != 0 ) /*0x1800a948c*/
                sub_18008A210(a1: v17, a2: *(_QWORD *)(a1 + 320)); /*0x1800a949c*/
              break; /*0x1800a94a1*/
            case 0x88: /*0x1800a9472*/
              sub_180127D50(a1, a2: v17); /*0x1800a947f*/
              break; /*0x1800a9484*/
            default:
              v20 = sub_1800293D0(a1: v17); /*0x1800a9523*/
              v21 = (const char *)sub_1801211B0(a1: (unsigned __int16)(*(_WORD *)v17 & 0x7FE) >> 1); /*0x1800a9531*/
              sub_180041A90( /*0x1800a9566*/
                a1: "afp-play",
                a2: "%s unknown tag(id=%d group=%d(%s)).",
                "afp_set_play_data_frame_core",
                v20,
                (unsigned __int16)(*(_WORD *)v17 & 0x7FE) >> 1,
                v21);
              break; /*0x1800a9566*/
          }
          ++v12; /*0x1800a956b*/
          v17 += 24; /*0x1800a956e*/
        }
        while ( v12 < v18 ); /*0x1800a9575*/
      }
      if ( a4 != 0 && a5 == 0 ) /*0x1800a9594*/
      {
        if ( *(_QWORD *)(a1 + 336) != 0 ) /*0x1800a95a0*/
          sub_1800BF770(a1); /*0x1800a95bb*/
        if ( (*(_DWORD *)(a1 + 36) & 0x800000) != 0 ) /*0x1800a95c7*/
          sub_18013FB10(a1); /*0x1800a95cc*/
      }
    }
  }
}
