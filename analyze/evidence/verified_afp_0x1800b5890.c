__int64 __fastcall sub_1800B5890(__int64 a1, int a2)
{
  __int64 v2; // r12
  int v4; // edi
  int v5; // ebp
  BOOL v6; // eax
  const char *v7; // rax
  const char *v8; // r13
  unsigned int v9; // ebx
  __int64 v10; // rbp
  const char *v11; // rax
  __int64 result; // rax
  __int16 v13; // ax
  bool v14; // zf
  __int64 i12; // rbx
  __int64 v16; // rbp
  __int64 v17; // rax
  __int64 v18; // rax
  int v19; // edx
  char v20; // al
  __int64 v21; // rax
  __int64 v22; // rax
  int v23; // edx
  __int64 v24; // rax
  __int64 v25; // rax
  int v26; // edx
  int v27; // r15d
  int v28; // esi
  int v29; // ebp
  int v30; // r8d
  int jj; // r15d
  __int64 i; // rax
  int j; // ebx
  __int64 v34; // rsi
  __int64 v35; // rbx
  __int64 v36; // r15
  int v37; // eax
  __int64 m; // rdi
  __int64 v39; // rax
  __int64 v40; // rdx
  __int64 v41; // rdi
  __int16 v42; // ax
  __int64 v43; // rax
  __int64 n; // r14
  __int64 v45; // rdx
  __int64 ii; // rax
  int v47; // edx
  __int64 v48; // rax
  __int64 v49; // rax
  int v50; // ebx
  int v51; // eax
  int v52; // edx
  int v53; // eax
  int i11; // r15d
  __int64 i5; // rax
  int i6; // ebx
  __int64 v57; // rbp
  __int64 v58; // rbx
  __int64 v59; // r15
  int v60; // eax
  __int64 i8; // rdi
  __int64 v62; // rax
  __int64 v63; // rdx
  __int64 v64; // rdi
  __int16 v65; // ax
  __int64 v66; // rax
  __int64 i9; // r14
  __int64 v68; // rdx
  __int64 i10; // rax
  int v70; // edx
  __int64 v71; // rax
  __int64 v72; // rax
  int v73; // ebx
  int i4; // r15d
  __int64 kk; // rax
  int mm; // ebx
  __int64 v77; // rbp
  __int64 v78; // rbx
  __int64 v79; // r15
  int v80; // eax
  __int64 i1; // rdi
  __int64 v82; // rax
  __int64 v83; // rdx
  __int64 v84; // rdi
  __int16 v85; // ax
  __int64 v86; // rax
  __int64 i2; // r14
  __int64 v88; // rdx
  __int64 i3; // rax
  __int64 v90; // rsi
  int v91; // eax
  int v92; // ebx
  const char *v93; // rax
  __int64 v94; // rsi
  int v95; // eax
  int v96; // ebx
  const char *v97; // rax
  int v98; // eax
  __int64 k; // rax
  __int64 v101; // rcx
  __int64 v102; // rcx
  int v103; // eax
  __int64 v104; // rsi
  int v105; // eax
  int v106; // ebx
  const char *v107; // rax
  int v108; // eax
  __int64 i7; // rax
  int v111; // eax
  __int64 nn; // rax
  __int64 v114; // rbx
  int v115; // eax
  int v116; // ebp
  const char *v117; // rax
  __int64 v118; // rsi
  int v119; // eax
  int v120; // ebx
  const char *v121; // rax
  BOOL v122; // eax
  int v123; // eax
  __int64 v124; // [rsp+30h] [rbp-88h] BYREF
  __int64 v125; // [rsp+38h] [rbp-80h]
  _QWORD v126[2]; // [rsp+40h] [rbp-78h] BYREF
  _QWORD v127[2]; // [rsp+50h] [rbp-68h] BYREF
  int v128; // [rsp+60h] [rbp-58h]
  int v129; // [rsp+68h] [rbp-50h]
  __int64 v130[9]; // [rsp+70h] [rbp-48h] BYREF

  v2 = a1; /*0x1800b58a0*/
  ++dword_180186610; /*0x1800b58a3*/
  v4 = 0; /*0x1800b58ac*/
  v5 = *(_DWORD *)(a1 + 32); /*0x1800b58ae*/
  if ( (v5 & 4) != 0 )
  {
    v123 = *(char *)(a1 + 110); /*0x1800b6b3a*/
    if ( v123 <= 0 ) /*0x1800b6b42*/
    {
      sub_180108AB0(a1); /*0x1800b6b6f*/
      return sub_180048E10(a1: v2); /*0x1800b6b77*/
    }
    else
    {
      *(_BYTE *)(a1 + 110) = v123 - 1; /*0x1800b6b46*/
      result = qword_180185B80; /*0x1800b6b4b*/
      if ( qword_180185B80 != 0 ) /*0x1800b6b55*/
        *(_BYTE *)(qword_180185B80 + 44) = 1; /*0x1800b6b57*/
    }
  }
  else
  {
    if ( (*(_DWORD *)(a1 + 36) & 0x80) == 0
      || (*(_WORD *)(a1 + 12) == 0
        ? (a1 = 0, v6 = ((*(_WORD *)(v2 + 6) | *(_WORD *)(v2 + 114)) & 0x210) == 0)
        : (v6 = false),
          !v6) )
    {
      v7 = *(const char **)(v2 + 72); /*0x1800b58e2*/
      v8 = v7; /*0x1800b58e7*/
      if ( v7 != nullptr ) /*0x1800b58ed*/
      {
        _mm_prefetch(v7, 1); /*0x1800b58ef*/
        _mm_prefetch(v7 + 64, 1); /*0x1800b58f2*/
        _mm_prefetch(v7 + 128, 1); /*0x1800b58f6*/
        _mm_prefetch(v7 + 192, 1); /*0x1800b58fd*/
      }
      if ( (v5 & 0x4000) != 0 ) /*0x1800b590a*/
      {
        v130[0] = 0; /*0x1800b591f*/
        if ( (unsigned int)sub_180093770(a1: v2, a2: 3, a3: 20, a4: v130) == 0 ) /*0x1800b592f*/
        {
          a1 = v130[0]; /*0x1800b5931*/
          if ( v130[0] != 0 ) /*0x1800b5939*/
            sub_18009F530(a1: v130[0]); /*0x1800b593b*/
        }
        v5 = *(_DWORD *)(v2 + 32); /*0x1800b5940*/
        v7 = *(const char **)(v2 + 72); /*0x1800b5945*/
        v4 = 1024; /*0x1800b594a*/
      }
      if ( v7 != nullptr && (v5 & 0x10000000) == 0 ) /*0x1800b595a*/
      {
        sub_1800B5540(a1: v2, (unsigned int)a2); /*0x1800b5962*/
        v5 = *(_DWORD *)(v2 + 32); /*0x1800b5967*/
      }
      v9 = v5 & 0xFFFFFF7F; /*0x1800b596e*/
      *(_DWORD *)(v2 + 32) = v5 & 0xFFFFFF7F; /*0x1800b5974*/
      if ( (v5 & 0x2000081) != 0 ) /*0x1800b597f*/
        goto LABEL_19; /*0x1800b597f*/
      if ( a2 < 0 ) /*0x1800b5984*/
      {
        if ( *(_WORD *)(v2 + 120) != 0 ) /*0x1800b6b0c*/
          goto LABEL_19; /*0x1800b6b0c*/
      }
      else if ( a2 == 0 ) /*0x1800b598a*/
      {
LABEL_19:
        v10 = *(_QWORD *)(v2 + 72); /*0x1800b5990*/
LABEL_20:
        if ( (v9 & 0x2000001) == 0 ) /*0x1800b59a2*/
          v4 |= 0x400u; /*0x1800b59a2*/
        if ( v10 != 0 ) /*0x1800b59a8*/
        {
          if ( v8 != nullptr ) /*0x1800b59ad*/
          {
            v11 = *((const char **)v8 + 11); /*0x1800b59af*/
            if ( v11 != nullptr ) /*0x1800b59b6*/
            {
              _mm_prefetch(v11, 1); /*0x1800b59b8*/
              _mm_prefetch(v11 + 64, 1); /*0x1800b59bb*/
              _mm_prefetch(v11 + 128, 1); /*0x1800b59bf*/
              _mm_prefetch(v11 + 192, 1); /*0x1800b59c6*/
            }
          }
          if ( (v9 & 0x10000000) != 0 ) /*0x1800b59d3*/
            sub_1800B5540(a1: v2, (unsigned int)a2); /*0x1800b59db*/
        }
        result = v4 | *(_WORD *)(v2 + 114) & 0xFBFFu; /*0x1800b59eb*/
        *(_WORD *)(v2 + 114) = v4 | *(_WORD *)(v2 + 114) & 0xFBFF; /*0x1800b59ed*/
        return result; /*0x1800b5a03*/
      }
      v16 = *(_QWORD *)(v2 + 64); /*0x1800b5a72*/
      if ( v16 != 0 )
      {
        if ( (*(_DWORD *)(v16 + 40) | *(__int16 *)(v16 + 120)) != 0 ) /*0x1800b5a87*/
        {
          if ( (*(_BYTE *)(v16 + 36) & 1) == 0 ) /*0x1800b5a8d*/
          {
LABEL_45:
            v20 = sub_1800BC9E0(a1: *(_QWORD *)(v16 + 64)); /*0x1800b5ad5*/
            v9 = *(_DWORD *)(v2 + 32); /*0x1800b5ade*/
            goto LABEL_54; /*0x1800b5ae3*/
          }
          if ( (*(_DWORD *)(v16 + 32) & 0x1000000) != 0 ) /*0x1800b5a96*/
          {
            v17 = *(_QWORD *)(v16 + 320); /*0x1800b5a9c*/
            if ( v17 != 0 ) /*0x1800b5aa6*/
            {
              v18 = v17 + 56; /*0x1800b5aa8*/
LABEL_40:
              if ( v18 != 0 ) /*0x1800b5aaf*/
              {
                v19 = *(_DWORD *)(*(_QWORD *)(v18 + 16) + 4LL); /*0x1800b5ab5*/
                goto LABEL_43; /*0x1800b5ab8*/
              }
            }
            v19 = 1; /*0x1800b5aba*/
LABEL_43:
            if ( *(unsigned __int16 *)(v16 + 118) >= v19 - 1 ) /*0x1800b5ac7*/
            {
              v9 = *(_DWORD *)(v2 + 32); /*0x1800b5ac9*/
              v20 = 1; /*0x1800b5ace*/
              goto LABEL_54; /*0x1800b5ad3*/
            }
            goto LABEL_45; /*0x1800b5ac7*/
          }
          v90 = *(_QWORD *)(v16 + 48); /*0x1800b65a9*/
          if ( v90 != 0 ) /*0x1800b65b0*/
          {
            v91 = (unsigned __int16)(*(_WORD *)v90 & 0x7FE) >> 1; /*0x1800b65ba*/
            if ( v91 == 121 ) /*0x1800b65bf*/
            {
              v18 = *(_QWORD *)(v90 + 16) + 24LL; /*0x1800b6628*/
              goto LABEL_40; /*0x1800b662c*/
            }
            if ( v91 != 123 && v91 != 126 ) /*0x1800b65c9*/
            {
              v92 = sub_1800293D0(a1: *(_QWORD *)(v16 + 48)); /*0x1800b65da*/
              v93 = (const char *)sub_1801211B0(a1: (unsigned __int16)(*(_WORD *)v90 & 0x7FE) >> 1); /*0x1800b65e7*/
              sub_180041A90( /*0x1800b6618*/
                a1: "afp-play",
                a2: "%s unknown tag(id=%d group=%d(%s)).",
                "afp_tag_get_tags",
                v92,
                (unsigned __int16)(*(_WORD *)v90 & 0x7FE) >> 1,
                v93);
              v18 = 0; /*0x1800b661d*/
              goto LABEL_40; /*0x1800b661f*/
            }
          }
          v18 = 0; /*0x1800b65cb*/
          goto LABEL_40; /*0x1800b65cd*/
        }
        if ( (*(_BYTE *)(v16 + 36) & 1) != 0
          || (qword_180185B80 == 0 ? (v122 = false) : (v122 = (*(_DWORD *)qword_180185B80 & 0x200) != 0), v122) )
        {
          if ( (*(_DWORD *)(v16 + 32) & 0x1000000) != 0 ) /*0x1800b5af6*/
          {
            v21 = *(_QWORD *)(v16 + 320); /*0x1800b5afc*/
            if ( v21 != 0 ) /*0x1800b5b06*/
            {
              v22 = v21 + 56; /*0x1800b5b08*/
LABEL_50:
              if ( v22 != 0 ) /*0x1800b5b0f*/
              {
                v23 = *(_DWORD *)(*(_QWORD *)(v22 + 16) + 4LL); /*0x1800b5b15*/
LABEL_53:
                v20 = *(unsigned __int16 *)(v16 + 118) >= v23 - 1; /*0x1800b5b1f*/
                goto LABEL_54; /*0x1800b5b27*/
              }
            }
            v23 = 1; /*0x1800b5b1a*/
            goto LABEL_53; /*0x1800b5b1a*/
          }
          v118 = *(_QWORD *)(v16 + 48); /*0x1800b6a50*/
          if ( v118 != 0 ) /*0x1800b6a57*/
          {
            v119 = (unsigned __int16)(*(_WORD *)v118 & 0x7FE) >> 1; /*0x1800b6a61*/
            if ( v119 == 121 ) /*0x1800b6a66*/
            {
              v22 = *(_QWORD *)(v118 + 16) + 24LL; /*0x1800b6ad4*/
              goto LABEL_50; /*0x1800b6ad8*/
            }
            if ( v119 != 123 && v119 != 126 ) /*0x1800b6a70*/
            {
              v120 = sub_1800293D0(a1: *(_QWORD *)(v16 + 48)); /*0x1800b6a81*/
              v121 = (const char *)sub_1801211B0(a1: (unsigned __int16)(*(_WORD *)v118 & 0x7FE) >> 1); /*0x1800b6a8e*/
              sub_180041A90( /*0x1800b6abf*/
                a1: "afp-play",
                a2: "%s unknown tag(id=%d group=%d(%s)).",
                "afp_tag_get_tags",
                v120,
                (unsigned __int16)(*(_WORD *)v118 & 0x7FE) >> 1,
                v121);
              v9 = *(_DWORD *)(v2 + 32); /*0x1800b6ac4*/
              v22 = 0; /*0x1800b6ac9*/
              goto LABEL_50; /*0x1800b6acb*/
            }
          }
          v22 = 0; /*0x1800b6a72*/
          goto LABEL_50; /*0x1800b6a74*/
        }
        v20 = 0; /*0x1800b5b29*/
      }
      else
      {
        v20 = 0; /*0x1800b5b29*/
      }
LABEL_54:
      if ( v20 != 0 ) /*0x1800b5b37*/
        goto LABEL_19; /*0x1800b5b37*/
      if ( (v9 & 0x1000000) != 0 ) /*0x1800b5b43*/
      {
        v24 = *(_QWORD *)(v2 + 320); /*0x1800b5b49*/
        if ( v24 == 0 ) /*0x1800b5b54*/
          goto LABEL_62; /*0x1800b5b54*/
        v25 = v24 + 56; /*0x1800b5b56*/
      }
      else
      {
        v114 = *(_QWORD *)(v2 + 48); /*0x1800b69c7*/
        if ( v114 == 0 ) /*0x1800b69cf*/
          goto LABEL_362; /*0x1800b69cf*/
        v115 = (unsigned __int16)(*(_WORD *)v114 & 0x7FE) >> 1; /*0x1800b69d9*/
        if ( v115 == 121 ) /*0x1800b69de*/
        {
          v25 = *(_QWORD *)(v114 + 16) + 24LL; /*0x1800b6a47*/
          goto LABEL_58; /*0x1800b6a4b*/
        }
        if ( v115 == 123 || v115 == 126 ) /*0x1800b69e8*/
        {
LABEL_362:
          v25 = 0; /*0x1800b69ea*/
        }
        else
        {
          v116 = sub_1800293D0(a1: *(_QWORD *)(v2 + 48)); /*0x1800b69f9*/
          v117 = (const char *)sub_1801211B0(a1: (unsigned __int16)(*(_WORD *)v114 & 0x7FE) >> 1); /*0x1800b6a06*/
          sub_180041A90( /*0x1800b6a37*/
            a1: "afp-play",
            a2: "%s unknown tag(id=%d group=%d(%s)).",
            "afp_tag_get_tags",
            v116,
            (unsigned __int16)(*(_WORD *)v114 & 0x7FE) >> 1,
            v117);
          v25 = 0; /*0x1800b6a3c*/
        }
      }
LABEL_58:
      if ( v25 != 0 ) /*0x1800b5b5d*/
      {
        v26 = *(_DWORD *)(*(_QWORD *)(v25 + 16) + 4LL); /*0x1800b5b63*/
        if ( v26 <= 1 ) /*0x1800b5b69*/
          *(_DWORD *)(v2 + 32) |= 0x2000000u; /*0x1800b5b6b*/
LABEL_63:
        v27 = *(unsigned __int16 *)(v2 + 118); /*0x1800b5b8b*/
        v28 = v26 - 1; /*0x1800b5b91*/
        v29 = a2 + v27; /*0x1800b5b94*/
        if ( a2 + v27 >= v26 - 1 && (*(_DWORD *)(v2 + 40) | *(__int16 *)(v2 + 120)) == 0 && qword_180185B80 != 0 ) /*0x1800b5bb3*/
          *(_DWORD *)qword_180185B80 |= 0x20000000u; /*0x1800b5bb5*/
        v30 = *(unsigned __int16 *)(v2 + 4) | 0x200; /*0x1800b5bc1*/
        *(_WORD *)(v2 + 4) |= 0x200u; /*0x1800b5bc8*/
        if ( v8 != nullptr ) /*0x1800b5bd1*/
        {
          v8 = *((const char **)v8 + 11); /*0x1800b5bd3*/
          if ( v8 != nullptr ) /*0x1800b5bda*/
          {
            _mm_prefetch(v8, 1); /*0x1800b5bdc*/
            _mm_prefetch(v8 + 64, 1); /*0x1800b5be1*/
            _mm_prefetch(v8 + 128, 1); /*0x1800b5be6*/
            _mm_prefetch(v8 + 192, 1); /*0x1800b5bee*/
          }
        }
        if ( v29 < v26 && v29 >= 0 ) /*0x1800b5c00*/
        {
          if ( a2 < 0 ) /*0x1800b5c09*/
          {
            if ( v27 != v29 ) /*0x1800b5c12*/
            {
              if ( v29 == 0 ) /*0x1800b5c16*/
                ++*(_WORD *)(v2 + 260); /*0x1800b5c18*/
              *(_WORD *)(v2 + 118) = v29; /*0x1800b5c21*/
            }
            if ( v29 <= v27 ) /*0x1800b5c2a*/
            {
              if ( v29 < v27 ) /*0x1800b5c6f*/
              {
                for ( i = *(_QWORD *)(v2 + 72); i != 0; i = *(_QWORD *)(i + 88) ) /*0x1800b5c7d*/
                  *(_DWORD *)(i + 32) = (((*(_DWORD *)(i + 32) & 2) != 0) << 15) | *(_DWORD *)(i + 32) & 0xFFFF7FFF; /*0x1800b5c94*/
                for ( j = 0; j < v29; ++j ) /*0x1800b5ca4*/
                  sub_1800A93B0(a1: v2, a2: j, a3: v29, a4: 0, a5: 1); /*0x1800b5cb6*/
                sub_1800A93B0(a1: v2, a2: v29, a3: v29, a4: 1, a5: 1); /*0x1800b5cd4*/
                v34 = 0; /*0x1800b5cd9*/
                v35 = *(_QWORD *)(v2 + 72); /*0x1800b5cdb*/
                v36 = 0; /*0x1800b5ce0*/
                if ( v35 != 0 ) /*0x1800b5ce6*/
                {
                  v129 = v4; /*0x1800b5cec*/
                  v128 = a2; /*0x1800b5cf0*/
                  do /*0x1800b5e00*/
                  {
                    v37 = *(_DWORD *)(v35 + 32); /*0x1800b5cf5*/
                    if ( (v37 & 0x8000) != 0 ) /*0x1800b5cfd*/
                    {
                      if ( *(int *)(v35 + 40) < 12288 ) /*0x1800b5d06*/
                        v36 = v35; /*0x1800b5d06*/
                      v35 = *(_QWORD *)(v35 + 88); /*0x1800b5d0a*/
                    }
                    else
                    {
                      if ( (v37 & 0x10000000) != 0 ) /*0x1800b5d18*/
                      {
                        v127[0] = 0; /*0x1800b66e3*/
                        v127[1] = 0; /*0x1800b66e8*/
                        v98 = sub_1800DC870(a1: v35, a2: v127, a3: 36, a4: 0); /*0x1800b66ed*/
                        if ( v98 != -17 && (v98 < -2999 || v98 > -1000) ) /*0x1800b6703*/
                        {
                          for ( k = v35; *(_QWORD *)(k + 64) != 0; k = *(_QWORD *)(k + 64) ) /*0x1800b671c*/
                          {
                            if ( (*(_DWORD *)(k + 32) & 0x200000) != 0 ) /*0x1800b6725*/
                              break; /*0x1800b6725*/
                          }
                          if ( (*(_DWORD *)(k + 36) & 0x80) == 0 ) /*0x1800b673b*/
                            sub_1800DBE40(a1: v35, a2: v127, a3: 2, a4: 0); /*0x1800b674e*/
                        }
                        sub_18005FF50(a1: v127); /*0x1800b6758*/
                      }
                      else
                      {
                        for ( m = *(_QWORD *)(v35 + 72); m != 0; m = *(_QWORD *)(m + 88) ) /*0x1800b5d25*/
                          sub_1800A7DF0(a1: m); /*0x1800b5d2a*/
                        sub_18008D7F0(a1: 2); /*0x1800b5d3d*/
                      }
                      v39 = *(_QWORD *)(v35 + 80); /*0x1800b5d42*/
                      v40 = *(_QWORD *)(v35 + 88); /*0x1800b5d49*/
                      v41 = v40; /*0x1800b5d4d*/
                      if ( v39 != 0 ) /*0x1800b5d50*/
                      {
                        *(_QWORD *)(v39 + 88) = v40; /*0x1800b5d52*/
                        v40 = *(_QWORD *)(v35 + 88); /*0x1800b5d56*/
                      }
                      else
                      {
                        *(_QWORD *)(v2 + 72) = v40; /*0x1800b5d5c*/
                      }
                      if ( v40 != 0 ) /*0x1800b5d64*/
                        *(_QWORD *)(v40 + 80) = v39; /*0x1800b5d66*/
                      *(_QWORD *)(v35 + 88) = v34; /*0x1800b5d6a*/
                      if ( v34 != 0 ) /*0x1800b5d71*/
                        *(_QWORD *)(v34 + 80) = v35; /*0x1800b5d73*/
                      v34 = v35; /*0x1800b5d77*/
                      if ( (*(_BYTE *)(v35 + 32) & 4) == 0 ) /*0x1800b5d7e*/
                      {
                        sub_180014130(a1: v35); /*0x1800b5d83*/
                        v42 = *(_WORD *)(v35 + 114) | 0x400; /*0x1800b5d94*/
                        *(_DWORD *)(v35 + 32) |= 0xCu; /*0x1800b5d99*/
                        *(_WORD *)(v35 + 114) = v42; /*0x1800b5d9d*/
                        sub_18008D610(a1: v35, a2: 1024); /*0x1800b5da1*/
                        v43 = qword_180185B80; /*0x1800b5da6*/
                        v14 = qword_180185B80 == 0; /*0x1800b5dad*/
                        *(_BYTE *)(v35 + 110) = 2; /*0x1800b5db0*/
                        if ( !v14 ) /*0x1800b5db4*/
                          *(_BYTE *)(v43 + 44) = 1; /*0x1800b5db6*/
                        for ( n = *(_QWORD *)(v35 + 72); n != 0; n = *(_QWORD *)(n + 88) ) /*0x1800b5dc1*/
                          sub_1800A7D80(a1: n); /*0x1800b5dc6*/
                      }
                      *(_DWORD *)(v35 + 40) = 12288; /*0x1800b5dd4*/
                      if ( (*(_DWORD *)(v2 + 32) & 0x10000000) != 0 && (*(_DWORD *)(v35 + 32) & 0x4000000) != 0 ) /*0x1800b5ded*/
                        sub_180127E60(a1: v2, a2: v35); /*0x1800b5df5*/
                      v35 = v41; /*0x1800b5dfa*/
                    }
                  }
                  while ( v35 != 0 ); /*0x1800b5e00*/
                  v4 = v129; /*0x1800b5e06*/
                  a2 = v128; /*0x1800b5e0d*/
                  if ( v34 != 0 ) /*0x1800b5e12*/
                  {
                    if ( v36 != 0 ) /*0x1800b5e17*/
                    {
                      v45 = *(_QWORD *)(v36 + 88); /*0x1800b5e1d*/
                      *(_QWORD *)(v36 + 88) = v34; /*0x1800b5e21*/
                      *(_QWORD *)(v34 + 80) = v36; /*0x1800b5e25*/
                    }
                    else
                    {
                      v45 = *(_QWORD *)(v2 + 72); /*0x1800b66b9*/
                      *(_QWORD *)(v2 + 72) = v34; /*0x1800b66be*/
                      *(_QWORD *)(v34 + 80) = 0; /*0x1800b66c3*/
                    }
                    for ( ii = *(_QWORD *)(v34 + 88); ii != 0; ii = *(_QWORD *)(ii + 88) ) /*0x1800b5e30*/
                      v34 = ii; /*0x1800b5e32*/
                    *(_QWORD *)(v34 + 88) = v45; /*0x1800b5e3e*/
                    if ( v45 != 0 ) /*0x1800b5e45*/
                      *(_QWORD *)(v45 + 80) = v34; /*0x1800b5e47*/
                  }
                }
                if ( *(_QWORD *)(v2 + 336) != 0 ) /*0x1800b5e56*/
                  sub_1800BF770(a1: v2, a2: (unsigned int)v29, a3: (~*(_WORD *)(v2 + 4) & 0x200) != 0); /*0x1800b5e71*/
                if ( (*(_DWORD *)(v2 + 36) & 0x800000) != 0 ) /*0x1800b5e7f*/
                  sub_18013FB10(a1: v2); /*0x1800b5e84*/
              }
            }
            else
            {
              for ( jj = v27 + 1; jj < v29; ++jj ) /*0x1800b5c32*/
                sub_1800A93B0(a1: v2, a2: jj, a3: v29, a4: 0, a5: 0); /*0x1800b5c45*/
              sub_1800A93B0(a1: v2, a2: v29, a3: v29, a4: 1, a5: 0); /*0x1800b5c65*/
            }
            sub_18006F5F0(); /*0x1800b5e89*/
            v10 = *(_QWORD *)(v2 + 72); /*0x1800b5e8e*/
            if ( v10 != 0 ) /*0x1800b5e96*/
            {
              while ( 1 ) /*0x1800b5e9c*/
              {
                v47 = *(_DWORD *)(v10 + 32); /*0x1800b5e9c*/
                if ( (v47 & 4) == 0 && (v47 & 0x2000000) == 0 ) /*0x1800b5eaa*/
                {
                  if ( (v47 & 0x1000000) == 0 ) /*0x1800b5eb2*/
                    break; /*0x1800b5eb2*/
                  v48 = *(_QWORD *)(v10 + 320); /*0x1800b5eb8*/
                  if ( v48 != 0 ) /*0x1800b5ec2*/
                  {
                    v49 = v48 + 56; /*0x1800b5ec4*/
                    goto LABEL_130; /*0x1800b5ec4*/
                  }
LABEL_136:
                  *(_DWORD *)(v10 + 32) = v47 | 0x2000000; /*0x1800b5f09*/
                }
LABEL_137:
                v10 = *(_QWORD *)(v10 + 88); /*0x1800b5f12*/
                if ( v10 == 0 ) /*0x1800b5f19*/
                  goto LABEL_280; /*0x1800b5f19*/
              }
              v94 = *(_QWORD *)(v10 + 48); /*0x1800b6631*/
              if ( v94 == 0 ) /*0x1800b6638*/
                goto LABEL_293; /*0x1800b6638*/
              v95 = (unsigned __int16)(*(_WORD *)v94 & 0x7FE) >> 1; /*0x1800b6642*/
              if ( v95 == 121 ) /*0x1800b6647*/
              {
                v49 = *(_QWORD *)(v94 + 16) + 24LL; /*0x1800b66b0*/
                goto LABEL_130; /*0x1800b66b4*/
              }
              if ( v95 == 123 || v95 == 126 ) /*0x1800b6651*/
              {
LABEL_293:
                v49 = 0; /*0x1800b6653*/
              }
              else
              {
                v96 = sub_1800293D0(a1: *(_QWORD *)(v10 + 48)); /*0x1800b6662*/
                v97 = (const char *)sub_1801211B0(a1: (unsigned __int16)(*(_WORD *)v94 & 0x7FE) >> 1); /*0x1800b666f*/
                sub_180041A90( /*0x1800b66a0*/
                  a1: "afp-play",
                  a2: "%s unknown tag(id=%d group=%d(%s)).",
                  "afp_tag_get_tags",
                  v96,
                  (unsigned __int16)(*(_WORD *)v94 & 0x7FE) >> 1,
                  v97);
                v49 = 0; /*0x1800b66a5*/
              }
LABEL_130:
              if ( v49 != 0 ) /*0x1800b5ecb*/
              {
                v50 = *(_DWORD *)(*(_QWORD *)(v49 + 16) + 4LL); /*0x1800b5ed1*/
                if ( v50 > 1 ) /*0x1800b5ed7*/
                {
                  if ( (*(unsigned __int16 *)(v2 + 118) - *(unsigned __int16 *)(v10 + 264)) % v50 != *(unsigned __int16 *)(v10 + 118) ) /*0x1800b5efa*/
                    sub_1800B50C0(a1: v10); /*0x1800b5eff*/
                }
                else
                {
                  *(_DWORD *)(v10 + 32) |= 0x2000000u; /*0x1800b5ed9*/
                }
                goto LABEL_137; /*0x1800b5ee0*/
              }
              v47 = *(_DWORD *)(v10 + 32); /*0x1800b5f06*/
              goto LABEL_136; /*0x1800b5f06*/
            }
            goto LABEL_331; /*0x1800b5e96*/
          }
          if ( v27 != v29 ) /*0x1800b5f23*/
          {
            if ( v29 == 0 ) /*0x1800b5f27*/
              ++*(_WORD *)(v2 + 260); /*0x1800b5f29*/
            *(_WORD *)(v2 + 118) = v29; /*0x1800b5f32*/
          }
          sub_1800A93B0(a1: v2, a2: v29, a3: v29, a4: 1, a5: 0); /*0x1800b5f4b*/
          goto LABEL_280; /*0x1800b5f50*/
        }
        v51 = 0; /*0x1800b5f55*/
        v9 = *(_DWORD *)(v2 + 32); /*0x1800b5f57*/
        v52 = 0; /*0x1800b5f5c*/
        if ( (v9 & 0x8000000) != 0 || (*(_BYTE *)(v2 + 36) & 3) != 0 ) /*0x1800b5f6c*/
          v51 = 1; /*0x1800b5f6e*/
        if ( v51 != 0 ) /*0x1800b5f75*/
          goto LABEL_158; /*0x1800b5f75*/
        if ( (*(_DWORD *)(v2 + 40) | *(__int16 *)(v2 + 120)) == 0 ) /*0x1800b5f82*/
        {
          v53 = *(_DWORD *)qword_180185B80; /*0x1800b5faa*/
          if ( (*(_DWORD *)(v2 + 36) & 0x100) != 0 ) /*0x1800b5fac*/
          {
            v52 = 1; /*0x1800b5fae*/
            goto LABEL_159; /*0x1800b5fb3*/
          }
          if ( (v53 & 0x2000) != 0 ) /*0x1800b5fba*/
          {
            v52 = 1; /*0x1800b5fbc*/
            goto LABEL_159; /*0x1800b5fc1*/
          }
          if ( (v53 & 0x600) == 0 && (v53 & 0x10) != 0 ) /*0x1800b5fcc*/
            goto LABEL_159; /*0x1800b5fcc*/
          sub_180013820(a1, a2: 0); /*0x1800b5fce*/
          v9 = *(_DWORD *)(v2 + 32); /*0x1800b5fd3*/
          LOWORD(v30) = *(_WORD *)(v2 + 4); /*0x1800b5fd8*/
LABEL_158:
          v10 = *(_QWORD *)(v2 + 72); /*0x1800b5fde*/
LABEL_281:
          *(_WORD *)(v2 + 4) = v30 & 0xFDFF; /*0x1800b6597*/
          goto LABEL_20; /*0x1800b65a4*/
        }
        if ( (*(_DWORD *)(v2 + 36) & 0x100) != 0 ) /*0x1800b5f8d*/
        {
          v52 = 1; /*0x1800b5f93*/
LABEL_159:
          if ( v29 >= 0 ) /*0x1800b5fec*/
            v28 = 0; /*0x1800b5fec*/
          if ( v52 == 0 ) /*0x1800b5ff1*/
          {
            if ( v27 != v28 ) /*0x1800b630b*/
            {
              if ( v28 == 0 ) /*0x1800b630f*/
                ++*(_WORD *)(v2 + 260); /*0x1800b6311*/
              *(_WORD *)(v2 + 118) = v28; /*0x1800b631a*/
            }
            if ( v28 <= v27 ) /*0x1800b6323*/
            {
              if ( v28 < v27 ) /*0x1800b6368*/
              {
                for ( kk = *(_QWORD *)(v2 + 72); kk != 0; kk = *(_QWORD *)(kk + 88) ) /*0x1800b6376*/
                  *(_DWORD *)(kk + 32) = (((*(_DWORD *)(kk + 32) & 2) != 0) << 15) | *(_DWORD *)(kk + 32) & 0xFFFF7FFF; /*0x1800b638d*/
                for ( mm = 0; mm < v28; ++mm ) /*0x1800b639d*/
                  sub_1800A93B0(a1: v2, a2: mm, a3: v28, a4: 0, a5: 1); /*0x1800b63af*/
                sub_1800A93B0(a1: v2, a2: v28, a3: v28, a4: 1, a5: 1); /*0x1800b63cd*/
                v77 = 0; /*0x1800b63d2*/
                v78 = *(_QWORD *)(v2 + 72); /*0x1800b63d4*/
                v79 = 0; /*0x1800b63d9*/
                if ( v78 != 0 ) /*0x1800b63df*/
                {
                  v129 = v4; /*0x1800b63e5*/
                  v128 = a2; /*0x1800b63e9*/
                  do /*0x1800b64f9*/
                  {
                    v80 = *(_DWORD *)(v78 + 32); /*0x1800b63ee*/
                    if ( (v80 & 0x8000) != 0 ) /*0x1800b63f6*/
                    {
                      if ( *(int *)(v78 + 40) < 12288 ) /*0x1800b63ff*/
                        v79 = v78; /*0x1800b63ff*/
                      v78 = *(_QWORD *)(v78 + 88); /*0x1800b6403*/
                    }
                    else
                    {
                      if ( (v80 & 0x10000000) != 0 ) /*0x1800b6411*/
                      {
                        v126[0] = 0; /*0x1800b6948*/
                        v126[1] = 0; /*0x1800b694d*/
                        v111 = sub_1800DC870(a1: v78, a2: v126, a3: 36, a4: 0); /*0x1800b6952*/
                        if ( v111 != -17 && (v111 < -2999 || v111 > -1000) ) /*0x1800b6968*/
                        {
                          for ( nn = v78; *(_QWORD *)(nn + 64) != 0; nn = *(_QWORD *)(nn + 64) ) /*0x1800b6981*/
                          {
                            if ( (*(_DWORD *)(nn + 32) & 0x200000) != 0 ) /*0x1800b698a*/
                              break; /*0x1800b698a*/
                          }
                          if ( (*(_DWORD *)(nn + 36) & 0x80) == 0 ) /*0x1800b69a0*/
                            sub_1800DBE40(a1: v78, a2: v126, a3: 2, a4: 0); /*0x1800b69b3*/
                        }
                        sub_18005FF50(a1: v126); /*0x1800b69bd*/
                      }
                      else
                      {
                        for ( i1 = *(_QWORD *)(v78 + 72); i1 != 0; i1 = *(_QWORD *)(i1 + 88) ) /*0x1800b641e*/
                          sub_1800A7DF0(a1: i1); /*0x1800b6423*/
                        sub_18008D7F0(a1: 2); /*0x1800b6436*/
                      }
                      v82 = *(_QWORD *)(v78 + 80); /*0x1800b643b*/
                      v83 = *(_QWORD *)(v78 + 88); /*0x1800b6442*/
                      v84 = v83; /*0x1800b6446*/
                      if ( v82 != 0 ) /*0x1800b6449*/
                      {
                        *(_QWORD *)(v82 + 88) = v83; /*0x1800b644b*/
                        v83 = *(_QWORD *)(v78 + 88); /*0x1800b644f*/
                      }
                      else
                      {
                        *(_QWORD *)(v2 + 72) = v83; /*0x1800b6455*/
                      }
                      if ( v83 != 0 ) /*0x1800b645d*/
                        *(_QWORD *)(v83 + 80) = v82; /*0x1800b645f*/
                      *(_QWORD *)(v78 + 88) = v77; /*0x1800b6463*/
                      if ( v77 != 0 ) /*0x1800b646a*/
                        *(_QWORD *)(v77 + 80) = v78; /*0x1800b646c*/
                      v77 = v78; /*0x1800b6470*/
                      if ( (*(_BYTE *)(v78 + 32) & 4) == 0 ) /*0x1800b6477*/
                      {
                        sub_180014130(a1: v78); /*0x1800b647c*/
                        v85 = *(_WORD *)(v78 + 114) | 0x400; /*0x1800b648d*/
                        *(_DWORD *)(v78 + 32) |= 0xCu; /*0x1800b6492*/
                        *(_WORD *)(v78 + 114) = v85; /*0x1800b6496*/
                        sub_18008D610(a1: v78, a2: 1024); /*0x1800b649a*/
                        v86 = qword_180185B80; /*0x1800b649f*/
                        v14 = qword_180185B80 == 0; /*0x1800b64a6*/
                        *(_BYTE *)(v78 + 110) = 2; /*0x1800b64a9*/
                        if ( !v14 ) /*0x1800b64ad*/
                          *(_BYTE *)(v86 + 44) = 1; /*0x1800b64af*/
                        for ( i2 = *(_QWORD *)(v78 + 72); i2 != 0; i2 = *(_QWORD *)(i2 + 88) ) /*0x1800b64ba*/
                          sub_1800A7D80(a1: i2); /*0x1800b64bf*/
                      }
                      *(_DWORD *)(v78 + 40) = 12288; /*0x1800b64cd*/
                      if ( (*(_DWORD *)(v2 + 32) & 0x10000000) != 0 && (*(_DWORD *)(v78 + 32) & 0x4000000) != 0 ) /*0x1800b64e6*/
                        sub_180127E60(a1: v2, a2: v78); /*0x1800b64ee*/
                      v78 = v84; /*0x1800b64f3*/
                    }
                  }
                  while ( v78 != 0 ); /*0x1800b64f9*/
                  v4 = v129; /*0x1800b64ff*/
                  a2 = v128; /*0x1800b6506*/
                  if ( v77 != 0 ) /*0x1800b650b*/
                  {
                    if ( v79 != 0 ) /*0x1800b6510*/
                    {
                      v88 = *(_QWORD *)(v79 + 88); /*0x1800b6516*/
                      *(_QWORD *)(v79 + 88) = v77; /*0x1800b651a*/
                      *(_QWORD *)(v77 + 80) = v79; /*0x1800b651e*/
                    }
                    else
                    {
                      v88 = *(_QWORD *)(v2 + 72); /*0x1800b691e*/
                      *(_QWORD *)(v2 + 72) = v77; /*0x1800b6923*/
                      *(_QWORD *)(v77 + 80) = 0; /*0x1800b6928*/
                    }
                    for ( i3 = *(_QWORD *)(v77 + 88); i3 != 0; i3 = *(_QWORD *)(i3 + 88) ) /*0x1800b6529*/
                      v77 = i3; /*0x1800b652b*/
                    *(_QWORD *)(v77 + 88) = v88; /*0x1800b6537*/
                    if ( v88 != 0 ) /*0x1800b653e*/
                      *(_QWORD *)(v88 + 80) = v77; /*0x1800b6540*/
                  }
                }
                if ( *(_QWORD *)(v2 + 336) != 0 ) /*0x1800b654f*/
                  sub_1800BF770(a1: v2, a2: (unsigned int)v28, a3: (~*(_WORD *)(v2 + 4) & 0x200) != 0); /*0x1800b656a*/
                if ( (*(_DWORD *)(v2 + 36) & 0x800000) != 0 ) /*0x1800b6578*/
                  sub_18013FB10(a1: v2); /*0x1800b657d*/
              }
            }
            else
            {
              for ( i4 = v27 + 1; i4 < v28; ++i4 ) /*0x1800b632b*/
                sub_1800A93B0(a1: v2, a2: i4, a3: v28, a4: 0, a5: 0); /*0x1800b633e*/
              sub_1800A93B0(a1: v2, a2: v28, a3: v28, a4: 1, a5: 0); /*0x1800b635e*/
            }
            sub_18006F5F0(); /*0x1800b6582*/
LABEL_280:
            v10 = *(_QWORD *)(v2 + 72); /*0x1800b6587*/
            v9 = *(_DWORD *)(v2 + 32); /*0x1800b658c*/
            LOWORD(v30) = *(_WORD *)(v2 + 4); /*0x1800b6591*/
            goto LABEL_281; /*0x1800b6591*/
          }
          if ( v27 != v28 ) /*0x1800b5ffa*/
          {
            if ( v28 == 0 ) /*0x1800b5ffe*/
              ++*(_WORD *)(v2 + 260); /*0x1800b6000*/
            *(_WORD *)(v2 + 118) = v28; /*0x1800b6009*/
          }
          if ( v28 <= v27 ) /*0x1800b6012*/
          {
            if ( v28 < v27 ) /*0x1800b6057*/
            {
              for ( i5 = *(_QWORD *)(v2 + 72); i5 != 0; i5 = *(_QWORD *)(i5 + 88) ) /*0x1800b6065*/
                *(_DWORD *)(i5 + 32) = (((*(_DWORD *)(i5 + 32) & 2) != 0) << 15) | *(_DWORD *)(i5 + 32) & 0xFFFF7FFF; /*0x1800b607c*/
              for ( i6 = 0; i6 < v28; ++i6 ) /*0x1800b608c*/
                sub_1800A93B0(a1: v2, a2: i6, a3: v28, a4: 0, a5: 1); /*0x1800b609e*/
              sub_1800A93B0(a1: v2, a2: v28, a3: v28, a4: 1, a5: 1); /*0x1800b60bc*/
              v57 = 0; /*0x1800b60c1*/
              v58 = *(_QWORD *)(v2 + 72); /*0x1800b60c3*/
              v59 = 0; /*0x1800b60c8*/
              if ( v58 != 0 ) /*0x1800b60ce*/
              {
                v129 = v4; /*0x1800b60d4*/
                v128 = a2; /*0x1800b60d8*/
                do /*0x1800b61e8*/
                {
                  v60 = *(_DWORD *)(v58 + 32); /*0x1800b60dd*/
                  if ( (v60 & 0x8000) != 0 ) /*0x1800b60e5*/
                  {
                    if ( *(int *)(v58 + 40) < 12288 ) /*0x1800b60ee*/
                      v59 = v58; /*0x1800b60ee*/
                    v58 = *(_QWORD *)(v58 + 88); /*0x1800b60f2*/
                  }
                  else
                  {
                    if ( (v60 & 0x10000000) != 0 ) /*0x1800b6100*/
                    {
                      v124 = 0; /*0x1800b689f*/
                      v125 = 0; /*0x1800b68a4*/
                      v108 = sub_1800DC870(a1: v58, a2: &v124, a3: 36, a4: 0); /*0x1800b68a9*/
                      if ( v108 != -17 && (v108 < -2999 || v108 > -1000) ) /*0x1800b68bf*/
                      {
                        for ( i7 = v58; *(_QWORD *)(i7 + 64) != 0; i7 = *(_QWORD *)(i7 + 64) ) /*0x1800b68d8*/
                        {
                          if ( (*(_DWORD *)(i7 + 32) & 0x200000) != 0 ) /*0x1800b68e1*/
                            break; /*0x1800b68e1*/
                        }
                        if ( (*(_DWORD *)(i7 + 36) & 0x80) == 0 ) /*0x1800b68f7*/
                          sub_1800DBE40(a1: v58, a2: &v124, a3: 2, a4: 0); /*0x1800b690a*/
                      }
                      sub_18005FF50(a1: &v124); /*0x1800b6914*/
                    }
                    else
                    {
                      for ( i8 = *(_QWORD *)(v58 + 72); i8 != 0; i8 = *(_QWORD *)(i8 + 88) ) /*0x1800b610d*/
                        sub_1800A7DF0(a1: i8); /*0x1800b6112*/
                      sub_18008D7F0(a1: 2); /*0x1800b6125*/
                    }
                    v62 = *(_QWORD *)(v58 + 80); /*0x1800b612a*/
                    v63 = *(_QWORD *)(v58 + 88); /*0x1800b6131*/
                    v64 = v63; /*0x1800b6135*/
                    if ( v62 != 0 ) /*0x1800b6138*/
                    {
                      *(_QWORD *)(v62 + 88) = v63; /*0x1800b613a*/
                      v63 = *(_QWORD *)(v58 + 88); /*0x1800b613e*/
                    }
                    else
                    {
                      *(_QWORD *)(v2 + 72) = v63; /*0x1800b6144*/
                    }
                    if ( v63 != 0 ) /*0x1800b614c*/
                      *(_QWORD *)(v63 + 80) = v62; /*0x1800b614e*/
                    *(_QWORD *)(v58 + 88) = v57; /*0x1800b6152*/
                    if ( v57 != 0 ) /*0x1800b6159*/
                      *(_QWORD *)(v57 + 80) = v58; /*0x1800b615b*/
                    v57 = v58; /*0x1800b615f*/
                    if ( (*(_BYTE *)(v58 + 32) & 4) == 0 ) /*0x1800b6166*/
                    {
                      sub_180014130(a1: v58); /*0x1800b616b*/
                      v65 = *(_WORD *)(v58 + 114) | 0x400; /*0x1800b617c*/
                      *(_DWORD *)(v58 + 32) |= 0xCu; /*0x1800b6181*/
                      *(_WORD *)(v58 + 114) = v65; /*0x1800b6185*/
                      sub_18008D610(a1: v58, a2: 1024); /*0x1800b6189*/
                      v66 = qword_180185B80; /*0x1800b618e*/
                      v14 = qword_180185B80 == 0; /*0x1800b6195*/
                      *(_BYTE *)(v58 + 110) = 2; /*0x1800b6198*/
                      if ( !v14 ) /*0x1800b619c*/
                        *(_BYTE *)(v66 + 44) = 1; /*0x1800b619e*/
                      for ( i9 = *(_QWORD *)(v58 + 72); i9 != 0; i9 = *(_QWORD *)(i9 + 88) ) /*0x1800b61a9*/
                        sub_1800A7D80(a1: i9); /*0x1800b61ae*/
                    }
                    *(_DWORD *)(v58 + 40) = 12288; /*0x1800b61bc*/
                    if ( (*(_DWORD *)(v2 + 32) & 0x10000000) != 0 && (*(_DWORD *)(v58 + 32) & 0x4000000) != 0 ) /*0x1800b61d5*/
                      sub_180127E60(a1: v2, a2: v58); /*0x1800b61dd*/
                    v58 = v64; /*0x1800b61e2*/
                  }
                }
                while ( v58 != 0 ); /*0x1800b61e8*/
                v4 = v129; /*0x1800b61ee*/
                a2 = v128; /*0x1800b61f5*/
                if ( v57 != 0 ) /*0x1800b61fa*/
                {
                  if ( v59 != 0 ) /*0x1800b61ff*/
                  {
                    v68 = *(_QWORD *)(v59 + 88); /*0x1800b6205*/
                    *(_QWORD *)(v59 + 88) = v57; /*0x1800b6209*/
                    *(_QWORD *)(v57 + 80) = v59; /*0x1800b620d*/
                  }
                  else
                  {
                    v68 = *(_QWORD *)(v2 + 72); /*0x1800b6875*/
                    *(_QWORD *)(v2 + 72) = v57; /*0x1800b687a*/
                    *(_QWORD *)(v57 + 80) = 0; /*0x1800b687f*/
                  }
                  for ( i10 = *(_QWORD *)(v57 + 88); i10 != 0; i10 = *(_QWORD *)(i10 + 88) ) /*0x1800b6218*/
                    v57 = i10; /*0x1800b621a*/
                  *(_QWORD *)(v57 + 88) = v68; /*0x1800b6226*/
                  if ( v68 != 0 ) /*0x1800b622d*/
                    *(_QWORD *)(v68 + 80) = v57; /*0x1800b622f*/
                }
              }
              if ( *(_QWORD *)(v2 + 336) != 0 ) /*0x1800b623e*/
                sub_1800BF770(a1: v2, a2: (unsigned int)v28, a3: (~*(_WORD *)(v2 + 4) & 0x200) != 0); /*0x1800b6259*/
              if ( (*(_DWORD *)(v2 + 36) & 0x800000) != 0 ) /*0x1800b6267*/
                sub_18013FB10(a1: v2); /*0x1800b626c*/
            }
          }
          else
          {
            for ( i11 = v27 + 1; i11 < v28; ++i11 ) /*0x1800b601a*/
              sub_1800A93B0(a1: v2, a2: i11, a3: v28, a4: 0, a5: 0); /*0x1800b602d*/
            sub_1800A93B0(a1: v2, a2: v28, a3: v28, a4: 1, a5: 0); /*0x1800b604d*/
          }
          sub_18006F5F0(); /*0x1800b6271*/
          v10 = *(_QWORD *)(v2 + 72); /*0x1800b6276*/
          if ( v10 == 0 ) /*0x1800b627e*/
          {
LABEL_331:
            v9 = *(_DWORD *)(v2 + 32); /*0x1800b6865*/
            LOWORD(v30) = *(_WORD *)(v2 + 4); /*0x1800b686a*/
            goto LABEL_281; /*0x1800b6870*/
          }
          while ( 1 ) /*0x1800b6284*/
          {
            v70 = *(_DWORD *)(v10 + 32); /*0x1800b6284*/
            if ( (v70 & 4) == 0 && (v70 & 0x2000000) == 0 ) /*0x1800b6292*/
            {
              if ( (v70 & 0x1000000) == 0 ) /*0x1800b629a*/
                break; /*0x1800b629a*/
              v71 = *(_QWORD *)(v10 + 320); /*0x1800b62a0*/
              if ( v71 != 0 ) /*0x1800b62aa*/
              {
                v72 = v71 + 56; /*0x1800b62ac*/
                goto LABEL_219; /*0x1800b62ac*/
              }
LABEL_225:
              *(_DWORD *)(v10 + 32) = v70 | 0x2000000; /*0x1800b62f1*/
            }
LABEL_226:
            v10 = *(_QWORD *)(v10 + 88); /*0x1800b62fa*/
            if ( v10 == 0 ) /*0x1800b6301*/
              goto LABEL_280; /*0x1800b6301*/
          }
          v104 = *(_QWORD *)(v10 + 48); /*0x1800b67dd*/
          if ( v104 == 0 ) /*0x1800b67e4*/
            goto LABEL_328; /*0x1800b67e4*/
          v105 = (unsigned __int16)(*(_WORD *)v104 & 0x7FE) >> 1; /*0x1800b67ee*/
          if ( v105 == 121 ) /*0x1800b67f3*/
          {
            v72 = *(_QWORD *)(v104 + 16) + 24LL; /*0x1800b685c*/
            goto LABEL_219; /*0x1800b6860*/
          }
          if ( v105 == 123 || v105 == 126 ) /*0x1800b67fd*/
          {
LABEL_328:
            v72 = 0; /*0x1800b67ff*/
          }
          else
          {
            v106 = sub_1800293D0(a1: *(_QWORD *)(v10 + 48)); /*0x1800b680e*/
            v107 = (const char *)sub_1801211B0(a1: (unsigned __int16)(*(_WORD *)v104 & 0x7FE) >> 1); /*0x1800b681b*/
            sub_180041A90( /*0x1800b684c*/
              a1: "afp-play",
              a2: "%s unknown tag(id=%d group=%d(%s)).",
              "afp_tag_get_tags",
              v106,
              (unsigned __int16)(*(_WORD *)v104 & 0x7FE) >> 1,
              v107);
            v72 = 0; /*0x1800b6851*/
          }
LABEL_219:
          if ( v72 != 0 ) /*0x1800b62b3*/
          {
            v73 = *(_DWORD *)(*(_QWORD *)(v72 + 16) + 4LL); /*0x1800b62b9*/
            if ( v73 > 1 ) /*0x1800b62bf*/
            {
              if ( (*(unsigned __int16 *)(v2 + 118) - *(unsigned __int16 *)(v10 + 264)) % v73 != *(unsigned __int16 *)(v10 + 118) ) /*0x1800b62e2*/
                sub_1800B50C0(a1: v10); /*0x1800b62e7*/
            }
            else
            {
              *(_DWORD *)(v10 + 32) |= 0x2000000u; /*0x1800b62c1*/
            }
            goto LABEL_226; /*0x1800b62c8*/
          }
          v70 = *(_DWORD *)(v10 + 32); /*0x1800b62ee*/
          goto LABEL_225; /*0x1800b62ee*/
        }
        v101 = *(_QWORD *)(v2 + 64); /*0x1800b6762*/
        if ( v101 != 0 ) /*0x1800b676a*/
        {
          if ( (*(_DWORD *)(v101 + 40) | *(__int16 *)(v101 + 120)) != 0 ) /*0x1800b6773*/
          {
            if ( (*(_BYTE *)(v101 + 36) & 2) == 0 ) /*0x1800b6779*/
            {
              v102 = *(_QWORD *)(v101 + 64); /*0x1800b677b*/
              LODWORD(v124) = 0; /*0x1800b677f*/
              LODWORD(v125) = v30; /*0x1800b6783*/
              LOBYTE(v103) = sub_1800BC980(a1: v102); /*0x1800b6788*/
              LOWORD(v30) = v125; /*0x1800b678d*/
              v52 = 0; /*0x1800b6792*/
              goto LABEL_315; /*0x1800b6796*/
            }
            goto LABEL_314; /*0x1800b6779*/
          }
          if ( (*(_BYTE *)(v101 + 36) & 2) != 0 ) /*0x1800b679c*/
          {
LABEL_314:
            LOBYTE(v103) = 1; /*0x1800b679e*/
            goto LABEL_315; /*0x1800b679e*/
          }
          v103 = qword_180185B80 != 0 && (*(_DWORD *)qword_180185B80 & 0x400) != 0; /*0x1800b67c6*/
          if ( v103 != 0 ) /*0x1800b67d4*/
            LOBYTE(v103) = 1; /*0x1800b67d4*/
        }
        else
        {
          LOBYTE(v103) = 0; /*0x1800b67d9*/
        }
LABEL_315:
        if ( (_BYTE)v103 == 0 ) /*0x1800b67a8*/
          goto LABEL_159; /*0x1800b67a8*/
        goto LABEL_158; /*0x1800b67a8*/
      }
      v9 = *(_DWORD *)(v2 + 32); /*0x1800b5b76*/
LABEL_62:
      v26 = 1; /*0x1800b5b7b*/
      *(_DWORD *)(v2 + 32) = v9 | 0x2000000; /*0x1800b5b86*/
      goto LABEL_63; /*0x1800b5b86*/
    }
    sub_180014130(a1: v2); /*0x1800b5a07*/
    v13 = *(_WORD *)(v2 + 114) | 0x400; /*0x1800b5a1a*/
    *(_DWORD *)(v2 + 32) |= 0xCu; /*0x1800b5a1f*/
    *(_WORD *)(v2 + 114) = v13; /*0x1800b5a25*/
    sub_18008D610(a1: v2, a2: 1024); /*0x1800b5a2b*/
    result = qword_180185B80; /*0x1800b5a30*/
    v14 = qword_180185B80 == 0; /*0x1800b5a37*/
    *(_BYTE *)(v2 + 110) = 2; /*0x1800b5a3a*/
    if ( !v14 ) /*0x1800b5a40*/
      *(_BYTE *)(result + 44) = 1; /*0x1800b5a42*/
    for ( i12 = *(_QWORD *)(v2 + 72); i12 != 0; i12 = *(_QWORD *)(i12 + 88) ) /*0x1800b5a4e*/
      result = sub_1800A7D80(a1: i12); /*0x1800b5a53*/
  }
  return result; /*0x1800b59f3*/
}
