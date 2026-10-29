float __fastcall sub_180057760(__int16 *a1)
{
  int v2; // r13d
  __int64 v3; // r12
  unsigned int v4; // eax
  const char *v5; // rsi
  int v6; // edi
  __int64 *v7; // r12
  int v8; // ecx
  int v9; // eax
  __int64 v10; // r9
  __int64 v11; // rdi
  const char *v12; // r12
  unsigned __int64 v13; // rdx
  int v14; // eax
  bool v15; // zf
  __int64 v16; // r14
  int v17; // r13d
  __int64 v18; // rax
  int v19; // r15d
  int v20; // r14d
  char *v21; // r13
  int v22; // eax
  int v23; // r13d
  int v24; // r9d
  const char *v25; // rax
  unsigned __int16 *v26; // rdx
  __int64 v27; // r13
  const char *v28; // rdi
  const char *v29; // rax
  int v30; // ecx
  unsigned __int16 v31; // ax
  __int64 v32; // rdi
  __int64 v33; // rcx
  const char *v34; // r12
  __int64 v35; // rcx
  const char *v36; // rax
  float result; // xmm0_4
  _BYTE *v38; // rbx
  int v39; // eax
  __int64 v40; // rbp
  unsigned int v41; // esi
  float v42; // xmm11_4
  _BYTE *v43; // rsi
  int v44; // eax
  __int64 v45; // rbx
  unsigned int v46; // edi
  float v47; // xmm11_4
  int v48; // ebx
  int v49; // r14d
  int v50; // eax
  __int64 v51; // r15
  int v52; // edi
  __int64 v53; // rax
  int v54; // r15d
  int v55; // r14d
  char *v56; // rdi
  int v57; // eax
  int v58; // ebx
  int v59; // r9d
  const char *v60; // rax
  int v61; // edi
  const char *v62; // rax
  unsigned __int64 v63; // rdi
  unsigned __int64 v64; // r13
  unsigned __int64 v65; // rdi
  int v66; // r13d
  const char *v67; // rax
  const char *v68; // r12
  unsigned int v69; // edi
  unsigned int v70; // eax
  int v71; // ebx
  const char *v72; // r13
  unsigned __int64 v73; // rax
  int v74; // ecx
  const char *v75; // r9
  __int64 v76; // rax
  __int64 v77; // rax
  __int64 v78; // [rsp+20h] [rbp-98h]
  int v79; // [rsp+20h] [rbp-98h]
  const char *v80; // [rsp+20h] [rbp-98h]
  const char *v81; // [rsp+30h] [rbp-88h] BYREF
  int v82; // [rsp+38h] [rbp-80h]
  unsigned __int16 v83; // [rsp+3Ch] [rbp-7Ch]
  char v84; // [rsp+3Eh] [rbp-7Ah]
  int v85; // [rsp+40h] [rbp-78h] BYREF
  int v86; // [rsp+44h] [rbp-74h] BYREF
  const char *v87; // [rsp+48h] [rbp-70h] BYREF
  int v88; // [rsp+50h] [rbp-68h]
  unsigned __int16 v89; // [rsp+54h] [rbp-64h]
  char v90; // [rsp+56h] [rbp-62h]

  v2 = *a1; /*0x180057779*/
  switch ( *a1 ) /*0x180057791*/
  {
    case 0: /*0x180057791*/
      return 0.0;
    case 2: /*0x180057791*/
      result = *(float *)&dword_180184424; /*0x1800581a5*/
      if ( *((_BYTE *)a1 + 8) == 0 ) /*0x1800581b1*/
        return 0.0; /*0x1800581b3*/
      return result; /*0x1800581b3*/
    case 3: /*0x180057791*/
    case 10: /*0x180057791*/
      return (float)*((int *)a1 + 2); /*0x18005819e*/
    case 4: /*0x180057791*/
      return (float)(int)*((_QWORD *)a1 + 1); /*0x180058188*/
    case 5: /*0x180057791*/
      return *((float *)a1 + 2); /*0x180058171*/
    case 6: /*0x180057791*/
      return *((double *)a1 + 1); /*0x18005815b*/
    case 7: /*0x180057791*/
      v43 = *((_BYTE **)a1 + 1); /*0x180058094*/
      if ( v43 == nullptr ) /*0x18005809b*/
        goto LABEL_119; /*0x18005809b*/
      for ( ; *v43 == 32; ++v43 ) /*0x1800580a0*/
        ; /*0x1800580a2*/
      v44 = avs2_core_222(a1: v43); /*0x1800580ad*/
      v45 = v44 - 1LL; /*0x1800580b5*/
      if ( v44 > 1LL && v43[v45] == 32 ) /*0x1800580be*/
      {
        v77 = sub_180040390(a1: v43); /*0x1800589bd*/
        v43 = (_BYTE *)v77; /*0x1800589c2*/
        do /*0x1800589d2*/
        {
          if ( *(_BYTE *)(v45 + v77) != 32 ) /*0x1800589c9*/
            break; /*0x1800589c9*/
          *(_BYTE *)(v45 + v77) = 0; /*0x1800589cb*/
          --v45; /*0x1800589cf*/
        }
        while ( v45 >= 0 ); /*0x1800589d2*/
        v46 = 1; /*0x1800589d4*/
      }
      else
      {
LABEL_119:
        v46 = 0; /*0x1800580c4*/
      }
      if ( (unsigned int)sub_1801215A0(a1: v43) != 0 ) /*0x1800580d0*/
      {
        v47 = (float)(int)avs2_core_245(a1: v43, a2: 0, a3: 10); /*0x1800580e2*/
      }
      else if ( (unsigned __int8)sub_180121610(a1: v43, a2: v46) != 0 ) /*0x1800580f8*/
      {
        v47 = avs2_core_247(a1: v43, a2: 0); /*0x180058115*/
      }
      else
      {
        LODWORD(v81) = 2143289344; /*0x1800580fa*/
        v47 = NAN; /*0x180058102*/
      }
      if ( v46 != 0 && v43 != nullptr ) /*0x180058121*/
        sub_180040D50(a1: v43); /*0x180058126*/
      return v47; /*0x180058145*/
    case 8: /*0x180057791*/
      LODWORD(v81) = 2139095040; /*0x180058075*/
      return INFINITY; /*0x180058093*/
    case 9: /*0x180057791*/
      if ( a1[1] < 256 ) /*0x180057f98*/
      {
        v38 = *((_BYTE **)a1 + 1); /*0x180057fb9*/
        if ( v38 == nullptr ) /*0x180057fc0*/
          v38 = (_BYTE *)((__int64 (*)(void))sub_1800D16F0)(); /*0x1800589b2*/
        if ( v38 == nullptr ) /*0x180057fc9*/
          goto LABEL_103; /*0x180057fc9*/
        for ( ; *v38 == 32; ++v38 ) /*0x180057fce*/
          ; /*0x180057fd0*/
        v39 = avs2_core_222(a1: v38); /*0x180057fdb*/
        v40 = v39 - 1LL; /*0x180057fe3*/
        if ( v39 > 1LL && v38[v40] == 32 ) /*0x180057fed*/
        {
          v76 = sub_180040390(a1: v38); /*0x18005898a*/
          v38 = (_BYTE *)v76; /*0x18005898f*/
          do /*0x1800589a1*/
          {
            if ( *(_BYTE *)(v40 + v76) != 32 ) /*0x180058997*/
              break; /*0x180058997*/
            *(_BYTE *)(v40 + v76) = 0; /*0x180058999*/
            --v40; /*0x18005899e*/
          }
          while ( v40 >= 0 ); /*0x1800589a1*/
          v41 = 1; /*0x1800589a3*/
        }
        else
        {
LABEL_103:
          v41 = 0; /*0x180057ff3*/
        }
        if ( (unsigned int)sub_1801215A0(a1: v38) != 0 ) /*0x180057fff*/
        {
          v42 = (float)(int)avs2_core_245(a1: v38, a2: 0, a3: 10); /*0x180058011*/
        }
        else if ( (unsigned __int8)sub_180121610(a1: v38, a2: v41) != 0 ) /*0x180058027*/
        {
          v42 = avs2_core_247(a1: v38, a2: 0); /*0x180058044*/
        }
        else
        {
          LODWORD(v81) = 2143289344; /*0x180058029*/
          v42 = NAN; /*0x180058031*/
        }
        if ( v41 != 0 && v38 != nullptr ) /*0x180058050*/
          sub_180040D50(a1: v38); /*0x180058055*/
        return v42; /*0x18005805a*/
      }
      else
      {
        LODWORD(v81) = 2143289344; /*0x180057f9a*/
        return NAN; /*0x180057fa2*/
      }
    case 11: /*0x180057791*/
      return (float)*((int *)a1 + 2); /*0x180057f8d*/
    case 12: /*0x180057791*/
    case 13: /*0x180057791*/
      v3 = *((_QWORD *)a1 + 1); /*0x180057798*/
      if ( v3 != 0 ) /*0x18005779f*/
      {
        v4 = ((_BYTE)word_180186528 + 1) & 7; /*0x1800577c1*/
        v5 = (const char *)&qword_1801862E0[8 * (unsigned __int64)(word_180186528 & 7) + 9]; /*0x1800577c4*/
        if ( ((0x3000u >> v2) & 1) != 0 ) /*0x1800577d3*/
        {
          v6 = a1[1]; /*0x1800577d9*/
          if ( v2 >= 12 && v2 <= 13 && ((v6 - 135) & (29 - v6)) < 0 ) /*0x180058257*/
          {
            if ( v6 != 93 ) /*0x180058260*/
            {
              v48 = 0; /*0x180058266*/
              word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180058268*/
              v49 = 0; /*0x18005826f*/
              v50 = *(__int16 *)(v3 + 8); /*0x180058272*/
              *(_DWORD *)v5 = 0; /*0x18005827a*/
              if ( v50 != v6 ) /*0x180058280*/
              {
                sub_180044B80(a1: v5, a2: &unk_1801467A0, a3: 64, a4: 0); /*0x180058295*/
                v48 = 1; /*0x18005829a*/
              }
              if ( v48 == 0 && (*(_BYTE *)(v3 + 4) & 0x10) != 0 ) /*0x1800582a9*/
              {
                v51 = *(_QWORD *)(v3 + 24); /*0x1800582ab*/
                if ( v51 != 0 ) /*0x1800582b3*/
                {
                  v52 = sub_180045570(a1: v5); /*0x1800582bd*/
                  v53 = sub_1800638F0(a1: v51); /*0x1800582c2*/
                  avs2_core_224(a1: &v5[v52], a2: v53, a3: (unsigned int)(64 - v52)); /*0x1800582d8*/
                  v49 = 1; /*0x1800582dd*/
                }
              }
              if ( v49 != 0 ) /*0x1800582e6*/
                goto LABEL_161; /*0x1800582e6*/
              v54 = sub_180045570(a1: v5); /*0x1800582f4*/
              v55 = *(__int16 *)(v3 + 8); /*0x1800582f7*/
              if ( v55 < 0 ) /*0x180058300*/
                goto LABEL_176; /*0x180058300*/
              if ( v55 < 30 ) /*0x18005830a*/
              {
                v56 = off_1801844E0[*(__int16 *)(v3 + 8)]; /*0x180058387*/
              }
              else if ( v55 < 164 ) /*0x180058313*/
              {
                v56 = (char *)sub_18004A200(a1: (unsigned int)(v55 + 134), a2: 87); /*0x180058370*/
                if ( v56 == nullptr ) /*0x180058376*/
                  goto LABEL_176; /*0x180058376*/
              }
              else
              {
                if ( v55 >= 351 ) /*0x18005831c*/
                {
                  if ( v55 < 354 ) /*0x180058325*/
                  {
                    v56 = (char *)sub_18004A200(a1: (unsigned int)v55, a2: 79); /*0x180058338*/
                    if ( v56 != nullptr ) /*0x18005833e*/
                      goto LABEL_160; /*0x18005833e*/
                  }
                  goto LABEL_176; /*0x18005833e*/
                }
                v56 = (char *)sub_18004A200(a1: (unsigned int)v55, a2: 67); /*0x180058352*/
                if ( v56 == nullptr ) /*0x180058358*/
                {
LABEL_176:
                  v63 = (unsigned __int64)(word_180186528 & 7) << 6; /*0x18005850e*/
                  word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180058535*/
                  v56 = (char *)&qword_1801862E0[9] + v63; /*0x180058541*/
                  avs2_core_258(a1: v56, a2: 64, a3: "(???:%d:%d)", 12, v55); /*0x180058549*/
                }
              }
LABEL_160:
              avs2_core_224(a1: &v5[v54], a2: v56, a3: (unsigned int)(64 - v54)); /*0x18005838f*/
LABEL_161:
              v86 = 0; /*0x1800583a7*/
              v57 = sub_180045570(a1: v5); /*0x1800583b2*/
              if ( v2 == 12 ) /*0x1800583bb*/
                LOBYTE(v86) = 80; /*0x1800583bd*/
              else
                LOBYTE(v86) = 79; /*0x1800583ca*/
              avs2_core_258( /*0x1800583fe*/
                a1: &v5[v57],
                a2: (unsigned int)(64 - v57),
                a3: "(%s:%d:%p)",
                (const char *)&v86,
                *(__int16 *)(v3 + 8),
                (const void *)v3);
              if ( v48 == 0 ) /*0x180058405*/
              {
                if ( (unsigned __int8)sub_1800D2B60(a1: (unsigned int)*(__int16 *)(v3 + 8), a2: 16) != 0 ) /*0x180058420*/
                {
                  if ( *(__int16 *)(v3 + 120) >= 0 ) /*0x18005842e*/
                  {
                    v58 = sub_180045570(a1: v5); /*0x18005843c*/
                    v59 = *(__int16 *)(v3 + 120); /*0x18005843e*/
                    if ( *(_WORD *)(v3 + 120) != 0 || (*(_DWORD *)(v3 + 32) & 0x4000000) != 0 ) /*0x180058452*/
                    {
                      v87 = nullptr; /*0x180058469*/
                      v88 = 0; /*0x180058472*/
                      v89 = 0; /*0x180058476*/
                      v90 = 0; /*0x18005847b*/
                      sub_1800B4FF0(a1: v3, a2: &v87); /*0x180058480*/
                      v60 = v87; /*0x180058485*/
                      if ( v87 == nullptr ) /*0x18005848d*/
                      {
                        v60 = (const char *)sub_1800D16F0(a1: v89); /*0x180058558*/
                        if ( v60 == nullptr ) /*0x180058560*/
                          v60 = (const char *)sub_1800D17E0(); /*0x180058566*/
                      }
                      v59 = *(__int16 *)(v3 + 120); /*0x18005849d*/
                      if ( v60 == nullptr ) /*0x1800584a3*/
                        v60 = (const char *)&unk_1801429B0; /*0x1800584a3*/
                    }
                    else
                    {
                      v60 = "_root"; /*0x180058454*/
                    }
                    avs2_core_258(a1: &v5[v58 - 1], a2: (unsigned int)(65 - v58), a3: ":%d:%s)", v59, v60); /*0x1800584c2*/
                  }
                }
                else if ( *(_WORD *)(v3 + 8) == 56 ) /*0x1800584d5*/
                {
                  v61 = sub_180045570(a1: v5); /*0x1800584e3*/
                  v62 = (const char *)sub_1800638F0(a1: v3); /*0x1800584e8*/
                  avs2_core_258(a1: &v5[v61], a2: (unsigned int)(64 - v61), a3: "(%s)", v62); /*0x180058504*/
                }
              }
              goto LABEL_91; /*0x1800584c7*/
            }
LABEL_15:
            word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x18005785a*/
            v7 = (__int64 *)sub_18004A200(a1: (unsigned int)(v6 + 134), a2: 87); /*0x180057871*/
            if ( v7 != nullptr ) /*0x180057877*/
              goto LABEL_18; /*0x180057877*/
            goto LABEL_136; /*0x180057877*/
          }
          if ( v6 >= 0 ) /*0x1800577ef*/
          {
            if ( v6 < 30 ) /*0x1800577f8*/
            {
              word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x18005787e*/
              v7 = (__int64 *)off_1801844E0[v6]; /*0x18005788f*/
              goto LABEL_18; /*0x18005788f*/
            }
            if ( v6 < 164 ) /*0x180057804*/
              goto LABEL_15; /*0x180057804*/
            if ( v6 < 351 ) /*0x18005780c*/
            {
              word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180057841*/
              v7 = (__int64 *)sub_18004A200(a1: (unsigned int)v6, a2: 67); /*0x18005784d*/
              if ( v7 != nullptr ) /*0x180057853*/
                goto LABEL_18; /*0x180057853*/
              goto LABEL_136; /*0x180057853*/
            }
            if ( v6 < 354 ) /*0x180057814*/
            {
              word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180057821*/
              v7 = (__int64 *)sub_18004A200(a1: (unsigned int)v6, a2: 79); /*0x18005782d*/
              if ( v7 != nullptr ) /*0x180057833*/
                goto LABEL_18; /*0x180057833*/
LABEL_136:
              LOBYTE(v4) = word_180186528; /*0x1800581e6*/
            }
          }
          word_180186528 = ((_BYTE)v4 + 1) & 7; /*0x1800581ed*/
          v7 = &qword_1801862E0[8 * (unsigned __int64)(v4 & 7) + 9]; /*0x180058217*/
          avs2_core_258(a1: v7, a2: 64, a3: "(???:%d:%d)", v2, v6); /*0x18005821f*/
LABEL_18:
          sub_180044B80(a1: v5, a2: v7, a3: 64, a4: 0); /*0x180057897*/
          v8 = a1[1]; /*0x1800578ab*/
          if ( ((v8 - 358) & (163 - v8)) >= 0 && v8 != 56 ) /*0x1800578cc*/
          {
            v9 = sub_180045570(a1: v5); /*0x1800578d1*/
            v10 = 79; /*0x1800578de*/
            if ( *a1 == 12 ) /*0x1800578f6*/
              v10 = 80; /*0x1800578f6*/
            LODWORD(v78) = a1[1]; /*0x180057908*/
            avs2_core_258(a1: &v5[v9], a2: (unsigned int)(64 - v9), a3: "(%c:%d:%p)", v10, v78, *((_QWORD *)a1 + 1)); /*0x180057912*/
          }
          if ( (unsigned int)avs2_core_230(a1: v5, a2: "(???", a3: 4) == 0 && *((_QWORD *)a1 + 1) == 0 ) /*0x18005793b*/
            avs2_core_258(a1: v5, a2: 64, a3: "null"); /*0x180058238*/
          goto LABEL_91; /*0x18005823d*/
        }
        if ( (unsigned int)v2 < 2 ) /*0x180057949*/
        {
          word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180057efb*/
          avs2_core_258(a1: v5, a2: 64, a3: "%s", (&off_1801845E0)[v2]); /*0x180057f23*/
          goto LABEL_91; /*0x180057f23*/
        }
        if ( v2 == 8 ) /*0x18005795d*/
        {
          v74 = *((_DWORD *)a1 + 2); /*0x1800587e7*/
          if ( v74 != 0 ) /*0x1800587ec*/
          {
            word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x1800588bc*/
            v75 = "-"; /*0x1800588db*/
            if ( v74 >= 0 ) /*0x1800588e5*/
              v75 = (const char *)&unk_1801429B0; /*0x1800588e5*/
            avs2_core_258(a1: v5, a2: 64, a3: "%s%s", v75, off_180184620[0]); /*0x1800588fa*/
            goto LABEL_91; /*0x1800588ff*/
          }
          goto LABEL_183; /*0x1800587ec*/
        }
        if ( v2 != 15 ) /*0x180057967*/
        {
          if ( v2 == 16 ) /*0x180057971*/
          {
            v11 = *(_QWORD *)(v3 + 8); /*0x180057977*/
            if ( v11 == 0 ) /*0x18005797f*/
            {
              word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180057983*/
              v12 = (const char *)sub_180053530(a1: 0); /*0x18005798f*/
              goto LABEL_60; /*0x180057992*/
            }
            v13 = (unsigned __int64)(((_BYTE)word_180186528 + 1) & 7) << 6; /*0x18005799b*/
            word_180186528 = ((_BYTE)v4 + 1) & 7; /*0x1800579a2*/
            v14 = 0; /*0x1800579a9*/
            v15 = (*(_BYTE *)(v11 + 4) & 0x10) == 0; /*0x1800579ab*/
            v12 = (char *)&qword_1801862E0[9] + v13; /*0x1800579af*/
            *(_DWORD *)v12 = 0; /*0x1800579b4*/
            if ( !v15 ) /*0x1800579bc*/
            {
              v16 = *(_QWORD *)(v11 + 24); /*0x1800579be*/
              if ( v16 != 0 ) /*0x1800579c5*/
              {
                v17 = sub_180045570(a1: (char *)&qword_1801862E0[9] + v13); /*0x1800579cf*/
                v18 = sub_1800638F0(a1: v16); /*0x1800579d5*/
                avs2_core_224(a1: &v12[v17], a2: v18, a3: (unsigned int)(64 - v17)); /*0x1800579ed*/
                v14 = 1; /*0x1800579f2*/
              }
            }
            if ( v14 != 0 ) /*0x1800579f9*/
            {
LABEL_49:
              v85 = 0; /*0x180057ab9*/
              v22 = sub_180045570(a1: v12); /*0x180057ac4*/
              v79 = *(__int16 *)(v11 + 8); /*0x180057ae7*/
              LOBYTE(v85) = 45; /*0x180057aec*/
              avs2_core_258( /*0x180057af6*/
                a1: &v12[v22],
                a2: (unsigned int)(64 - v22),
                a3: "(%s:%d:%p)",
                (const char *)&v85,
                v79,
                (const void *)v11);
              if ( (unsigned __int8)sub_1800D2B60(a1: (unsigned int)*(__int16 *)(v11 + 8), a2: 16) != 0 ) /*0x180057b0e*/
              {
                if ( *(__int16 *)(v11 + 120) >= 0 ) /*0x180057b1a*/
                {
                  v23 = sub_180045570(a1: v12); /*0x180057b28*/
                  v24 = *(__int16 *)(v11 + 120); /*0x180057b2b*/
                  if ( *(_WORD *)(v11 + 120) != 0 || (*(_DWORD *)(v11 + 32) & 0x4000000) != 0 ) /*0x180057b3c*/
                  {
                    v81 = nullptr; /*0x180057b54*/
                    v82 = 0; /*0x180057b5d*/
                    v83 = 0; /*0x180057b61*/
                    v84 = 0; /*0x180057b66*/
                    sub_1800B4FF0(a1: v11, a2: &v81); /*0x180057b6b*/
                    v25 = v81; /*0x180057b70*/
                    if ( v81 == nullptr ) /*0x180057b78*/
                    {
                      v25 = (const char *)sub_1800D16F0(a1: v83); /*0x1800585bc*/
                      if ( v25 == nullptr ) /*0x1800585c4*/
                        v25 = (const char *)sub_1800D17E0(); /*0x1800585ca*/
                    }
                    v24 = *(__int16 *)(v11 + 120); /*0x180057b88*/
                    if ( v25 == nullptr ) /*0x180057b8d*/
                      v25 = (const char *)&unk_1801429B0; /*0x180057b8d*/
                  }
                  else
                  {
                    v25 = "_root"; /*0x180057b3e*/
                  }
                  avs2_core_258(a1: &v12[v23 - 1], a2: (unsigned int)(65 - v23), a3: ":%d:%s)", v24, v25); /*0x180057baf*/
                }
              }
              else if ( *(_WORD *)(v11 + 8) == 56 ) /*0x180057bbd*/
              {
                v66 = sub_180045570(a1: v12); /*0x180058625*/
                v67 = (const char *)sub_1800638F0(a1: v11); /*0x18005862b*/
                avs2_core_258(a1: &v12[v66], a2: (unsigned int)(64 - v66), a3: "(%s)", v67); /*0x18005864a*/
              }
LABEL_60:
              v26 = *((unsigned __int16 **)a1 + 1); /*0x180057bc3*/
              v27 = *v26; /*0x180057bc7*/
              if ( *v26 < 0x1Eu ) /*0x180057bcf*/
              {
                v28 = off_1801844E0[v27]; /*0x180057c5c*/
                goto LABEL_71; /*0x180057c5c*/
              }
              if ( *v26 < 0xA4u ) /*0x180057bdc*/
              {
                v28 = (const char *)sub_18004A200(a1: (unsigned int)(v27 + 134), a2: 87); /*0x180057c43*/
                if ( v28 != nullptr ) /*0x180057c49*/
                {
                  v26 = *((unsigned __int16 **)a1 + 1); /*0x180057c4f*/
                  goto LABEL_71; /*0x180057c53*/
                }
              }
              else if ( *v26 < 0x15Fu ) /*0x180057be5*/
              {
                v28 = (const char *)sub_18004A200(a1: *v26, a2: 67); /*0x180057c20*/
                if ( v28 != nullptr ) /*0x180057c26*/
                {
                  v26 = *((unsigned __int16 **)a1 + 1); /*0x180057c2c*/
                  goto LABEL_71; /*0x180057c30*/
                }
              }
              else if ( *v26 < 0x162u ) /*0x180057bee*/
              {
                v28 = (const char *)sub_18004A200(a1: *v26, a2: 79); /*0x180057c01*/
                if ( v28 != nullptr ) /*0x180057c07*/
                {
                  v26 = *((unsigned __int16 **)a1 + 1); /*0x180057c0d*/
LABEL_71:
                  v29 = (const char *)sub_1800D16F0(a1: v26[1]); /*0x180057c64*/
                  avs2_core_258(a1: v5, a2: 64, a3: "%s.%s.%s", v12, v28, v29); /*0x180057c89*/
LABEL_91:
                  sub_180041A90(a1: "afp-variant", a2: "%s unknown variant type(%s)", "afp_variant_get_fv", v5); /*0x180057f28*/
                  return 0.0; /*0x180057f5f*/
                }
              }
              v65 = (unsigned __int64)(word_180186528 & 7) << 6; /*0x1800585e7*/
              word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x1800585fb*/
              v28 = (char *)&qword_1801862E0[9] + v65; /*0x180058607*/
              avs2_core_258(a1: v28, a2: 64, a3: "(???:%d:%d)", 12, v27); /*0x18005860f*/
              v26 = *((unsigned __int16 **)a1 + 1); /*0x180058614*/
              goto LABEL_71; /*0x180058618*/
            }
            v19 = sub_180045570(a1: v12); /*0x180057a07*/
            v20 = *(__int16 *)(v11 + 8); /*0x180057a0a*/
            if ( v20 < 0 ) /*0x180057a12*/
              goto LABEL_179; /*0x180057a12*/
            if ( v20 < 30 ) /*0x180057a1c*/
            {
              v21 = off_1801844E0[*(__int16 *)(v11 + 8)]; /*0x180057a99*/
            }
            else if ( v20 < 164 ) /*0x180057a25*/
            {
              v21 = (char *)sub_18004A200(a1: (unsigned int)(v20 + 134), a2: 87); /*0x180057a82*/
              if ( v21 == nullptr ) /*0x180057a88*/
                goto LABEL_179; /*0x180057a88*/
            }
            else
            {
              if ( v20 >= 351 ) /*0x180057a2e*/
              {
                if ( v20 < 354 ) /*0x180057a37*/
                {
                  v21 = (char *)sub_18004A200(a1: (unsigned int)v20, a2: 79); /*0x180057a4a*/
                  if ( v21 != nullptr ) /*0x180057a50*/
                    goto LABEL_48; /*0x180057a50*/
                }
                goto LABEL_179; /*0x180057a50*/
              }
              v21 = (char *)sub_18004A200(a1: (unsigned int)v20, a2: 67); /*0x180057a64*/
              if ( v21 == nullptr ) /*0x180057a6a*/
              {
LABEL_179:
                v64 = (unsigned __int64)(word_180186528 & 7) << 6; /*0x180058570*/
                word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180058599*/
                v21 = (char *)&qword_1801862E0[9] + v64; /*0x1800585a5*/
                avs2_core_258(a1: v21, a2: 64, a3: "(???:%d:%d)", 12, v20); /*0x1800585ad*/
              }
            }
LABEL_48:
            avs2_core_224(a1: &v12[v19], a2: v21, a3: (unsigned int)(64 - v19)); /*0x180057aa1*/
            goto LABEL_49; /*0x180057ab4*/
          }
          if ( v2 == 19 ) /*0x180057c97*/
          {
            v30 = *((_DWORD *)a1 + 2); /*0x180057c9d*/
            word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180057ca0*/
            if ( (((__int16)v30 - 351) & (163 - (__int16)v30)) >= 0 ) /*0x180057cc2*/
            {
              avs2_core_258(a1: v5, a2: 64, a3: "%s(???:%d)", off_180184678[0], v30); /*0x180057d2b*/
            }
            else
            {
              v31 = sub_1800D2840(); /*0x180057cc4*/
              v80 = (const char *)sub_1800D16F0(a1: v31); /*0x180057cd1*/
              avs2_core_258(a1: v5, a2: 64, a3: "%s(%s:%d)", (&off_1801845E0)[*a1], v80, *((_DWORD *)a1 + 2)); /*0x180057cff*/
            }
            goto LABEL_91; /*0x180057d04*/
          }
          if ( (unsigned int)v2 > 0x14 ) /*0x180057d38*/
          {
            word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180057d44*/
            avs2_core_258(a1: v5, a2: 64, a3: "(???:%d:%d:%#x)", v2, a1[1], *((_DWORD *)a1 + 2)); /*0x180057d6c*/
            goto LABEL_91; /*0x180057d71*/
          }
LABEL_183:
          word_180186528 = ((_BYTE)v4 + 1) & 7; /*0x180058654*/
          v68 = (const char *)&qword_1801862E0[8 * (unsigned __int64)v4 + 9]; /*0x180058672*/
          sub_18005C860(a1, a2: v68, a3: 64, a4: 0); /*0x18005867a*/
          v69 = *a1; /*0x18005867f*/
          v70 = 0x3000u >> *a1; /*0x180058689*/
          v71 = a1[1]; /*0x18005868b*/
          if ( (v70 & 1) != 0 ) /*0x180058691*/
          {
            if ( v71 < 0 ) /*0x180058699*/
              goto LABEL_198; /*0x180058699*/
            if ( v71 < 30 ) /*0x1800586a2*/
            {
              v72 = off_1801844E0[v71]; /*0x180058718*/
            }
            else if ( v71 < 164 ) /*0x1800586aa*/
            {
              v72 = (const char *)sub_18004A200(a1: (unsigned int)(v71 + 134), a2: 87); /*0x180058704*/
              if ( v72 == nullptr ) /*0x18005870a*/
                goto LABEL_198; /*0x18005870a*/
            }
            else
            {
              if ( v71 >= 351 ) /*0x1800586b2*/
              {
                if ( v71 < 354 ) /*0x1800586ba*/
                {
                  v72 = (const char *)sub_18004A200(a1: (unsigned int)v71, a2: 79); /*0x1800586cc*/
                  if ( v72 != nullptr ) /*0x1800586d2*/
                    goto LABEL_199; /*0x1800586d2*/
                }
                goto LABEL_198; /*0x1800586d2*/
              }
              v72 = (const char *)sub_18004A200(a1: (unsigned int)v71, a2: 67); /*0x1800586e6*/
              if ( v72 == nullptr ) /*0x1800586ec*/
              {
LABEL_198:
                v73 = (unsigned __int64)(word_180186528 & 7) << 6; /*0x18005873f*/
                word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180058767*/
                v72 = (char *)&qword_1801862E0[9] + v73; /*0x180058773*/
                avs2_core_258(a1: v72, a2: 64, a3: "(???:%d:%d)", v69, v71); /*0x18005877b*/
              }
            }
          }
          else
          {
            if ( v69 > 0x14 ) /*0x180058724*/
              goto LABEL_198; /*0x180058724*/
            v72 = (&off_1801845E0)[v69]; /*0x180058735*/
          }
LABEL_199:
          avs2_core_258(a1: v5, a2: 64, a3: "%s(%s)", v72, v68); /*0x180058780*/
          goto LABEL_91; /*0x18005879c*/
        }
        v32 = (unsigned __int16)a1[4]; /*0x180057d76*/
        if ( (unsigned __int16)a1[4] < 0x1Eu ) /*0x180057d7d*/
        {
          word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180057e03*/
          v34 = off_1801844E0[v32]; /*0x180057e11*/
        }
        else if ( (unsigned __int16)a1[4] < 0xA4u ) /*0x180057d89*/
        {
          word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180057dea*/
          v34 = (const char *)sub_18004A200(a1: (unsigned int)(v32 + 134), a2: 87); /*0x180057df6*/
          if ( v34 == nullptr ) /*0x180057dfc*/
            goto LABEL_200; /*0x180057dfc*/
        }
        else if ( (unsigned __int16)a1[4] < 0x15Fu ) /*0x180057d91*/
        {
          v35 = (unsigned __int16)a1[4]; /*0x180057dbf*/
          word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180057dc6*/
          v34 = (const char *)sub_18004A200(a1: v35, a2: 67); /*0x180057dd2*/
          if ( v34 == nullptr ) /*0x180057dd8*/
            goto LABEL_200; /*0x180057dd8*/
        }
        else
        {
          if ( (unsigned __int16)a1[4] >= 0x162u ) /*0x180057d99*/
          {
LABEL_201:
            word_180186528 = ((_BYTE)v4 + 1) & 7; /*0x1800587a8*/
            v34 = (const char *)&qword_1801862E0[8 * (unsigned __int64)(v4 & 7) + 9]; /*0x1800587d5*/
            avs2_core_258(a1: v34, a2: 64, a3: "(???:%d:%d)", 12, v32); /*0x1800587dd*/
            goto LABEL_89; /*0x1800587e2*/
          }
          v33 = (unsigned __int16)a1[4]; /*0x180057d9f*/
          word_180186528 = ((_BYTE)word_180186528 + 1) & 7; /*0x180057da6*/
          v34 = (const char *)sub_18004A200(a1: v33, a2: 79); /*0x180057db2*/
          if ( v34 == nullptr ) /*0x180057db8*/
          {
LABEL_200:
            LOBYTE(v4) = word_180186528; /*0x1800587a1*/
            goto LABEL_201; /*0x1800587a1*/
          }
        }
LABEL_89:
        v36 = (const char *)sub_1800D16F0(a1: (unsigned __int16)a1[5]); /*0x180057e19*/
        avs2_core_258(a1: v5, a2: 64, a3: "%s.%s", v34, v36); /*0x180057e39*/
        goto LABEL_91; /*0x180057e3e*/
      }
      return 0.0; /*0x180057f4f*/
    default:
      return NAN; /*0x1800581cf*/
  }
}
