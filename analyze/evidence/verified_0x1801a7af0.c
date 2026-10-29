__int64 __fastcall sub_1801A7AF0(__int64 *a1, unsigned int *a2, __int64 a3, __int64 a4)
{
  unsigned __int8 v4; // r13
  unsigned __int8 v5; // bp
  __int64 v8; // rax
  __int64 v9; // rdx
  int v10; // ecx
  __int64 v11; // r14
  int v12; // eax
  int v13; // edx
  int v14; // eax
  int v15; // eax
  unsigned int v16; // eax
  __int64 v17; // r8
  __int64 v18; // rcx
  __int128 *v19; // rax
  __int64 v20; // rdi
  __int64 v21; // rbx
  CLibXact *v22; // rax
  int v23; // ebx
  __int64 v24; // rdx
  unsigned int v25; // edi
  __int64 v26; // r9
  __int64 v27; // rax
  void (__fastcall *v28)(__int64 *, __int64); // r8
  int v29; // edi
  int v30; // ecx
  char v31; // r13
  __int64 v32; // rax
  __int64 v33; // rcx
  __int64 v34; // rax
  int v35; // ebp
  int v36; // r9d
  unsigned int v37; // eax
  __int64 v38; // rax
  int v39; // ebp
  int v40; // r9d
  bool v41; // zf
  __int64 v42; // r8
  __int64 v43; // rdx
  __int64 v44; // rbx
  __int64 v45; // rax
  _QWORD *v46; // rbx
  __int64 v47; // rdx
  __int64 v48; // rdx
  char v49; // al
  __int64 v50; // r8
  __int64 v51; // rcx
  __int128 *v52; // rax
  __int64 v53; // rdi
  __int64 v54; // rbx
  CLibXact *Instance; // rax
  __int64 v56; // rcx
  int v57; // eax
  int v58; // r9d
  __int64 v59; // rbx
  _QWORD *v60; // rcx
  int v61; // eax
  int v62; // r9d
  __int64 v63; // rbx
  __int64 i; // rdi
  __int64 *v65; // rbx
  __int64 *v66; // rdi
  unsigned __int8 v68; // al
  int v69; // [rsp+20h] [rbp-78h]
  int v70; // [rsp+28h] [rbp-70h]
  char v71; // [rsp+A0h] [rbp+8h]
  unsigned __int8 v72; // [rsp+B0h] [rbp+18h]
  unsigned __int8 v73; // [rsp+B8h] [rbp+20h]

  v72 = a3; /*0x1801a7af0*/
  v4 = 0; /*0x1801a7b0d*/
  v5 = a3; /*0x1801a7b10*/
  v71 = 0; /*0x1801a7b14*/
  if ( (void **)*a1 == &TrillNotes::`vftable' ) /*0x1801a7b25*/
  {
    v8 = a1[1]; /*0x1801a7b2b*/
    v9 = *(unsigned __int8 *)(v8 + 97); /*0x1801a7b2f*/
    v73 = *(_BYTE *)(v8 + 97); /*0x1801a7b33*/
  }
  else
  {
    v68 = (*(__int64 (**)(void))(*a1 + 192))(); /*0x1801a83bd*/
    v9 = v68; /*0x1801a83c3*/
    v73 = v68; /*0x1801a83c6*/
  }
  v10 = *((_DWORD *)a1 + 79); /*0x1801a7b3a*/
  v11 = 2; /*0x1801a7b40*/
  if ( v10 == 0 ) /*0x1801a7b55*/
  {
    v23 = *((_DWORD *)a1 + 6) - dword_18070A610 - HIDWORD(qword_181366908); /*0x1801a7d07*/
    if ( byte_18136691C != 0 || (_BYTE)v9 != 0 ) /*0x1801a7d1c*/
    {
      if ( v23 < 1 ) /*0x1801a81c1*/
      {
        v56 = a1[1]; /*0x1801a81c7*/
        v57 = *((_DWORD *)a1 + 8); /*0x1801a81cb*/
        *((_BYTE *)a1 + 328) = 1; /*0x1801a81ce*/
        *((_DWORD *)a1 + 80) = 4; /*0x1801a81d5*/
        *((_DWORD *)a1 + 83) = v23; /*0x1801a81df*/
        *((_BYTE *)a1 + 108) = 1; /*0x1801a81e5*/
        *((_DWORD *)a1 + 86) = 99999; /*0x1801a81e9*/
        *(_DWORD *)(v56 + 80) = v57; /*0x1801a81f3*/
      }
      goto LABEL_84; /*0x1801a81f6*/
    }
    v25 = sub_180305A80(a1: (unsigned int)v23, a2: v9, a3, a4); /*0x1801a7d29*/
    if ( v25 == 1 ) /*0x1801a7d2e*/
    {
      if ( v23 < 0 ) /*0x1801a7d36*/
      {
        *((_DWORD *)a1 + 80) = 1; /*0x1801a7d3c*/
        v27 = *a1; /*0x1801a7d42*/
        v28 = *(void (__fastcall **)(__int64 *, __int64))(*a1 + 232); /*0x1801a7d45*/
        if ( (void **)*a1 == &TrillNotes::`vftable' ) /*0x1801a7d4f*/
        {
          if ( *((_DWORD *)a1 + 21) != v25 ) /*0x1801a7d58*/
            *((_DWORD *)a1 + 21) = v25; /*0x1801a7d5a*/
        }
        else
        {
          v28(a1, a2: 1); /*0x1801a83ed*/
          v27 = *a1; /*0x1801a83f0*/
        }
        v4 = 1; /*0x1801a7d62*/
        *((_DWORD *)a1 + 88) = 100; /*0x1801a7d65*/
        if ( (void **)v27 == &TrillNotes::`vftable' ) /*0x1801a7d6e*/
        {
          v29 = *((_DWORD *)a1 + 12); /*0x1801a7d74*/
        }
        else
        {
          v29 = (*(__int64 (__fastcall **)(__int64 *, __int64, void (__fastcall *)(__int64 *, __int64)))(v27 + 72))( /*0x1801a83fe*/
                  a1,
                  a2: v24,
                  a3: v28);
          v27 = *a1; /*0x1801a8400*/
        }
        if ( (void **)v27 == &TrillNotes::`vftable' ) /*0x1801a7d7a*/
          v30 = *((_DWORD *)a1 + 11); /*0x1801a7d80*/
        else
          v30 = (*(__int64 (__fastcall **)(__int64 *, __int64, void (__fastcall *)(__int64 *, __int64)))(v27 + 64))( /*0x1801a840e*/
                  a1,
                  a2: v24,
                  a3: v28);
        sub_180305430( /*0x1801a7dfa*/
          a1: *((_DWORD *)a1 + 5),
          a2: v24,
          a3: (int *)a1 + 7,
          a4: (int *)a1 + 8,
          a5: nullptr,
          a6: nullptr,
          a7: nullptr,
          a8: nullptr,
          a9: v30,
          a10: v29,
          a11: nullptr,
          a12: nullptr);
        goto LABEL_84; /*0x1801a7dff*/
      }
    }
    else if ( v25 == 0 ) /*0x1801a7e06*/
    {
      goto LABEL_84; /*0x1801a7e06*/
    }
    v31 = 0; /*0x1801a7e0c*/
    if ( *((_BYTE *)a1 + 76) != 1 || v23 > 0 ) /*0x1801a7e17*/
    {
      if ( a2 != nullptr ) /*0x1801a7e20*/
      {
        v37 = a2[1]; /*0x1801a7f1b*/
        if ( v37 != 1 ) /*0x1801a7f22*/
        {
          if ( v37 != 3 ) /*0x1801a810d*/
            goto LABEL_56; /*0x1801a810d*/
          v50 = *a2; /*0x1801a8113*/
          if ( (int)v50 < 0 ) /*0x1801a8119*/
            goto LABEL_56; /*0x1801a8119*/
          v51 = 0; /*0x1801a811f*/
          v52 = &xmmword_1813E4AD8; /*0x1801a8121*/
          while ( *(_DWORD *)v52 != (_DWORD)v50 ) /*0x1801a8133*/
          {
            v51 = (unsigned int)(v51 + 1); /*0x1801a8135*/
            v52 = (__int128 *)((char *)v52 + 24); /*0x1801a8137*/
            if ( (__int64)v52 >= (__int64)&dword_1813E56D8 ) /*0x1801a813e*/
            {
              v4 = 0; /*0x1801a8140*/
              goto LABEL_84; /*0x1801a8143*/
            }
          }
          if ( (_DWORD)v51 == -1 ) /*0x1801a814b*/
            goto LABEL_56; /*0x1801a814b*/
          v53 = 3LL * (int)v51; /*0x1801a815b*/
          v54 = *(_QWORD *)&dword_1813668B0[6 * (int)v51 + 129166]; /*0x1801a815f*/
          sub_180360060(a1: v51, a2: &dword_1813E56D8, a3: v50, a4: v26); /*0x1801a8167*/
          Instance = (CLibXact *)CLibXact::getInstance(); /*0x1801a816c*/
          CLibXact::FadeOutSound(this: Instance, a2: v54, a3: false); /*0x1801a817b*/
          v4 = 0; /*0x1801a8181*/
          dword_1813668B0[2 * v53 + 129162] = -1; /*0x1801a8184*/
          LOWORD(dword_1813668B0[2 * v53 + 129163]) = 0; /*0x1801a818f*/
          BYTE2(dword_1813668B0[2 * v53 + 129163]) = 0; /*0x1801a8199*/
          *(_QWORD *)&dword_1813668B0[2 * v53 + 129164] = -1; /*0x1801a81a1*/
          *(_QWORD *)&dword_1813668B0[2 * v53 + 129166] = 0; /*0x1801a81ad*/
LABEL_84:
          if ( *((_BYTE *)a1 + 328) != 0 ) /*0x1801a8045*/
          {
            v42 = *((unsigned int *)a1 + 8); /*0x1801a804b*/
            v43 = *((unsigned int *)a1 + 7); /*0x1801a804f*/
            *((_DWORD *)a1 + 79) = 1; /*0x1801a8052*/
            (*(void (__fastcall **)(_QWORD, __int64, __int64))(**(_QWORD **)a1[36] + 56LL))( /*0x1801a8069*/
              a1: *(_QWORD *)a1[36],
              a2: v43,
              a3: v42);
            if ( *(_DWORD *)(a1[1] + 48) != 2 && (_BYTE)dword_1813E473C != 0 ) /*0x1801a8081*/
            {
              v44 = dword_1813E57A4; /*0x1801a8087*/
              v45 = sub_1802BA000(); /*0x1801a808e*/
              v46 = (_QWORD *)sub_1802B9180(a1: *(_QWORD *)(v45 + 8 * v44 + 128)); /*0x1801a80a3*/
              sub_1800D22E0(a1: *v46, a2: 1); /*0x1801a80ad*/
              LOBYTE(v47) = 1; /*0x1801a80b5*/
              sub_1800D2150(a1: *v46, a2: v47); /*0x1801a80b7*/
              a1[38] = (__int64)v46; /*0x1801a80bc*/
              (*(void (__fastcall **)(_QWORD, __int64))(*(_QWORD *)*v46 + 224LL))(a1: *v46, a2: 54); /*0x1801a80ce*/
              sub_1800D22E0(a1: *(_QWORD *)a1[38], a2: 1); /*0x1801a80e5*/
              if ( (void **)*a1 == &TrillNotes::`vftable' ) /*0x1801a80f7*/
                v49 = *(_BYTE *)(a1[1] + 97); /*0x1801a8101*/
              else
                v49 = (*(__int64 (__fastcall **)(__int64 *))(*a1 + 192))(a1); /*0x1801a8462*/
              if ( v49 != 0 ) /*0x1801a81fd*/
              {
                v69 = dword_1812D3350; /*0x1801a8231*/
                (*(void (__fastcall **)(_QWORD))(**(_QWORD **)a1[38] + 144LL))(a1: *(_QWORD *)a1[38]); /*0x1801a823a*/
              }
              LOBYTE(v48) = 1; /*0x1801a8247*/
              sub_1800D22F0(a1: *(_QWORD *)a1[38], a2: v48); /*0x1801a824c*/
              sub_1802B9B90(a1: a1[38], a2: *((_DWORD *)a1 + 7), a3: *((_DWORD *)a1 + 8), a4: v58, a5: v69); /*0x1801a825f*/
            }
            if ( byte_18136691C == 0 && v73 == 0 ) /*0x1801a827e*/
              sub_1801A7910(a1, a2); /*0x1801a8286*/
          }
          goto LABEL_105; /*0x1801a8286*/
        }
        *(_DWORD *)(a1[1] + 80) = *((_DWORD *)a1 + 8); /*0x1801a7f2f*/
        if ( v25 == 1 ) /*0x1801a7f35*/
        {
          v71 = 1; /*0x1801a7f3c*/
          *((_DWORD *)a1 + 88) = 100; /*0x1801a7f44*/
          *((_DWORD *)a1 + 80) = 1; /*0x1801a7f4a*/
        }
        else if ( v23 <= 0 || dword_1813668F0 < v23 ) /*0x1801a7f61*/
        {
          v31 = 1; /*0x1801a7f7a*/
          if ( dword_1813668F0 >= v23 && v23 >= 0 && v25 == 4 ) /*0x1801a7f8c*/
          {
            v38 = *a1; /*0x1801a7f8e*/
            v23 = 0; /*0x1801a7f98*/
            if ( (void **)*a1 == &TrillNotes::`vftable' ) /*0x1801a7f9d*/
            {
              v39 = *((_DWORD *)a1 + 12); /*0x1801a7fa3*/
            }
            else
            {
              v39 = (*(__int64 (__fastcall **)(__int64 *))(v38 + 72))(a1); /*0x1801a8440*/
              v38 = *a1; /*0x1801a8449*/
            }
            if ( (void **)v38 == &TrillNotes::`vftable' ) /*0x1801a7fa9*/
              v40 = *((_DWORD *)a1 + 11); /*0x1801a7faf*/
            else
              v40 = (*(__int64 (__fastcall **)(__int64 *))(v38 + 64))(a1); /*0x1801a8457*/
            sub_180305430( /*0x1801a8000*/
              a1: *((_DWORD *)a1 + 5),
              a2: (__int64)a1 + 356,
              a3: (int *)a1 + 7,
              a4: (int *)a1 + 8,
              a5: nullptr,
              a6: (float *)a1 + 89,
              a7: (float *)a1 + 90,
              a8: (float *)a1 + 91,
              a9: v40,
              a10: v39,
              a11: nullptr,
              a12: nullptr);
          }
        }
        else
        {
          *((_BYTE *)a1 + 76) = 1; /*0x1801a7f65*/
          *((_DWORD *)a1 + 20) = (v25 == 4) + 3; /*0x1801a7f72*/
        }
        *((_DWORD *)a1 + 81) = sub_1803053E0(a1: (unsigned int)v23, a2: v25); /*0x1801a800e*/
        if ( v31 == 0 ) /*0x1801a8017*/
        {
LABEL_83:
          v4 = v71; /*0x1801a8035*/
          goto LABEL_84; /*0x1801a8035*/
        }
      }
      else
      {
        v32 = a1[12]; /*0x1801a7e26*/
        if ( v32 == 0 || *((_DWORD *)a1 + 26) != 0 && v23 != dword_1813668EC || *(_BYTE *)(v32 + 96) != 0 ) /*0x1801a7e41*/
          goto LABEL_56; /*0x1801a7e41*/
        v33 = a1[1]; /*0x1801a7e47*/
        if ( *(int *)(v32 + 80) <= 0 ) /*0x1801a7e4b*/
          *(_BYTE *)(v33 + 96) = 1; /*0x1801a7e4d*/
        *(_DWORD *)(v33 + 80) = *((_DWORD *)a1 + 8); /*0x1801a7e54*/
        if ( v23 <= 0 ) /*0x1801a7e59*/
        {
          if ( dword_1813668F0 >= v23 && v23 >= 0 && v25 == 4 ) /*0x1801a7e98*/
          {
            v34 = *a1; /*0x1801a7e9e*/
            v23 = 0; /*0x1801a7eab*/
            if ( (void **)*a1 == &TrillNotes::`vftable' ) /*0x1801a7eb1*/
            {
              v35 = *((_DWORD *)a1 + 12); /*0x1801a7eb7*/
            }
            else
            {
              v35 = (*(__int64 (__fastcall **)(__int64 *))(v34 + 72))(a1); /*0x1801a841b*/
              v34 = *a1; /*0x1801a8424*/
            }
            if ( (void **)v34 == &TrillNotes::`vftable' ) /*0x1801a7ebd*/
              v36 = *((_DWORD *)a1 + 11); /*0x1801a7ec3*/
            else
              v36 = (*(__int64 (__fastcall **)(__int64 *))(v34 + 64))(a1); /*0x1801a8432*/
            sub_180305430( /*0x1801a7f11*/
              a1: *((_DWORD *)a1 + 5),
              a2: (__int64)a1 + 356,
              a3: (int *)a1 + 7,
              a4: (int *)a1 + 8,
              a5: nullptr,
              a6: (float *)a1 + 89,
              a7: (float *)a1 + 90,
              a8: (float *)a1 + 91,
              a9: v36,
              a10: v35,
              a11: nullptr,
              a12: nullptr);
          }
        }
        else if ( dword_1813668F0 >= v23 ) /*0x1801a7e61*/
        {
          *((_BYTE *)a1 + 76) = 1; /*0x1801a7e69*/
          *((_DWORD *)a1 + 20) = (v25 == 4) + 3; /*0x1801a7e76*/
LABEL_56:
          v4 = 0; /*0x1801a7e79*/
          goto LABEL_84; /*0x1801a7e7c*/
        }
      }
    }
    v41 = *((_BYTE *)a1 + 76) == 1; /*0x1801a8019*/
    *((_BYTE *)a1 + 328) = 1; /*0x1801a801d*/
    *((_DWORD *)a1 + 83) = v23; /*0x1801a8024*/
    if ( v41 ) /*0x1801a802a*/
      v25 = *((_DWORD *)a1 + 20); /*0x1801a802c*/
    *((_DWORD *)a1 + 80) = v25; /*0x1801a802f*/
    goto LABEL_83; /*0x1801a802f*/
  }
  if ( v10 != 1 ) /*0x1801a7b5e*/
    goto LABEL_118; /*0x1801a7b5e*/
  v12 = *((_DWORD *)a1 + 78) - *((_DWORD *)a1 + 83) - dword_18070A610 - HIDWORD(qword_181366908); /*0x1801a7b76*/
  if ( byte_18136691C == 0 && (_BYTE)v9 == 0 ) /*0x1801a7b8b*/
  {
    if ( *((_BYTE *)a1 + 329) != 0 || v12 < 1 ) /*0x1801a7b9b*/
    {
      v13 = *((_DWORD *)a1 + 86); /*0x1801a7b9d*/
      v4 = 1; /*0x1801a7ba3*/
      if ( v13 <= 0 ) /*0x1801a7ba8*/
        v14 = 2; /*0x1801a7bb2*/
      else
        v14 = *((_DWORD *)a1 + 80); /*0x1801a7baa*/
      *((_DWORD *)a1 + 80) = v14; /*0x1801a7bb5*/
      v15 = 40; /*0x1801a7bbd*/
      if ( v13 > 0 ) /*0x1801a7bc7*/
        v15 = 100; /*0x1801a7bc7*/
      *((_DWORD *)a1 + 88) = v15; /*0x1801a7bca*/
    }
    if ( a2 == nullptr ) /*0x1801a7bd3*/
      goto LABEL_106; /*0x1801a7bd3*/
    v16 = a2[1]; /*0x1801a7bd9*/
    if ( v16 == 1 ) /*0x1801a7be0*/
    {
      *((_BYTE *)a1 + 328) = 1; /*0x1801a7be5*/
      sub_1801A7910(a1, a2); /*0x1801a7bee*/
      goto LABEL_106; /*0x1801a7bf3*/
    }
    if ( v16 != 3 ) /*0x1801a7bfb*/
      goto LABEL_106; /*0x1801a7bfb*/
    ++dword_1813E572C; /*0x1801a7c01*/
    v17 = *a2; /*0x1801a7c07*/
    if ( (int)v17 < 0 ) /*0x1801a7c0d*/
      goto LABEL_106; /*0x1801a7c0d*/
    v18 = 0; /*0x1801a7c13*/
    v19 = &xmmword_1813E4AD8; /*0x1801a7c15*/
    while ( *(_DWORD *)v19 != (_DWORD)v17 ) /*0x1801a7c26*/
    {
      v18 = (unsigned int)(v18 + 1); /*0x1801a7c28*/
      v19 = (__int128 *)((char *)v19 + 24); /*0x1801a7c2a*/
      if ( (__int64)v19 >= (__int64)&dword_1813E56D8 ) /*0x1801a7c31*/
        goto LABEL_106; /*0x1801a7c31*/
    }
    if ( (_DWORD)v18 == -1 ) /*0x1801a7c3b*/
    {
LABEL_106:
      if ( v4 == 0 ) /*0x1801a829f*/
        goto LABEL_118; /*0x1801a829f*/
      goto LABEL_107; /*0x1801a829f*/
    }
    v20 = 3LL * (int)v18; /*0x1801a7c4b*/
    v21 = *(_QWORD *)&dword_1813668B0[6 * (int)v18 + 129166]; /*0x1801a7c4f*/
    sub_180360060(a1: v18, a2: &dword_1813E56D8, a3: v17, a4); /*0x1801a7c57*/
    v22 = (CLibXact *)CLibXact::getInstance(); /*0x1801a7c5c*/
    CLibXact::FadeOutSound(this: v22, a2: v21, a3: false); /*0x1801a7c6b*/
    dword_1813668B0[2 * v20 + 129162] = -1; /*0x1801a7c71*/
    LOWORD(dword_1813668B0[2 * v20 + 129163]) = 0; /*0x1801a7c7c*/
    BYTE2(dword_1813668B0[2 * v20 + 129163]) = 0; /*0x1801a7c86*/
    *(_QWORD *)&dword_1813668B0[2 * v20 + 129164] = -1; /*0x1801a7c8e*/
    *(_QWORD *)&dword_1813668B0[2 * v20 + 129166] = 0; /*0x1801a7c9a*/
LABEL_105:
    v5 = v72; /*0x1801a8294*/
    goto LABEL_106; /*0x1801a8294*/
  }
  *((_BYTE *)a1 + 108) = 0; /*0x1801a7cab*/
  *((_BYTE *)a1 + 328) = 1; /*0x1801a7caf*/
  if ( v12 >= 1 ) /*0x1801a7cb9*/
    goto LABEL_118; /*0x1801a7cb9*/
  if ( (void **)*a1 != &TrillNotes::`vftable' ) /*0x1801a7cc5*/
  {
    (*(void (__fastcall **)(__int64 *, __int64))(*a1 + 232))(a1, a2: 1); /*0x1801a83da*/
  }
  else if ( *((_DWORD *)a1 + 21) != 1 ) /*0x1801a7ccf*/
  {
    *((_DWORD *)a1 + 21) = 1; /*0x1801a7cd6*/
    *((_DWORD *)a1 + 88) = 100; /*0x1801a7cdd*/
    v4 = 1; /*0x1801a7ce3*/
LABEL_107:
    v59 = *a1; /*0x1801a82a5*/
    if ( (void **)*a1 == &TrillNotes::`vftable' ) /*0x1801a82ab*/
    {
      *((_BYTE *)a1 + 68) = 1; /*0x1801a82b1*/
    }
    else
    {
      (*(void (__fastcall **)(__int64 *))(v59 + 40))(a1); /*0x1801a8470*/
      v59 = *a1; /*0x1801a8473*/
    }
    (*(void (__fastcall **)(__int64 *, _QWORD, _QWORD, _QWORD))(v59 + 104))( /*0x1801a82c9*/
      a1,
      a2: *((unsigned int *)a1 + 80),
      a3: v5,
      a4: *((unsigned int *)a1 + 88));
    if ( *((_DWORD *)a1 + 88) != 100 && *((_DWORD *)a1 + 81) == 0 ) /*0x1801a82dc*/
      *((_DWORD *)a1 + 81) = 1; /*0x1801a82de*/
    (*(void (__fastcall **)(__int64 *, _QWORD))(*a1 + 112))(a1, a2: *((unsigned int *)a1 + 81)); /*0x1801a82f4*/
    v60 = (_QWORD *)a1[38]; /*0x1801a82f7*/
    if ( v60 != nullptr ) /*0x1801a8301*/
    {
      sub_1800D21A0(a1: *v60, a2: &unk_1803D0954, a3: 1); /*0x1801a8314*/
      v61 = sub_180300D50(); /*0x1801a8319*/
      sub_180300C40(a1: v61, a2: 21, a3: a1[38], a4: v62, a5: v69, a6: v70, a7: *((_DWORD *)a1 + 7)); /*0x1801a8334*/
      a1[38] = 0; /*0x1801a8339*/
    }
    v63 = (__int64)(a1 + 16); /*0x1801a8344*/
    for ( i = 2; i != 0; --i ) /*0x1801a834b*/
    {
      sub_1800D2A70(a1: v63); /*0x1801a8353*/
      v63 += 40; /*0x1801a8358*/
    }
    v65 = a1 + 46; /*0x1801a8362*/
    v66 = a1 + 46; /*0x1801a8369*/
    do /*0x1801a838e*/
    {
      (*(void (__fastcall **)(__int64 *))(*v65 + 24))(a1: v66); /*0x1801a8376*/
      (*(void (__fastcall **)(__int64 *))(*v65 + 32))(a1: v66); /*0x1801a837f*/
      v66 += 7; /*0x1801a8382*/
      v65 += 7; /*0x1801a8386*/
      --v11; /*0x1801a838a*/
    }
    while ( v11 != 0 ); /*0x1801a838e*/
LABEL_118:
    *((_DWORD *)a1 + 30) = *((_DWORD *)a1 + 80); /*0x1801a8390*/
    *((_BYTE *)a1 + 124) = v5; /*0x1801a83aa*/
    return v4; /*0x1801a83ae*/
  }
  v4 = 1; /*0x1801a7ceb*/
  *((_DWORD *)a1 + 88) = 100; /*0x1801a7cf3*/
  goto LABEL_107; /*0x1801a7cf9*/
}
