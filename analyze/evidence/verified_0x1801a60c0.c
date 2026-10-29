__int64 __fastcall sub_1801A60C0(__int64 *a1, __int64 a2, unsigned __int8 a3, __int64 a4)
{
  unsigned __int8 v4; // r15
  unsigned int *v5; // rdi
  int v6; // r14d
  unsigned int v8; // ebp
  char v9; // al
  __int64 v10; // rax
  int v11; // ebx
  int v12; // ecx
  char v13; // al
  bool v14; // r12
  unsigned int v15; // eax
  _DWORD *v16; // rax
  int v17; // eax
  unsigned __int8 v18; // r14
  unsigned __int8 v19; // cl
  __int64 v21; // rdx
  __int64 v22; // rcx
  __int128 *v23; // rax
  __int64 v24; // rdi
  __int64 v25; // rbx
  CLibXact *Instance; // rax
  __int64 v27; // rax
  __int64 *v28; // rbp
  int v29; // edi
  __int64 *v30; // rbx
  int v31; // ecx
  int v32; // eax
  int v33; // ecx
  int v34; // eax
  __int64 v35; // rax
  __int64 v36; // rax
  int v37; // ebx
  int v38; // ecx
  int v39; // eax
  __int64 v40; // rdx
  int v41; // eax
  __int64 v42; // rax
  int v43; // ebx
  int v44; // ecx
  __int64 v45; // rax
  __int64 v46; // [rsp+60h] [rbp-58h]
  unsigned int v47; // [rsp+C0h] [rbp+8h]
  unsigned int *v48; // [rsp+C8h] [rbp+10h]
  __int64 v50; // [rsp+D8h] [rbp+20h]

  v48 = (unsigned int *)a2; /*0x1801a60c5*/
  v4 = a3; /*0x1801a60dc*/
  v5 = (unsigned int *)a2; /*0x1801a60e7*/
  v6 = *((_DWORD *)a1 + 6) - dword_18070A610 - HIDWORD(qword_181366908); /*0x1801a60ea*/
  if ( dword_1813668F8 < v6 || v6 < dword_1813668FC ) /*0x1801a6104*/
  {
    if ( dword_1813668F0 < v6 || v6 < dword_1813668F4 ) /*0x1801a611d*/
    {
      if ( dword_1813668E8 < v6 || v6 < dword_1813668EC ) /*0x1801a6136*/
        v8 = (unsigned int)v6 >> 31; /*0x1801a6142*/
      else
        v8 = 2; /*0x1801a6138*/
    }
    else
    {
      v8 = 3; /*0x1801a611f*/
    }
  }
  else
  {
    v8 = 4; /*0x1801a6106*/
  }
  v47 = v8; /*0x1801a6168*/
  if ( byte_18136691C == 0 )
  {
    v9 = (void **)*a1 == &GlissandoNotes::`vftable'
       ? *(_BYTE *)(a1[1] + 97)
       : (*(__int64 (__fastcall **)(__int64 *, __int64, void **))(*a1 + 192))(a1, a2, a3: &GlissandoNotes::`vftable');
    if ( v9 == 0 )
    {
      if ( v8 == 1 ) /*0x1801a6194*/
      {
        if ( v6 < 0 ) /*0x1801a619d*/
        {
          if ( (void **)*a1 == &GlissandoNotes::`vftable' ) /*0x1801a61ac*/
          {
            if ( *((_DWORD *)a1 + 21) != 1 ) /*0x1801a61b6*/
              *((_DWORD *)a1 + 21) = 1; /*0x1801a61b8*/
          }
          else
          {
            (*(void (__fastcall **)(__int64 *, __int64, void **))(*a1 + 232))(a1, a2: 1, a3: &GlissandoNotes::`vftable'); /*0x1801a6731*/
          }
          *(_DWORD *)(a1[1] + 80) = *((_DWORD *)a1 + 8); /*0x1801a61c3*/
          v10 = *a1; /*0x1801a61c6*/
          if ( (void **)*a1 == &GlissandoNotes::`vftable' ) /*0x1801a61cc*/
          {
            v11 = *((_DWORD *)a1 + 12); /*0x1801a61d2*/
          }
          else
          {
            v11 = (*(__int64 (__fastcall **)(__int64 *, __int64, void **))(v10 + 72))( /*0x1801a6749*/
                    a1,
                    a2,
                    a3: &GlissandoNotes::`vftable');
            v10 = *a1; /*0x1801a6752*/
          }
          if ( (void **)v10 == &GlissandoNotes::`vftable' ) /*0x1801a61d8*/
            v12 = *((_DWORD *)a1 + 11); /*0x1801a61de*/
          else
            v12 = (*(__int64 (__fastcall **)(__int64 *, __int64, void **))(v10 + 64))( /*0x1801a6760*/
                    a1,
                    a2,
                    a3: &GlissandoNotes::`vftable');
          sub_180305430( /*0x1801a6245*/
            a1: *((_DWORD *)a1 + 5),
            a2,
            a3: (int *)a1 + 7,
            a4: (int *)a1 + 8,
            a5: nullptr,
            a6: nullptr,
            a7: nullptr,
            a8: nullptr,
            a9: v12,
            a10: v11,
            a11: nullptr,
            a12: nullptr);
          (*(void (__fastcall **)(__int64 *, __int64, _QWORD, __int64))(*a1 + 104))(a1, a2: 1, a3: 0, a4: 100); /*0x1801a625a*/
          goto LABEL_95; /*0x1801a625d*/
        }
      }
      else if ( v8 == 0 ) /*0x1801a6264*/
      {
        goto LABEL_39; /*0x1801a6264*/
      }
      v13 = *((_BYTE *)a1 + 76); /*0x1801a626a*/
      v14 = false; /*0x1801a626e*/
      if ( v13 == 1 ) /*0x1801a627a*/
        v14 = v6 <= 0; /*0x1801a6283*/
      if ( v5 == nullptr || v14 )
      {
        if ( v6 <= 0 && v13 == 0 )
        {
          if ( v14 ) /*0x1801a63e0*/
            goto LABEL_75; /*0x1801a63e0*/
          v27 = dword_1813E4974; /*0x1801a63e6*/
          if ( dword_1813E4974 <= 1 ) /*0x1801a63f0*/
            goto LABEL_39; /*0x1801a63f0*/
          v50 = dword_1813E4974; /*0x1801a63f6*/
          v28 = &qword_1813DF048; /*0x1801a63fe*/
          v46 = 1; /*0x1801a6405*/
          do
          {
            if ( *((_BYTE *)a1 + 76) != 0 ) /*0x1801a6414*/
              break; /*0x1801a6414*/
            v29 = 0; /*0x1801a641a*/
            if ( *(int *)v28 > 0 )
            {
              v30 = v28 - 98; /*0x1801a6427*/
              while ( 1 )
              {
                v31 = (void **)*a1 == &GlissandoNotes::`vftable'
                    ? *((_DWORD *)a1 + 11)
                    : (*(__int64 (__fastcall **)(__int64 *, __int64, void **))(*a1 + 64))(
                        a1,
                        a2,
                        a3: &GlissandoNotes::`vftable');
                v32 = *(_DWORD *)v30; /*0x1801a643f*/
                if ( *(_DWORD *)v30 >= v31 ) /*0x1801a6443*/
                {
                  a2 = *a1; /*0x1801a6445*/
                  if ( (void **)*a1 == &GlissandoNotes::`vftable' ) /*0x1801a644b*/
                  {
                    v33 = *((_DWORD *)a1 + 12); /*0x1801a6451*/
                  }
                  else
                  {
                    v33 = (*(__int64 (__fastcall **)(__int64 *, __int64, void **))(a2 + 72))( /*0x1801a6781*/
                            a1,
                            a2,
                            a3: &GlissandoNotes::`vftable');
                    v32 = *(_DWORD *)v30; /*0x1801a678a*/
                  }
                  if ( v32 <= v33 ) /*0x1801a6456*/
                  {
                    v34 = *((_DWORD *)a1 + 68); /*0x1801a6458*/
                    if ( v34 == 1 || v34 == *((_DWORD *)v30 + 5) ) /*0x1801a6466*/
                      break; /*0x1801a6466*/
                  }
                }
                ++v29; /*0x1801a6468*/
                v30 = (__int64 *)((char *)v30 + 28); /*0x1801a646a*/
                if ( v29 >= *(_DWORD *)v28 ) /*0x1801a6471*/
                  goto LABEL_71; /*0x1801a6471*/
              }
              *(_DWORD *)(a1[1] + 80) = *((_DWORD *)a1 + 8); /*0x1801a647c*/
              v35 = *a1; /*0x1801a647f*/
              *((_BYTE *)a1 + 76) = 1; /*0x1801a6482*/
              *((_DWORD *)a1 + 20) = 4; /*0x1801a6486*/
              if ( (void **)v35 == &GlissandoNotes::`vftable' ) /*0x1801a6490*/
              {
                if ( *((_DWORD *)a1 + 21) != 1 ) /*0x1801a649a*/
                  *((_DWORD *)a1 + 21) = 1; /*0x1801a649c*/
              }
              else
              {
                (*(void (__fastcall **)(__int64 *, __int64, void **))(v35 + 232))( /*0x1801a6797*/
                  a1,
                  a2: 1,
                  a3: &GlissandoNotes::`vftable');
              }
              v14 = true; /*0x1801a64a0*/
LABEL_71:
              v27 = v50; /*0x1801a64a4*/
            }
            v28 += 99; /*0x1801a64b1*/
            ++v46; /*0x1801a64bb*/
          }
          while ( v46 < v27 );
          v8 = v47; /*0x1801a64c9*/
          v5 = v48; /*0x1801a64d0*/
        }
        if ( !v14 ) /*0x1801a64db*/
          goto LABEL_39; /*0x1801a64db*/
LABEL_75:
        if ( *((_BYTE *)a1 + 76) == 1 ) /*0x1801a64e5*/
          v8 = *((_DWORD *)a1 + 20); /*0x1801a64e7*/
        v36 = *a1; /*0x1801a64ea*/
        if ( (void **)*a1 == &GlissandoNotes::`vftable' ) /*0x1801a64f0*/
        {
          v37 = *((_DWORD *)a1 + 12); /*0x1801a64f6*/
        }
        else
        {
          v37 = (*(__int64 (__fastcall **)(__int64 *, __int64, void **))(v36 + 72))( /*0x1801a67af*/
                  a1,
                  a2,
                  a3: &GlissandoNotes::`vftable');
          v36 = *a1; /*0x1801a67b8*/
        }
        if ( (void **)v36 == &GlissandoNotes::`vftable' ) /*0x1801a64fc*/
          v38 = *((_DWORD *)a1 + 11); /*0x1801a6502*/
        else
          v38 = (*(__int64 (__fastcall **)(__int64 *, __int64, void **))(v36 + 64))( /*0x1801a67c6*/
                  a1,
                  a2,
                  a3: &GlissandoNotes::`vftable');
        sub_180305430( /*0x1801a6565*/
          a1: *((_DWORD *)a1 + 5),
          a2,
          a3: (int *)a1 + 7,
          a4: (int *)a1 + 8,
          a5: nullptr,
          a6: nullptr,
          a7: nullptr,
          a8: nullptr,
          a9: v38,
          a10: v37,
          a11: nullptr,
          a12: nullptr);
        v39 = sub_1803053E0(a1: (unsigned int)v6, a2: v8); /*0x1801a656f*/
        v18 = a3; /*0x1801a6574*/
        *((_DWORD *)a1 + 69) = v39; /*0x1801a6583*/
        (*(void (__fastcall **)(__int64 *, __int64, _QWORD, __int64))(*a1 + 104))(a1, a2: v40, a3, a4: 100); /*0x1801a6593*/
        (*(void (__fastcall **)(__int64 *, _QWORD))(*a1 + 112))(a1, a2: *((unsigned int *)a1 + 69)); /*0x1801a65a2*/
        if ( v5 != nullptr ) /*0x1801a65ac*/
          v41 = *v5; /*0x1801a65ae*/
        else
          v41 = *((_DWORD *)a1 + 13); /*0x1801a65bc*/
        *(_DWORD *)(a1[1] + 84) = v41; /*0x1801a65b0*/
        *((_BYTE *)a1 + 108) = 1; /*0x1801a65b3*/
        goto LABEL_96; /*0x1801a65b7*/
      }
      v15 = v5[1]; /*0x1801a629c*/
      if ( v15 != 1 ) /*0x1801a62a2*/
      {
        if ( v15 != 2 ) /*0x1801a62a7*/
        {
          if ( v15 == 3 ) /*0x1801a6337*/
          {
            v21 = *v5; /*0x1801a6339*/
            if ( (int)v21 >= 0 ) /*0x1801a633d*/
            {
              v22 = 0; /*0x1801a633f*/
              v23 = &xmmword_1813E4AD8; /*0x1801a6342*/
              while ( *(_DWORD *)v23 != (_DWORD)v21 ) /*0x1801a6352*/
              {
                v22 = (unsigned int)(v22 + 1); /*0x1801a6354*/
                v23 = (__int128 *)((char *)v23 + 24); /*0x1801a6356*/
                if ( (__int64)v23 >= (__int64)&dword_1813E56D8 ) /*0x1801a635d*/
                  goto LABEL_39; /*0x1801a635d*/
              }
              if ( (_DWORD)v22 != -1 ) /*0x1801a6364*/
              {
                v24 = 3LL * (int)v22; /*0x1801a6370*/
                v25 = *(_QWORD *)&dword_1813668B0[6 * (int)v22 + 129166]; /*0x1801a6374*/
                sub_180360060(a1: v22, a2: v21, a3: &dword_1813E56D8, a4); /*0x1801a637c*/
                Instance = (CLibXact *)CLibXact::getInstance(); /*0x1801a6381*/
                CLibXact::FadeOutSound(this: Instance, a2: v25, a3: false); /*0x1801a6390*/
                dword_1813668B0[2 * v24 + 129162] = -1; /*0x1801a6396*/
                LOWORD(dword_1813668B0[2 * v24 + 129163]) = 0; /*0x1801a63a2*/
                BYTE2(dword_1813668B0[2 * v24 + 129163]) = 0; /*0x1801a63ab*/
                *(_QWORD *)&dword_1813668B0[2 * v24 + 129164] = -1; /*0x1801a63b3*/
                *(_QWORD *)&dword_1813668B0[2 * v24 + 129166] = 0; /*0x1801a63bf*/
              }
            }
          }
          goto LABEL_39; /*0x1801a63c7*/
        }
        a2 = *v5; /*0x1801a62b4*/
        v16 = *(_DWORD **)qword_1813E46F8; /*0x1801a62b6*/
        if ( *(void **)qword_1813E46F8 != qword_1813E46F8 ) /*0x1801a62bc*/
        {
          while ( v16[4] != (_DWORD)a2 ) /*0x1801a62c3*/
          {
            v16 = *(_DWORD **)v16; /*0x1801a62c5*/
            if ( v16 == qword_1813E46F8 ) /*0x1801a62cb*/
              goto LABEL_35; /*0x1801a62cb*/
          }
          goto LABEL_39; /*0x1801a62c3*/
        }
      }
LABEL_35:
      v17 = *((_DWORD *)a1 + 68); /*0x1801a62cd*/
      if ( v17 == 1 || v17 == v5[5] ) /*0x1801a62db*/
      {
        *(_DWORD *)(a1[1] + 80) = *((_DWORD *)a1 + 8); /*0x1801a62e4*/
        if ( v6 > 0 ) /*0x1801a62ea*/
        {
          *((_BYTE *)a1 + 76) = 1; /*0x1801a62f0*/
          *((_DWORD *)a1 + 20) = 4; /*0x1801a62f4*/
          goto LABEL_39; /*0x1801a62f4*/
        }
        goto LABEL_75; /*0x1801a62ea*/
      }
LABEL_39:
      v18 = a3; /*0x1801a62fb*/
      v19 = 0; /*0x1801a6304*/
      goto LABEL_40; /*0x1801a6304*/
    }
  }
  if ( v6 >= 1 ) /*0x1801a65cf*/
    goto LABEL_39; /*0x1801a65cf*/
  v8 = 4; /*0x1801a65d8*/
  if ( (void **)*a1 != &GlissandoNotes::`vftable' ) /*0x1801a65e0*/
  {
    (*(void (__fastcall **)(__int64 *, __int64, void **))(*a1 + 232))(a1, a2: 1, a3: &GlissandoNotes::`vftable'); /*0x1801a67d9*/
    goto LABEL_89; /*0x1801a67e6*/
  }
  if ( *((_DWORD *)a1 + 21) != 1 ) /*0x1801a65ea*/
  {
    *((_DWORD *)a1 + 21) = 1; /*0x1801a65f2*/
LABEL_89:
    v4 = a3; /*0x1801a65f6*/
  }
  *(_DWORD *)(a1[1] + 80) = *((_DWORD *)a1 + 8); /*0x1801a65ff*/
  v42 = *a1; /*0x1801a6609*/
  if ( (void **)*a1 == &GlissandoNotes::`vftable' ) /*0x1801a660f*/
  {
    v43 = *((_DWORD *)a1 + 12); /*0x1801a6615*/
  }
  else
  {
    v43 = (*(__int64 (__fastcall **)(__int64 *, __int64, void **))(v42 + 72))(a1, a2, a3: &GlissandoNotes::`vftable'); /*0x1801a67f1*/
    v42 = *a1; /*0x1801a67fa*/
  }
  if ( (void **)v42 == &GlissandoNotes::`vftable' ) /*0x1801a661b*/
    v44 = *((_DWORD *)a1 + 11); /*0x1801a6621*/
  else
    v44 = (*(__int64 (__fastcall **)(__int64 *, __int64, void **))(v42 + 64))(a1, a2, a3: &GlissandoNotes::`vftable'); /*0x1801a6808*/
  sub_180305430( /*0x1801a6688*/
    a1: *((_DWORD *)a1 + 5),
    a2,
    a3: (int *)a1 + 7,
    a4: (int *)a1 + 8,
    a5: nullptr,
    a6: nullptr,
    a7: nullptr,
    a8: nullptr,
    a9: v44,
    a10: v43,
    a11: nullptr,
    a12: nullptr);
  (*(void (__fastcall **)(__int64 *, __int64, _QWORD, __int64))(*a1 + 104))(a1, a2: 4, a3: v4, a4: 100); /*0x1801a669d*/
  *((_BYTE *)a1 + 108) = 1; /*0x1801a66a0*/
  *((_BYTE *)a1 + 116) = 1; /*0x1801a66a4*/
LABEL_95:
  v18 = a3; /*0x1801a66a8*/
LABEL_96:
  if ( (void **)*a1 == &GlissandoNotes::`vftable' ) /*0x1801a66be*/
    *((_BYTE *)a1 + 68) = 1; /*0x1801a66c0*/
  else
    (*(void (__fastcall **)(__int64 *))(*a1 + 40))(a1); /*0x1801a6714*/
  sub_1800D2A70(a1: (__int64)(a1 + 16)); /*0x1801a66cb*/
  v45 = a1[1]; /*0x1801a66d0*/
  if ( *(_QWORD *)(v45 + 64) != 0 || (v19 = 1, *(_QWORD *)(v45 + 72) != 0) ) /*0x1801a66e2*/
  {
    (*(void (__fastcall **)(__int64 *))(a1[27] + 24))(a1: a1 + 27); /*0x1801a66f6*/
    (*(void (__fastcall **)(__int64 *))(a1[27] + 32))(a1: a1 + 27); /*0x1801a6707*/
    v19 = 1; /*0x1801a670a*/
  }
LABEL_40:
  *((_DWORD *)a1 + 30) = v8; /*0x1801a6306*/
  *((_BYTE *)a1 + 124) = v18; /*0x1801a6321*/
  return v19; /*0x1801a630e*/
}
