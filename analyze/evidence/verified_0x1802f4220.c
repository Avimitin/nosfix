__int64 __fastcall sub_1802F4220(__int64 *a1, unsigned int *a2, unsigned __int8 a3)
{
  unsigned __int8 v3; // di
  __int64 v6; // rax
  __int64 v7; // rdx
  __int64 v8; // rcx
  __int128 *v9; // r14
  __int64 v10; // r13
  int v11; // ebp
  int v12; // ebx
  int v13; // eax
  unsigned int v14; // eax
  _QWORD *v15; // rdi
  __int64 *v16; // r12
  __int64 v17; // rdx
  __int64 v18; // r8
  __int64 v19; // r9
  int v20; // eax
  __int64 v21; // rdx
  __int64 v22; // rcx
  __int128 *v23; // rax
  __int64 v24; // rdi
  __int64 v25; // rbx
  CLibXact *v26; // rax
  int v27; // ebx
  __int64 v28; // rdx
  unsigned int v29; // edi
  __int64 v30; // r8
  __int64 v31; // r9
  void **v32; // r10
  __int64 v33; // rax
  void (__fastcall *v34)(__int64 *, __int64); // r8
  int v35; // edi
  int v36; // ecx
  __int64 v37; // rax
  __int64 v38; // rbx
  __int64 v39; // rax
  _QWORD *v40; // rbx
  __int64 v41; // rdx
  __int64 v42; // rdx
  char v43; // al
  int v44; // r9d
  bool v45; // zf
  __int64 v46; // rax
  __int64 v47; // rcx
  __int64 v48; // rax
  int v49; // ebp
  int v50; // r9d
  unsigned int v51; // eax
  __int64 v52; // rax
  int v53; // ebp
  int v54; // r9d
  int v55; // eax
  __int64 v56; // rcx
  int v57; // eax
  _QWORD *v58; // rbx
  __int64 *v59; // r15
  __int64 v60; // rdx
  __int64 v61; // rcx
  __int128 *v62; // rax
  __int64 v63; // rdi
  __int64 v64; // rbx
  CLibXact *Instance; // rax
  int v66; // eax
  __int64 v67; // rcx
  int v68; // eax
  __int64 v69; // rbx
  unsigned __int8 v70; // bp
  __int64 v71; // rdx
  __int64 v72; // r8
  __int64 v73; // r9
  _QWORD *v74; // rcx
  int v75; // eax
  int v76; // r9d
  __int64 v77; // rcx
  int v78; // eax
  __int64 v79; // rdi
  __int64 v80; // rbx
  CLibXact *v81; // rax
  __int64 *v82; // rbx
  unsigned __int8 v84; // al
  int v85; // [rsp+20h] [rbp-98h]
  int v86; // [rsp+28h] [rbp-90h]
  unsigned __int8 v87; // [rsp+60h] [rbp-58h]
  char v88; // [rsp+C0h] [rbp+8h]
  int v91; // [rsp+D8h] [rbp+20h] BYREF

  v3 = 0; /*0x1802f4244*/
  v88 = 0; /*0x1802f424a*/
  if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f4258*/
  {
    v6 = a1[1]; /*0x1802f425e*/
    v7 = *(unsigned __int8 *)(v6 + 97); /*0x1802f4262*/
    v87 = *(_BYTE *)(v6 + 97); /*0x1802f4266*/
  }
  else
  {
    v84 = (*(__int64 (**)(void))(*a1 + 192))(); /*0x1802f4d45*/
    v7 = v84; /*0x1802f4d4b*/
    v87 = v84; /*0x1802f4d4e*/
  }
  v8 = *((unsigned int *)a1 + 75); /*0x1802f426a*/
  v9 = &xmmword_1813E4AD8; /*0x1802f4270*/
  v10 = 2; /*0x1802f4294*/
  if ( (_DWORD)v8 == 0 ) /*0x1802f429a*/
  {
    v27 = *((_DWORD *)a1 + 6) - dword_18070A610 - HIDWORD(qword_181366908); /*0x1802f4524*/
    if ( byte_18136691C != 0 || (_BYTE)v7 != 0 ) /*0x1802f4539*/
    {
      if ( v27 < 1 ) /*0x1802f4a7a*/
      {
        v45 = *((_DWORD *)a1 + 22) == -1; /*0x1802f4a80*/
        *((_BYTE *)a1 + 312) = 1; /*0x1802f4a84*/
        *((_DWORD *)a1 + 76) = 4; /*0x1802f4a8b*/
        if ( !v45 && *((_DWORD *)a1 + 116) == 0 ) /*0x1802f4a9d*/
          *((_DWORD *)a1 + 116) = 2; /*0x1802f4a9f*/
        v66 = *((_DWORD *)a1 + 8); /*0x1802f4aa6*/
        v67 = a1[1]; /*0x1802f4aa9*/
        *((_DWORD *)a1 + 80) = v27; /*0x1802f4aad*/
        *(_DWORD *)(v67 + 80) = v66; /*0x1802f4ab3*/
        *((_DWORD *)a1 + 28) = *((_DWORD *)a1 + 79); /*0x1802f4abc*/
        *((_BYTE *)a1 + 108) = 1; /*0x1802f4abf*/
        *((_BYTE *)a1 + 116) = 1; /*0x1802f4ac3*/
      }
      goto LABEL_56; /*0x1802f4ac7*/
    }
    v29 = sub_180305A80(a1: (unsigned int)v27, a2: v7, a3: dword_1813668B0, a4: &dword_1813E56D8); /*0x1802f4546*/
    if ( v29 == 1 ) /*0x1802f454b*/
    {
      if ( v27 < 0 ) /*0x1802f4553*/
      {
        *((_DWORD *)a1 + 76) = 1; /*0x1802f4559*/
        v33 = *a1; /*0x1802f455f*/
        v34 = *(void (__fastcall **)(__int64 *, __int64))(*a1 + 232); /*0x1802f4562*/
        if ( (void **)*a1 == v32 ) /*0x1802f456c*/
        {
          if ( *((_DWORD *)a1 + 21) != v29 ) /*0x1802f4575*/
            *((_DWORD *)a1 + 21) = v29; /*0x1802f4577*/
        }
        else
        {
          v34(a1, a2: 1); /*0x1802f4d79*/
          v33 = *a1; /*0x1802f4d7c*/
          v32 = &LongNotes::`vftable'; /*0x1802f4d7f*/
        }
        v88 = 1; /*0x1802f457a*/
        *((_DWORD *)a1 + 81) = 100; /*0x1802f4588*/
        if ( (void **)v33 == v32 ) /*0x1802f4592*/
        {
          v35 = *((_DWORD *)a1 + 12); /*0x1802f4598*/
        }
        else
        {
          v35 = (*(__int64 (__fastcall **)(__int64 *, __int64, void (__fastcall *)(__int64 *, __int64)))(v33 + 72))( /*0x1802f4d91*/
                  a1,
                  a2: v28,
                  a3: v34);
          v32 = &LongNotes::`vftable'; /*0x1802f4d93*/
          v33 = *a1; /*0x1802f4d9a*/
        }
        if ( (void **)v33 == v32 ) /*0x1802f459e*/
          v36 = *((_DWORD *)a1 + 11); /*0x1802f45a4*/
        else
          v36 = (*(__int64 (__fastcall **)(__int64 *, __int64, void (__fastcall *)(__int64 *, __int64)))(v33 + 64))( /*0x1802f4da8*/
                  a1,
                  a2: v28,
                  a3: v34);
        sub_180305430( /*0x1802f4606*/
          a1: *((_DWORD *)a1 + 5),
          a2: v28,
          a3: (int *)a1 + 7,
          a4: (int *)a1 + 8,
          a5: nullptr,
          a6: nullptr,
          a7: nullptr,
          a8: nullptr,
          a9: v36,
          a10: v35,
          a11: nullptr,
          a12: nullptr);
LABEL_55:
        v3 = v88; /*0x1802f460b*/
        goto LABEL_56; /*0x1802f460b*/
      }
    }
    else if ( v29 == 0 ) /*0x1802f4733*/
    {
      goto LABEL_77; /*0x1802f4733*/
    }
    v45 = *((_BYTE *)a1 + 76) == 1; /*0x1802f4735*/
    LOBYTE(v91) = 0; /*0x1802f4739*/
    if ( v45 && v27 <= 0 ) /*0x1802f4745*/
      goto LABEL_101; /*0x1802f4745*/
    if ( a2 == nullptr ) /*0x1802f474e*/
    {
      v46 = a1[12]; /*0x1802f4754*/
      if ( v46 == 0 || *((_DWORD *)a1 + 26) != 0 && v27 != dword_1813668EC || *(_BYTE *)(v46 + 96) != 0 ) /*0x1802f476e*/
        goto LABEL_77; /*0x1802f476e*/
      v47 = a1[1]; /*0x1802f4770*/
      if ( *(int *)(v46 + 80) <= 0 ) /*0x1802f4777*/
        *(_BYTE *)(v47 + 96) = 1; /*0x1802f4779*/
      *(_DWORD *)(v47 + 80) = *((_DWORD *)a1 + 8); /*0x1802f4780*/
      if ( v27 <= 0 ) /*0x1802f4785*/
      {
        if ( dword_1813668F0 >= v27 && v27 >= 0 && v29 == 4 ) /*0x1802f47c4*/
        {
          v48 = *a1; /*0x1802f47ca*/
          v27 = 0; /*0x1802f47cd*/
          if ( (void **)*a1 == v32 ) /*0x1802f47d2*/
          {
            v49 = *((_DWORD *)a1 + 12); /*0x1802f47d8*/
          }
          else
          {
            v49 = (*(__int64 (__fastcall **)(__int64 *))(v48 + 72))(a1); /*0x1802f4db5*/
            v32 = &LongNotes::`vftable'; /*0x1802f4db7*/
            v48 = *a1; /*0x1802f4dbe*/
          }
          if ( (void **)v48 == v32 ) /*0x1802f47de*/
            v50 = *((_DWORD *)a1 + 11); /*0x1802f47e4*/
          else
            v50 = (*(__int64 (__fastcall **)(__int64 *))(v48 + 64))(a1); /*0x1802f4dcc*/
          sub_180305430( /*0x1802f4835*/
            a1: *((_DWORD *)a1 + 5),
            a2: (__int64)(a1 + 42),
            a3: (int *)a1 + 7,
            a4: (int *)a1 + 8,
            a5: nullptr,
            a6: (float *)a1 + 84,
            a7: (float *)a1 + 85,
            a8: (float *)a1 + 86,
            a9: v50,
            a10: v49,
            a11: nullptr,
            a12: nullptr);
        }
      }
      else if ( dword_1813668F0 >= v27 ) /*0x1802f478d*/
      {
        *((_BYTE *)a1 + 76) = 1; /*0x1802f4796*/
        *((_DWORD *)a1 + 20) = (v29 == 4) + 3; /*0x1802f47a2*/
        goto LABEL_77; /*0x1802f47a2*/
      }
LABEL_101:
      v56 = a1[1]; /*0x1802f4943*/
      *((_BYTE *)a1 + 312) = 1; /*0x1802f4947*/
      if ( a2 != nullptr ) /*0x1802f4951*/
        v57 = *a2; /*0x1802f4953*/
      else
        v57 = *((_DWORD *)a1 + 13); /*0x1802f4959*/
      *(_DWORD *)(v56 + 84) = v57; /*0x1802f495c*/
      *((_DWORD *)a1 + 80) = v27; /*0x1802f4967*/
      v58 = qword_1813E46F8; /*0x1802f496d*/
      v91 = *(_DWORD *)(v56 + 84); /*0x1802f497a*/
      v59 = *((__int64 **)qword_1813E46F8 + 1); /*0x1802f4981*/
      v17 = sub_180081F70(a1: v56, a2: qword_1813E46F8, a3: v59, a4: &v91); /*0x1802f498d*/
      if ( qword_1813E4700 == 0xAAAAAAAAAAAAAA9LL ) /*0x1802f49a8*/
        goto LABEL_161; /*0x1802f49a8*/
      ++qword_1813E4700; /*0x1802f49b1*/
      v58[1] = v17; /*0x1802f49b8*/
      *v59 = v17; /*0x1802f49bc*/
      if ( *((_BYTE *)a1 + 76) == 1 ) /*0x1802f49c3*/
        v29 = *((_DWORD *)a1 + 20); /*0x1802f49c5*/
      *((_DWORD *)a1 + 76) = v29; /*0x1802f49c8*/
      goto LABEL_55; /*0x1802f49ce*/
    }
    v51 = a2[1]; /*0x1802f483f*/
    if ( v51 == 1 ) /*0x1802f4847*/
    {
      *(_DWORD *)(a1[1] + 80) = *((_DWORD *)a1 + 8); /*0x1802f4854*/
      if ( v29 == 1 ) /*0x1802f485a*/
      {
        v88 = 1; /*0x1802f4862*/
        *((_DWORD *)a1 + 81) = 100; /*0x1802f486a*/
        *((_DWORD *)a1 + 76) = 1; /*0x1802f4871*/
      }
      else if ( v27 <= 0 || dword_1813668F0 < v27 ) /*0x1802f4888*/
      {
        LOBYTE(v91) = 1; /*0x1802f48a1*/
        if ( dword_1813668F0 >= v27 && v27 >= 0 && v29 == 4 ) /*0x1802f48b4*/
        {
          v52 = *a1; /*0x1802f48b6*/
          v27 = 0; /*0x1802f48b9*/
          if ( (void **)*a1 == v32 ) /*0x1802f48be*/
          {
            v53 = *((_DWORD *)a1 + 12); /*0x1802f48c4*/
          }
          else
          {
            v53 = (*(__int64 (__fastcall **)(__int64 *))(v52 + 72))(a1); /*0x1802f4dda*/
            v32 = &LongNotes::`vftable'; /*0x1802f4ddc*/
            v52 = *a1; /*0x1802f4de3*/
          }
          if ( (void **)v52 == v32 ) /*0x1802f48ca*/
            v54 = *((_DWORD *)a1 + 11); /*0x1802f48d0*/
          else
            v54 = (*(__int64 (__fastcall **)(__int64 *))(v52 + 64))(a1); /*0x1802f4df1*/
          sub_180305430( /*0x1802f4921*/
            a1: *((_DWORD *)a1 + 5),
            a2: (__int64)(a1 + 42),
            a3: (int *)a1 + 7,
            a4: (int *)a1 + 8,
            a5: nullptr,
            a6: (float *)a1 + 84,
            a7: (float *)a1 + 85,
            a8: (float *)a1 + 86,
            a9: v54,
            a10: v53,
            a11: nullptr,
            a12: nullptr);
        }
      }
      else
      {
        *((_BYTE *)a1 + 76) = 1; /*0x1802f488d*/
        *((_DWORD *)a1 + 20) = (v29 == 4) + 3; /*0x1802f4899*/
      }
      v55 = sub_1803053E0(a1: (unsigned int)v27, a2: v29); /*0x1802f492a*/
      v45 = (_BYTE)v91 == 0; /*0x1802f492f*/
      *((_DWORD *)a1 + 77) = v55; /*0x1802f4937*/
      if ( v45 ) /*0x1802f493d*/
        goto LABEL_55; /*0x1802f493d*/
      goto LABEL_101; /*0x1802f493d*/
    }
    if ( v51 == 3 ) /*0x1802f49d6*/
    {
      v60 = *a2; /*0x1802f49dc*/
      if ( (int)v60 >= 0 ) /*0x1802f49e2*/
      {
        v61 = 0; /*0x1802f49e8*/
        v62 = &xmmword_1813E4AD8; /*0x1802f49ea*/
        while ( *(_DWORD *)v62 != (_DWORD)v60 ) /*0x1802f49f2*/
        {
          v61 = (unsigned int)(v61 + 1); /*0x1802f49f4*/
          v62 = (__int128 *)((char *)v62 + 24); /*0x1802f49f6*/
          if ( (__int64)v62 >= v31 ) /*0x1802f49fd*/
          {
            v3 = 0; /*0x1802f49ff*/
            goto LABEL_56; /*0x1802f4a02*/
          }
        }
        if ( (_DWORD)v61 != -1 ) /*0x1802f4a0a*/
        {
          v63 = 3LL * (int)v61; /*0x1802f4a13*/
          v64 = *(_QWORD *)(v30 + 24LL * (int)v61 + 516664); /*0x1802f4a17*/
          sub_180360060(a1: v61, a2: v60, a3: v30, a4: v31); /*0x1802f4a1f*/
          Instance = (CLibXact *)CLibXact::getInstance(); /*0x1802f4a24*/
          CLibXact::FadeOutSound(this: Instance, a2: v64, a3: false); /*0x1802f4a33*/
          dword_1813668B0[2 * v63 + 129162] = -1; /*0x1802f4a40*/
          LOWORD(dword_1813668B0[2 * v63 + 129163]) = 0; /*0x1802f4a4b*/
          BYTE2(dword_1813668B0[2 * v63 + 129163]) = 0; /*0x1802f4a53*/
          *(_QWORD *)&dword_1813668B0[2 * v63 + 129164] = -1; /*0x1802f4a5b*/
          *(_QWORD *)&dword_1813668B0[2 * v63 + 129166] = 0; /*0x1802f4a67*/
          v3 = 0; /*0x1802f4a6f*/
LABEL_56:
          if ( *((_BYTE *)a1 + 312) != 0 ) /*0x1802f461a*/
          {
            v37 = a1[1]; /*0x1802f4620*/
            *((_DWORD *)a1 + 75) = 1; /*0x1802f4624*/
            if ( *(_DWORD *)(v37 + 48) != 2 && (_BYTE)dword_1813E473C != 0 ) /*0x1802f463f*/
            {
              v38 = dword_1813E57A0; /*0x1802f4645*/
              v39 = sub_1802BA000(); /*0x1802f464c*/
              v40 = (_QWORD *)sub_1802B9180(a1: *(_QWORD *)(v39 + 8 * v38 + 128)); /*0x1802f4661*/
              sub_1800D22E0(a1: *v40, a2: 1); /*0x1802f466b*/
              LOBYTE(v41) = 1; /*0x1802f4673*/
              sub_1800D2150(a1: *v40, a2: v41); /*0x1802f4675*/
              a1[36] = (__int64)v40; /*0x1802f467a*/
              (*(void (__fastcall **)(_QWORD, __int64))(*(_QWORD *)*v40 + 224LL))(a1: *v40, a2: 54); /*0x1802f468c*/
              sub_1800D22E0(a1: *(_QWORD *)a1[36], a2: 1); /*0x1802f46a3*/
              if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f46b5*/
                v43 = *(_BYTE *)(a1[1] + 97); /*0x1802f46bf*/
              else
                v43 = (*(__int64 (__fastcall **)(__int64 *))(*a1 + 192))(a1); /*0x1802f4e09*/
              if ( v43 != 0 ) /*0x1802f46c5*/
              {
                v85 = dword_181366090; /*0x1802f46f9*/
                (*(void (__fastcall **)(_QWORD))(**(_QWORD **)a1[36] + 144LL))(a1: *(_QWORD *)a1[36]); /*0x1802f4702*/
              }
              LOBYTE(v42) = 1; /*0x1802f470f*/
              sub_1800D22F0(a1: *(_QWORD *)a1[36], a2: v42); /*0x1802f4714*/
              sub_1802B9B90(a1: a1[36], a2: *((_DWORD *)a1 + 7), a3: *((_DWORD *)a1 + 8), a4: v44, a5: v85); /*0x1802f4727*/
            }
            if ( byte_18136691C == 0 && v87 == 0 ) /*0x1802f4ae1*/
            {
              if ( a2 != nullptr ) /*0x1802f4ae6*/
              {
                v68 = dword_1813E56E0 + 1; /*0x1802f4af3*/
                if ( dword_1813E56E0 + 1 > 20000 ) /*0x1802f4afa*/
                  v68 = 10000; /*0x1802f4afa*/
                dword_1813E56E0 = v68; /*0x1802f4afd*/
                *((_DWORD *)a1 + 79) = v68; /*0x1802f4b03*/
                *((_DWORD *)a1 + 28) = v68; /*0x1802f4b09*/
                *((_BYTE *)a1 + 108) = 1; /*0x1802f4b0c*/
              }
              else
              {
                *((_BYTE *)a1 + 108) = 1; /*0x1802f4b17*/
                *((_DWORD *)a1 + 79) = -1; /*0x1802f4b1b*/
                *((_DWORD *)a1 + 28) = -1; /*0x1802f4b21*/
              }
            }
          }
          goto LABEL_129; /*0x1802f4b10*/
        }
      }
    }
LABEL_77:
    v3 = 0; /*0x1802f47a5*/
    goto LABEL_56; /*0x1802f47a8*/
  }
  if ( (_DWORD)v8 != 1 ) /*0x1802f42a3*/
    goto LABEL_149; /*0x1802f42a3*/
  v11 = *((_DWORD *)a1 + 74) - *((_DWORD *)a1 + 80) - dword_18070A610 - HIDWORD(qword_181366908); /*0x1802f42bb*/
  if ( byte_18136691C != 0 || (_BYTE)v7 != 0 ) /*0x1802f42d0*/
  {
    *((_BYTE *)a1 + 108) = 0; /*0x1802f44c6*/
    *((_BYTE *)a1 + 312) = 1; /*0x1802f44ca*/
    if ( v11 < 1 ) /*0x1802f44d4*/
    {
      if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f44e0*/
      {
        if ( *((_DWORD *)a1 + 21) != 1 ) /*0x1802f44ea*/
          *((_DWORD *)a1 + 21) = 1; /*0x1802f44ec*/
      }
      else
      {
        (*(void (__fastcall **)(__int64 *, __int64, int *, int *))(*a1 + 232))( /*0x1802f4d66*/
          a1,
          a2: 1,
          a3: dword_1813668B0,
          a4: &dword_1813E56D8);
      }
      v88 = 1; /*0x1802f44f9*/
      *((_DWORD *)a1 + 81) = 100; /*0x1802f4501*/
      goto LABEL_130; /*0x1802f4516*/
    }
    goto LABEL_149; /*0x1802f44d4*/
  }
  v12 = 50; /*0x1802f42d6*/
  *((_BYTE *)a1 + 108) = 0; /*0x1802f42d9*/
  if ( *((_BYTE *)a1 + 313) != 0 || v11 < 1 ) /*0x1802f42ea*/
  {
    v3 = 1; /*0x1802f42ec*/
    v88 = 1; /*0x1802f42ef*/
    if ( v11 > 15 ) /*0x1802f42fa*/
      v8 = 2; /*0x1802f4304*/
    else
      v8 = *((unsigned int *)a1 + 76); /*0x1802f42fc*/
    v13 = 50; /*0x1802f4307*/
    *((_DWORD *)a1 + 76) = v8; /*0x1802f4309*/
    if ( v11 <= 15 ) /*0x1802f430f*/
      v13 = 100; /*0x1802f430f*/
    *((_DWORD *)a1 + 81) = v13; /*0x1802f4313*/
    if ( (_DWORD)v8 == 2 ) /*0x1802f431c*/
      ++dword_1813E572C; /*0x1802f431e*/
  }
  if ( a2 == nullptr ) /*0x1802f4327*/
  {
LABEL_129:
    if ( v3 != 0 ) /*0x1802f4b41*/
      goto LABEL_130; /*0x1802f4b41*/
LABEL_149:
    v70 = a3; /*0x1802f4d00*/
    goto LABEL_150; /*0x1802f4d00*/
  }
  v14 = a2[1]; /*0x1802f432d*/
  if ( v14 == 2 ) /*0x1802f4335*/
  {
    *((_BYTE *)a1 + 312) = 1; /*0x1802f4337*/
    v15 = qword_1813E46F8; /*0x1802f4346*/
    v91 = *a2; /*0x1802f4354*/
    v16 = *((__int64 **)qword_1813E46F8 + 1); /*0x1802f435b*/
    v17 = sub_180081F70(a1: v8, a2: qword_1813E46F8, a3: v16, a4: &v91); /*0x1802f4367*/
    if ( qword_1813E4700 == 0xAAAAAAAAAAAAAA9LL ) /*0x1802f4382*/
LABEL_161:
      sub_1800E2374(a1: "list<T> too long", a2: v17); /*0x1802f4df9*/
    ++qword_1813E4700; /*0x1802f438b*/
    v15[1] = v17; /*0x1802f439a*/
    *v16 = v17; /*0x1802f439e*/
    v14 = a2[1]; /*0x1802f43a2*/
  }
  if ( v14 == 3 && (unsigned __int8)sub_1802F4060(a1) == 0 ) /*0x1802f43b8*/
  {
    v88 = 1; /*0x1802f43be*/
    if ( v11 > 15 ) /*0x1802f43c9*/
      v20 = 2; /*0x1802f43d3*/
    else
      v20 = *((_DWORD *)a1 + 76); /*0x1802f43cb*/
    if ( v11 <= 15 ) /*0x1802f43d6*/
      v12 = 100; /*0x1802f43d6*/
    *((_DWORD *)a1 + 76) = v20; /*0x1802f43da*/
    *((_DWORD *)a1 + 81) = v12; /*0x1802f43e0*/
    if ( v20 == 2 ) /*0x1802f43e9*/
      ++dword_1813E572C; /*0x1802f43eb*/
    v21 = *((unsigned int *)a1 + 79); /*0x1802f43f1*/
    if ( (int)v21 >= 0 ) /*0x1802f43fa*/
    {
      v22 = 0; /*0x1802f440f*/
      v23 = &xmmword_1813E4AD8; /*0x1802f4411*/
      while ( *(_DWORD *)v23 != (_DWORD)v21 ) /*0x1802f4416*/
      {
        v22 = (unsigned int)(v22 + 1); /*0x1802f4418*/
        v23 = (__int128 *)((char *)v23 + 24); /*0x1802f441a*/
        if ( (__int64)v23 >= (__int64)&dword_1813E56D8 ) /*0x1802f4421*/
          goto LABEL_36; /*0x1802f4421*/
      }
      if ( (_DWORD)v22 != -1 ) /*0x1802f4428*/
      {
        v24 = 3LL * (int)v22; /*0x1802f442d*/
        v25 = *(_QWORD *)&dword_1813668B0[6 * (int)v22 + 129166]; /*0x1802f4438*/
        sub_180360060(a1: v22, a2: v21, a3: v18, a4: v19); /*0x1802f4440*/
        v26 = (CLibXact *)CLibXact::getInstance(); /*0x1802f4445*/
        CLibXact::FadeOutSound(this: v26, a2: v25, a3: false); /*0x1802f4454*/
        dword_1813668B0[2 * v24 + 129162] = -1; /*0x1802f4461*/
        LOWORD(dword_1813668B0[2 * v24 + 129163]) = 0; /*0x1802f446d*/
        BYTE2(dword_1813668B0[2 * v24 + 129163]) = 0; /*0x1802f4478*/
        *(_QWORD *)&dword_1813668B0[2 * v24 + 129164] = -1; /*0x1802f4481*/
        *(_QWORD *)&dword_1813668B0[2 * v24 + 129166] = 0; /*0x1802f448d*/
      }
    }
  }
LABEL_36:
  if ( v11 >= 1 ) /*0x1802f44ac*/
  {
    v3 = v88; /*0x1802f4b26*/
    goto LABEL_129; /*0x1802f4b26*/
  }
  v88 = 1; /*0x1802f44b2*/
  *((_DWORD *)a1 + 81) = 100; /*0x1802f44ba*/
LABEL_130:
  v69 = *a1; /*0x1802f4b47*/
  if ( (void **)*a1 == &LongNotes::`vftable' ) /*0x1802f4b4d*/
  {
    *((_BYTE *)a1 + 68) = 1; /*0x1802f4b53*/
  }
  else
  {
    (*(void (__fastcall **)(__int64 *))(v69 + 40))(a1); /*0x1802f4d3a*/
    v69 = *a1; /*0x1802f4d3d*/
  }
  v70 = a3; /*0x1802f4b57*/
  (*(void (__fastcall **)(__int64 *, _QWORD, _QWORD, _QWORD))(v69 + 104))( /*0x1802f4b73*/
    a1,
    a2: *((unsigned int *)a1 + 76),
    a3,
    a4: *((unsigned int *)a1 + 81));
  if ( *((_DWORD *)a1 + 81) != 100 && *((_DWORD *)a1 + 77) == 0 ) /*0x1802f4b86*/
    *((_DWORD *)a1 + 77) = 1; /*0x1802f4b88*/
  (*(void (__fastcall **)(__int64 *, _QWORD))(*a1 + 112))(a1, a2: *((unsigned int *)a1 + 77)); /*0x1802f4b9e*/
  v74 = (_QWORD *)a1[36]; /*0x1802f4ba1*/
  if ( v74 != nullptr ) /*0x1802f4bab*/
  {
    sub_1800D21A0(a1: *v74, a2: &unk_1803FD5B4, a3: 1); /*0x1802f4bbe*/
    v75 = sub_180300D50(); /*0x1802f4bc3*/
    sub_180300C40(a1: v75, a2: 20, a3: a1[36], a4: v76, a5: v85, a6: v86, a7: *((_DWORD *)a1 + 7)); /*0x1802f4bde*/
    a1[36] = 0; /*0x1802f4be3*/
  }
  v77 = *((unsigned int *)a1 + 79); /*0x1802f4bee*/
  if ( (_DWORD)v77 != -1 && *((int *)a1 + 76) < 3 && (int)v77 >= 0 ) /*0x1802f4c0c*/
  {
    v78 = 0; /*0x1802f4c12*/
    while ( *(_DWORD *)v9 != (_DWORD)v77 ) /*0x1802f4c17*/
    {
      ++v78; /*0x1802f4c19*/
      v9 = (__int128 *)((char *)v9 + 24); /*0x1802f4c1b*/
      if ( (__int64)v9 >= (__int64)&dword_1813E56D8 ) /*0x1802f4c22*/
        goto LABEL_146; /*0x1802f4c22*/
    }
    if ( v78 != -1 ) /*0x1802f4c29*/
    {
      v79 = 3LL * v78; /*0x1802f4c34*/
      v80 = *(_QWORD *)&dword_1813668B0[6 * v78 + 129166]; /*0x1802f4c38*/
      sub_180360060(a1: v77, a2: v71, a3: v72, a4: v73); /*0x1802f4c40*/
      v81 = (CLibXact *)CLibXact::getInstance(); /*0x1802f4c45*/
      CLibXact::FadeOutSound(this: v81, a2: v80, a3: false); /*0x1802f4c54*/
      dword_1813668B0[2 * v79 + 129162] = -1; /*0x1802f4c5a*/
      LOWORD(dword_1813668B0[2 * v79 + 129163]) = 0; /*0x1802f4c66*/
      BYTE2(dword_1813668B0[2 * v79 + 129163]) = 0; /*0x1802f4c71*/
      *(_QWORD *)&dword_1813668B0[2 * v79 + 129164] = -1; /*0x1802f4c7a*/
      *(_QWORD *)&dword_1813668B0[2 * v79 + 129166] = 0; /*0x1802f4c86*/
    }
  }
LABEL_146:
  v82 = a1 + 16; /*0x1802f4c92*/
  do /*0x1802f4cb0*/
  {
    sub_1800D2A70(a1: v82); /*0x1802f4ca3*/
    v82 += 5; /*0x1802f4ca8*/
    --v10; /*0x1802f4cac*/
  }
  while ( v10 != 0 ); /*0x1802f4cb0*/
  (*(void (__fastcall **)(__int64 *))(a1[44] + 24))(a1: a1 + 44); /*0x1802f4cc0*/
  (*(void (__fastcall **)(__int64 *))(a1[44] + 32))(a1: a1 + 44); /*0x1802f4cd1*/
  (*(void (__fastcall **)(__int64 *))(a1[51] + 24))(a1: a1 + 51); /*0x1802f4ce2*/
  (*(void (__fastcall **)(__int64 *))(a1[51] + 32))(a1: a1 + 51); /*0x1802f4cf3*/
  v3 = v88; /*0x1802f4cf6*/
LABEL_150:
  *((_DWORD *)a1 + 30) = *((_DWORD *)a1 + 76); /*0x1802f4d08*/
  *((_BYTE *)a1 + 124) = v70; /*0x1802f4d22*/
  return v3; /*0x1802f4d26*/
}
