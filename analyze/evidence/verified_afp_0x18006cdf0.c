void __fastcall sub_18006CDF0(__int64 a1, __int64 a2)
{
  int v3; // esi
  int v5; // r13d
  int v6; // eax
  __int16 *v7; // rdx
  __int64 v8; // rbp
  __int64 v9; // rdi
  __int64 v10; // r15
  __int64 v11; // r14
  __int64 v12; // rdx
  __int64 v13; // rbp
  int v14; // edi
  BOOL v15; // r8d
  int v16; // ecx
  int v17; // edx
  __int16 v18; // ax
  __int64 v19; // rax
  int v20; // ecx
  __int64 v21; // rbp
  __int64 v22; // r14
  int v23; // r12d
  __int64 v24; // rax
  __int64 v25; // r10
  int v26; // r9d
  __int64 v27; // rdx
  int v28; // ebp
  __int64 i; // rbx
  int v30; // r13d
  __int64 v31; // r14
  __int64 v32; // rdi
  __int64 v33; // rcx
  __int64 v34; // rax
  int v35; // r9d
  __int64 v36; // rcx
  __int64 v37; // r13
  char v38; // di
  char v39; // di
  int v40; // eax
  __int64 v41; // [rsp+80h] [rbp-98h]
  __int64 v42; // [rsp+80h] [rbp-98h]
  __int64 v43; // [rsp+88h] [rbp-90h]
  BOOL v44; // [rsp+90h] [rbp-88h]
  BOOL v45; // [rsp+90h] [rbp-88h]
  __int64 v46; // [rsp+98h] [rbp-80h]
  __int64 v47; // [rsp+A0h] [rbp-78h]
  __int64 v48; // [rsp+A0h] [rbp-78h]
  __int64 v49; // [rsp+A8h] [rbp-70h]
  int v50; // [rsp+B0h] [rbp-68h]
  __int64 v51; // [rsp+B8h] [rbp-60h]
  unsigned int v52; // [rsp+C0h] [rbp-58h]
  int v53; // [rsp+C8h] [rbp-50h]
  int v54; // [rsp+D0h] [rbp-48h]

  v3 = -1; /*0x18006ce06*/
  v5 = -1; /*0x18006ce0e*/
  v6 = *(_DWORD *)(a2 + 44); /*0x18006ce11*/
  if ( v6 >= 3 && v6 <= 5 || (sub_1800D1510(a1: "MotionBase", a2), (v6 = *(_DWORD *)(a2 + 44)) >= 3) ) /*0x18006ce33*/
  {
    v7 = *(__int16 **)(a2 + 24); /*0x18006ce39*/
    v8 = 16 * (*v7 - (__int64)v6); /*0x18006ce47*/
    v9 = *((_QWORD *)v7 + 1); /*0x18006ce4b*/
    LODWORD(v10) = sub_180051210(a1: v9 + v8); /*0x18006ce58*/
    v11 = v9 + v8 + 16; /*0x18006ce5b*/
    v52 = *(unsigned __int16 *)(v9 + v8 + 18); /*0x18006ce6a*/
    if ( v52 < 0x100 ) /*0x18006ce71*/
      sub_1800D0F70(a1: 155, a2: *(unsigned __int16 *)(a2 + 12), a3: "name", a4: v9 + v8 + 16); /*0x18006d5fe*/
    v13 = sub_18004D490(a1: v9 + v8 + 32, a2: 33); /*0x18006ce88*/
    if ( v13 == 0 ) /*0x18006ce8e*/
      sub_1800D1160(a1: 155, a2: *(unsigned __int16 *)(a2 + 12), a3: (unsigned int)"values", a4: 33, a5: v11 + 16); /*0x18006d5e0*/
    if ( *(int *)(a2 + 44) >= 4 ) /*0x18006ce98*/
    {
      v3 = sub_180051210(a1: v11 + 32); /*0x18006cea3*/
      if ( *(int *)(a2 + 44) >= 5 ) /*0x18006cea9*/
        v5 = sub_180051210(a1: v11 + 48); /*0x18006ceb4*/
    }
    v14 = 0; /*0x18006ceb7*/
    v15 = true; /*0x18006ceb9*/
    if ( v13 != 0 ) /*0x18006cec2*/
    {
      v14 = *(_DWORD *)(v13 + 40); /*0x18006cec4*/
      if ( v14 > 0 ) /*0x18006cec9*/
        v15 = (unsigned __int8)sub_18004FF40(a1: **(_QWORD **)(v13 + 32), a2: v12, a3: 1) == 0; /*0x18006cee4*/
    }
    v16 = *(__int16 *)(a1 + 14); /*0x18006cee8*/
    v17 = *(__int16 *)(a1 + 30); /*0x18006ceee*/
    if ( v16 >= v17 ) /*0x18006cef6*/
      v17 = *(__int16 *)(a1 + 14); /*0x18006cef6*/
    if ( v17 >= v14 ) /*0x18006cefb*/
    {
      *(_WORD *)(a1 + 30) = v17; /*0x18006cf0e*/
    }
    else
    {
      v18 = v14; /*0x18006ceff*/
      v17 = v14; /*0x18006cf01*/
      if ( v16 >= v14 ) /*0x18006cf03*/
        v18 = *(_WORD *)(a1 + 14); /*0x18006cf03*/
      *(_WORD *)(a1 + 30) = v18; /*0x18006cf06*/
    }
    if ( v3 == -1 || v5 == -1 ) /*0x18006cf1d*/
    {
      v3 = 0; /*0x18006cf1f*/
      v5 = v17; /*0x18006cf21*/
    }
    if ( v16 < v5 ) /*0x18006cf27*/
    {
      v44 = v15; /*0x18006cf3a*/
      v19 = sub_1800407A0(a1: *(_QWORD *)(a1 + 56), a2: (unsigned int)(8 * v5)); /*0x18006cf42*/
      v15 = v44; /*0x18006cf47*/
      v20 = *(__int16 *)(a1 + 14); /*0x18006cf4f*/
      *(_QWORD *)(a1 + 56) = v19; /*0x18006cf58*/
      if ( v20 < v5 ) /*0x18006cf60*/
      {
        v47 = v13; /*0x18006cfbd*/
        v21 = v20; /*0x18006cfc5*/
        v41 = v11; /*0x18006cfc8*/
        v22 = a1; /*0x18006cfd0*/
        v23 = v20; /*0x18006cfdb*/
        do /*0x18006d068*/
        {
          v24 = sub_1800402D0(a1: 144); /*0x18006cff0*/
          v25 = *(_QWORD *)(v22 + 56); /*0x18006cff5*/
          *(_OWORD *)v24 = 0; /*0x18006cff9*/
          *(_OWORD *)(v24 + 16) = 0; /*0x18006cffd*/
          *(_OWORD *)(v24 + 32) = 0; /*0x18006d002*/
          *(_OWORD *)(v24 + 48) = 0; /*0x18006d007*/
          *(_OWORD *)(v24 + 64) = xmmword_180156000; /*0x18006d00c*/
          *(_OWORD *)(v24 + 80) = xmmword_180156010; /*0x18006d011*/
          *(_OWORD *)(v24 + 96) = xmmword_180156020; /*0x18006d016*/
          *(_OWORD *)(v24 + 112) = xmmword_180156030; /*0x18006d01b*/
          *(_WORD *)(v24 + 4) = v23++; /*0x18006d020*/
          v26 = dword_180184424; /*0x18006d028*/
          *(_QWORD *)(v24 + 136) = 0; /*0x18006d02f*/
          *(_QWORD *)(v25 + 8 * v21++) = v24; /*0x18006d03a*/
          *(_WORD *)(v24 + 2) = 526; /*0x18006d041*/
          *(_BYTE *)(v24 + 9) = 2; /*0x18006d048*/
          *(_DWORD *)(v24 + 32) = v26; /*0x18006d04c*/
          *(_DWORD *)(v24 + 36) = v26; /*0x18006d050*/
          *(_DWORD *)(v24 + 128) = -1; /*0x18006d054*/
          *(_DWORD *)(v24 + 132) = 0; /*0x18006d05e*/
        }
        while ( v23 < v5 ); /*0x18006d068*/
        a1 = v22; /*0x18006d070*/
        v15 = v44; /*0x18006d079*/
        v13 = v47; /*0x18006d08e*/
        v11 = v41; /*0x18006d096*/
      }
      *(_WORD *)(a1 + 14) = v5; /*0x18006d0ae*/
    }
    if ( v3 < v5 ) /*0x18006d0b7*/
    {
      v27 = 0; /*0x18006d0c5*/
      v46 = 0; /*0x18006d0c7*/
      v49 = (int)v10; /*0x18006d0cf*/
      v54 = v5; /*0x18006d0d7*/
      v53 = v14; /*0x18006d0df*/
      v45 = v15; /*0x18006d0e6*/
      v48 = v13; /*0x18006d0ee*/
      v28 = 0; /*0x18006d0f6*/
      v42 = v11; /*0x18006d0f8*/
      v43 = a2; /*0x18006d100*/
      for ( i = v3; ; ++i ) /*0x18006d108*/
      {
        v30 = *(__int16 *)(a1 + 40); /*0x18006d110*/
        v31 = *(_QWORD *)(a1 + 48); /*0x18006d116*/
        v32 = *(_QWORD *)(*(_QWORD *)(a1 + 56) + 8 * i); /*0x18006d11b*/
        if ( *(unsigned __int8 *)(v32 + 10) == v30 ) /*0x18006d126*/
          goto LABEL_42; /*0x18006d126*/
        v33 = 8 * (unsigned int)(unsigned __int8)*(_WORD *)(a1 + 40); /*0x18006d130*/
        *(_BYTE *)(v32 + 10) = *(_WORD *)(a1 + 40); /*0x18006d133*/
        v34 = sub_1800402D0(a1: v33); /*0x18006d137*/
        *(_QWORD *)(v32 + 136) = v34; /*0x18006d13c*/
        v27 = 0; /*0x18006d143*/
        if ( v30 > 0 ) /*0x18006d148*/
        {
          v50 = v10; /*0x18006d151*/
          v51 = a1; /*0x18006d159*/
          a1 = 0; /*0x18006d161*/
          while ( 1 ) /*0x18006d164*/
          {
            v35 = *(unsigned __int16 *)(v31 + 2 * a1); /*0x18006d164*/
            if ( v35 == 2938 ) /*0x18006d170*/
            {
              v10 = sub_1800402D0(a1: 28); /*0x18006d17c*/
              sub_180126210(a1: v10); /*0x18006d182*/
            }
            else
            {
              if ( v35 != 2841 ) /*0x18006d190*/
              {
                sub_180042450(a1: "afp-motion", a2: "%s unknown filter type(%d)", a3: "afp_keyframe_base_check_filters"); /*0x18006d1e8*/
LABEL_42:
                v34 = *(_QWORD *)(v32 + 136); /*0x18006d1ed*/
                break; /*0x18006d1ed*/
              }
              v10 = sub_1800402D0(a1: 8); /*0x18006d19c*/
              sub_180126270(a1: v10); /*0x18006d1a2*/
            }
            *(_QWORD *)(*(_QWORD *)(v32 + 136) + 8 * a1++) = v10; /*0x18006d1ae*/
            if ( a1 >= v30 ) /*0x18006d1b8*/
            {
              LODWORD(v10) = v50; /*0x18006d1ba*/
              a1 = v51; /*0x18006d1c2*/
              v34 = *(_QWORD *)(v32 + 136); /*0x18006d1ca*/
              break; /*0x18006d1d1*/
            }
          }
        }
        if ( v28 < v53 ) /*0x18006d1fb*/
        {
          v27 = *(_QWORD *)(((int)(v28 + ((unsigned int)(v28 >> 2) >> 29)) & 0xFFFFFFFFFFFFFFF8uLL) /*0x18006d237*/
                          + *(_QWORD *)(v48 + 32))
              + 16LL * (v28 % 8);
          if ( *(_WORD *)v27 != 0 /*0x18006d279*/
            && (((unsigned int)(*(__int16 *)(v27 + 2) - 164) >> 31)
              & (0x3000u >> *(_WORD *)v27)
              & 1
              & (*(_QWORD *)(v27 + 8) == 0)) == 0 )
          {
            v46 = *(_QWORD *)(((int)(v28 + ((unsigned int)(v28 >> 2) >> 29)) & 0xFFFFFFFFFFFFFFF8uLL) /*0x18006d299*/
                            + *(_QWORD *)(v48 + 32))
                + 16LL * (v28 % 8);
          }
          else
          {
            v36 = v46; /*0x18006d283*/
            if ( !v45 ) /*0x18006d28b*/
              v36 = *(_QWORD *)(((int)(v28 + ((unsigned int)(v28 >> 2) >> 29)) & 0xFFFFFFFFFFFFFFF8uLL) /*0x18006d28b*/
                              + *(_QWORD *)(v48 + 32))
                  + 16LL * (v28 % 8);
            v46 = v36; /*0x18006d28f*/
          }
        }
        if ( v34 == 0 ) /*0x18006d2a4*/
          goto LABEL_81; /*0x18006d2a4*/
        if ( (int)v10 < 0 ) /*0x18006d2ad*/
          goto LABEL_81; /*0x18006d2ad*/
        if ( (int)v10 >= *(unsigned __int8 *)(v32 + 10) ) /*0x18006d2ba*/
          goto LABEL_81; /*0x18006d2ba*/
        v37 = *(_QWORD *)(v34 + 8 * v49); /*0x18006d2c8*/
        if ( v37 == 0 ) /*0x18006d2cf*/
          goto LABEL_81; /*0x18006d2cf*/
        if ( *(_BYTE *)v37 != 0 ) /*0x18006d2dd*/
          break; /*0x18006d2dd*/
        switch ( v52 ) /*0x18006d2ee*/
        {
          case 0x1A2u: /*0x18006d2ee*/
            sub_1801226A0(a1: v37, a2: v46); /*0x18006d54b*/
            goto LABEL_81; /*0x18006d550*/
          case 0x219u: /*0x18006d2ee*/
            sub_1801226D0(a1: v37, a2: v46); /*0x18006d536*/
            goto LABEL_81; /*0x18006d53b*/
          case 0x25Du: /*0x18006d2ee*/
            v39 = *(_BYTE *)(v37 + 2) & 0x7F; /*0x18006d50a*/
            if ( (unsigned __int8)sub_18004BC90(a1: v46, a2: v27) != 0 ) /*0x18006d522*/
              v39 |= 0x80u; /*0x18006d522*/
            *(_BYTE *)(v37 + 2) = v39; /*0x18006d525*/
            goto LABEL_81; /*0x18006d529*/
          case 0x481u: /*0x18006d2ee*/
            *(float *)(v37 + 16) = sub_180057760(a1: v46, a2: v27); /*0x18006d4f5*/
            goto LABEL_81; /*0x18006d4fb*/
          case 0x5D6u: /*0x18006d2ee*/
            v40 = sub_180051210(a1: v46); /*0x18006d5a3*/
            if ( v40 <= 0 ) /*0x18006d5ac*/
              v40 = 0; /*0x18006d5ac*/
            if ( v40 >= 255 ) /*0x18006d5b9*/
              LOBYTE(v40) = -1; /*0x18006d5b9*/
            *(_BYTE *)(v37 + 3) = v40; /*0x18006d5bc*/
            goto LABEL_81; /*0x18006d5c0*/
          case 0x7B1u: /*0x18006d2ee*/
            *(float *)(v37 + 8) = sub_180057760(a1: v46, a2: v27); /*0x18006d590*/
            goto LABEL_81; /*0x18006d596*/
          case 0x7B2u: /*0x18006d2ee*/
            *(float *)(v37 + 12) = sub_180057760(a1: v46, a2: v27); /*0x18006d578*/
            goto LABEL_81; /*0x18006d57e*/
          case 0x7BEu: /*0x18006d2ee*/
            *(float *)(v37 + 24) = sub_180057760(a1: v46, a2: v27); /*0x18006d4e0*/
            goto LABEL_81; /*0x18006d4e6*/
          case 0x7BFu: /*0x18006d2ee*/
            sub_180122710(a1: v37, a2: v46); /*0x18006d40b*/
            goto LABEL_81; /*0x18006d410*/
          default:
            break; /*0x18006d387*/
        }
        if ( v52 != 1984 ) /*0x18006d387*/
        {
          if ( v52 == 1985 ) /*0x18006d3a9*/
          {
            v38 = *(_BYTE *)(v37 + 2) & 0xDF; /*0x18006d3b8*/
            if ( (unsigned __int8)sub_18004BC90(a1: v46, a2: v27) == 0 ) /*0x18006d3ca*/
              v38 |= 0x20u; /*0x18006d3ca*/
            *(_BYTE *)(v37 + 2) = v38; /*0x18006d3cd*/
            goto LABEL_81; /*0x18006d3d1*/
          }
LABEL_70:
          sub_1800D0F70(a1: 155, a2: *(unsigned __int16 *)(v43 + 12), a3: "name", a4: v42); /*0x18006d3d6*/
          goto LABEL_81; /*0x18006d3fb*/
        }
        sub_180122740(a1: v37, a2: v46); /*0x18006d394*/
LABEL_81:
        ++v3; /*0x18006d4ab*/
        ++v28; /*0x18006d4b0*/
        if ( v3 >= v54 ) /*0x18006d4b9*/
          return; /*0x18006d4b9*/
      }
      if ( *(_BYTE *)v37 != 99 ) /*0x18006d419*/
      {
        sub_180042450( /*0x18006d4a6*/
          a1: "afp-motion",
          a2: "%s unknown filter type(%d)",
          a3: "afp_motion_base_add_filter_property_array");
        goto LABEL_81; /*0x18006d4a6*/
      }
      switch ( v52 ) /*0x18006d426*/
      {
        case 0x7BAu: /*0x18006d426*/
          *(_BYTE *)(v37 + 4) = sub_180051210(a1: v46); /*0x18006d562*/
          goto LABEL_81; /*0x18006d566*/
        case 0x7BBu: /*0x18006d426*/
          *(_BYTE *)(v37 + 5) = sub_180051210(a1: v46); /*0x18006d48b*/
          goto LABEL_81; /*0x18006d48f*/
        case 0x7BCu: /*0x18006d426*/
          *(_BYTE *)(v37 + 3) = sub_180051210(a1: v46); /*0x18006d453*/
          goto LABEL_81; /*0x18006d457*/
        case 0x7BDu: /*0x18006d426*/
          *(_WORD *)(v37 + 6) = sub_180051210(a1: v46); /*0x18006d477*/
          goto LABEL_81; /*0x18006d47c*/
        default:
          break; /*0x18006d464*/
      }
      goto LABEL_70; /*0x18006d464*/
    }
  }
}
