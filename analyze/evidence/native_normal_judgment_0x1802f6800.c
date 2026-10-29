__int64 __fastcall sub_1802F6800(int *a1, __int64 a2, unsigned __int8 a3, __int64 a4)
{
  unsigned __int8 v4; // r12
  int v6; // edi
  int *v8; // r15
  int v9; // ecx
  int v10; // ebx
  unsigned int v11; // r14d
  char v12; // al
  __int64 v13; // rax
  void (__fastcall *v14)(int *, __int64); // r8
  int v15; // r15d
  int v16; // ecx
  int v17; // ebp
  __int64 v18; // rax
  __int64 v19; // rcx
  unsigned int v20; // eax
  __int64 v21; // rax
  int v22; // ebx
  int v23; // ecx
  int v24; // eax
  __int64 v25; // rdx
  __int64 v26; // rcx
  __int128 *v27; // rax
  __int64 v28; // rdi
  __int64 v29; // rbx
  CLibXact *Instance; // rax
  __int64 v31; // rax
  void (__fastcall *v32)(int *, __int64); // r8
  int v33; // r15d
  int v34; // ecx

  v4 = 0; /*0x1802f681f*/
  v6 = a1[6] - dword_18070A610 - HIDWORD(qword_181366908); /*0x1802f682b*/
  v8 = (int *)a2; /*0x1802f683b*/
  v9 = dword_1813668EC; /*0x1802f683e*/
  v10 = 2; /*0x1802f6844*/
  if ( dword_1813668F8 < v6 || v6 < dword_1813668FC ) /*0x1802f6851*/
  {
    if ( dword_1813668F0 < v6 || v6 < dword_1813668F4 ) /*0x1802f6867*/
    {
      if ( dword_1813668E8 < v6 || v6 < dword_1813668EC ) /*0x1802f687b*/
        v11 = (unsigned int)v6 >> 31; /*0x1802f6885*/
      else
        v11 = 2; /*0x1802f687d*/
    }
    else
    {
      v11 = 3; /*0x1802f6869*/
    }
  }
  else
  {
    v11 = 4; /*0x1802f6853*/
  }
  if ( byte_18136691C != 0
    || (*(void ***)a1 != &NormalNotes::`vftable'
      ? (v12 = (*(__int64 (__fastcall **)(int *))(*(_QWORD *)a1 + 192LL))(a1), v9 = dword_1813668EC)
      : (v12 = *(_BYTE *)(*((_QWORD *)a1 + 1) + 97LL)),
        v12 != 0) )
  {
    if ( v6 >= 1 ) /*0x1802f6c26*/
      goto LABEL_86; /*0x1802f6c26*/
    v31 = *(_QWORD *)a1; /*0x1802f6c2c*/
    v11 = 4; /*0x1802f6c2f*/
    v32 = *(void (__fastcall **)(int *, __int64))(*(_QWORD *)a1 + 232LL); /*0x1802f6c35*/
    if ( *(void ***)a1 == &NormalNotes::`vftable' ) /*0x1802f6c3f*/
    {
      if ( a1[21] != 1 ) /*0x1802f6c49*/
        a1[21] = 1; /*0x1802f6c50*/
    }
    else
    {
      v32(a1, a2: 1); /*0x1802f6db0*/
      v31 = *(_QWORD *)a1; /*0x1802f6db3*/
    }
    if ( (void **)v31 == &NormalNotes::`vftable' ) /*0x1802f6c56*/
    {
      v33 = a1[12]; /*0x1802f6c5c*/
    }
    else
    {
      v33 = (*(__int64 (__fastcall **)(int *, __int64, void (__fastcall *)(int *, __int64)))(v31 + 72))(a1, a2, a3: v32); /*0x1802f6dc1*/
      v31 = *(_QWORD *)a1; /*0x1802f6dc4*/
    }
    if ( (void **)v31 == &NormalNotes::`vftable' ) /*0x1802f6c63*/
      v34 = a1[11]; /*0x1802f6c69*/
    else
      v34 = (*(__int64 (__fastcall **)(int *, __int64, void (__fastcall *)(int *, __int64)))(v31 + 64))(a1, a2, a3: v32); /*0x1802f6dd2*/
    sub_180305430( /*0x1802f6ccc*/
      a1: a1[5],
      a2,
      a3: a1 + 7,
      a4: a1 + 8,
      a5: nullptr,
      a6: nullptr,
      a7: nullptr,
      a8: nullptr,
      a9: v34,
      a10: v33,
      a11: nullptr,
      a12: nullptr);
    (*(void (__fastcall **)(int *, __int64, _QWORD, __int64))(*(_QWORD *)a1 + 104LL))(a1, a2: 4, a3, a4: 100); /*0x1802f6ce2*/
    *(_DWORD *)(*((_QWORD *)a1 + 1) + 80LL) = a1[8]; /*0x1802f6cec*/
    *((_BYTE *)a1 + 108) = 1; /*0x1802f6cef*/
    *((_BYTE *)a1 + 116) = 1; /*0x1802f6cf3*/
    goto LABEL_83; /*0x1802f6cf3*/
  }
  if ( v11 == 1 ) /*0x1802f68bd*/
  {
    if ( v6 < 0 ) /*0x1802f68c5*/
    {
      v13 = *(_QWORD *)a1; /*0x1802f68cb*/
      v14 = *(void (__fastcall **)(int *, __int64))(*(_QWORD *)a1 + 232LL); /*0x1802f68d1*/
      if ( *(void ***)a1 == &NormalNotes::`vftable' ) /*0x1802f68db*/
      {
        if ( a1[21] != 1 ) /*0x1802f68e4*/
          a1[21] = 1; /*0x1802f68e6*/
      }
      else
      {
        v14(a1, a2: 1); /*0x1802f6d62*/
        v13 = *(_QWORD *)a1; /*0x1802f6d65*/
      }
      if ( (void **)v13 == &NormalNotes::`vftable' ) /*0x1802f68ec*/
      {
        v15 = a1[12]; /*0x1802f68f2*/
      }
      else
      {
        v15 = (*(__int64 (__fastcall **)(int *, __int64, void (__fastcall *)(int *, __int64)))(v13 + 72))( /*0x1802f6d73*/
                a1,
                a2,
                a3: v14);
        v13 = *(_QWORD *)a1; /*0x1802f6d76*/
      }
      if ( (void **)v13 == &NormalNotes::`vftable' ) /*0x1802f68f9*/
        v16 = a1[11]; /*0x1802f68ff*/
      else
        v16 = (*(__int64 (__fastcall **)(int *, __int64, void (__fastcall *)(int *, __int64)))(v13 + 64))( /*0x1802f6d84*/
                a1,
                a2,
                a3: v14);
      sub_180305430( /*0x1802f6962*/
        a1: a1[5],
        a2,
        a3: a1 + 7,
        a4: a1 + 8,
        a5: nullptr,
        a6: nullptr,
        a7: nullptr,
        a8: nullptr,
        a9: v16,
        a10: v15,
        a11: nullptr,
        a12: nullptr);
      (*(void (__fastcall **)(int *, __int64, _QWORD, __int64))(*(_QWORD *)a1 + 104LL))(a1, a2: 1, a3: 0, a4: 100); /*0x1802f6976*/
LABEL_83:
      if ( *(void ***)a1 == &NormalNotes::`vftable' ) /*0x1802f6d04*/
        *((_BYTE *)a1 + 68) = 1; /*0x1802f6d0a*/
      else
        (*(void (__fastcall **)(int *))(*(_QWORD *)a1 + 40LL))(a1); /*0x1802f6ddc*/
      sub_1800D2A70(a1: a1 + 32); /*0x1802f6d17*/
      v4 = 1; /*0x1802f6d1c*/
      goto LABEL_86; /*0x1802f6d1c*/
    }
  }
  else if ( v11 == 0 ) /*0x1802f6981*/
  {
    goto LABEL_86; /*0x1802f6981*/
  }
  LOBYTE(a2) = 0; /*0x1802f6987*/
  v17 = 0; /*0x1802f6989*/
  if ( *((_BYTE *)a1 + 76) == 1 && v6 <= 0 ) /*0x1802f6993*/
  {
LABEL_56:
    if ( *((_BYTE *)a1 + 76) == 1 ) /*0x1802f6aa6*/
      v11 = a1[20]; /*0x1802f6aa8*/
    v21 = *(_QWORD *)a1; /*0x1802f6aac*/
    if ( *(void ***)a1 == &NormalNotes::`vftable' ) /*0x1802f6ab9*/
    {
      v22 = a1[12]; /*0x1802f6abf*/
    }
    else
    {
      v22 = (*(__int64 (__fastcall **)(int *, __int64))(v21 + 72))(a1, a2); /*0x1802f6d91*/
      v21 = *(_QWORD *)a1; /*0x1802f6d93*/
    }
    if ( (void **)v21 == &NormalNotes::`vftable' ) /*0x1802f6ac5*/
      v23 = a1[11]; /*0x1802f6acb*/
    else
      v23 = (*(__int64 (__fastcall **)(int *, __int64))(v21 + 64))(a1, a2); /*0x1802f6da1*/
    sub_180305430( /*0x1802f6b2b*/
      a1: a1[5],
      a2,
      a3: a1 + 7,
      a4: a1 + 8,
      a5: nullptr,
      a6: nullptr,
      a7: nullptr,
      a8: nullptr,
      a9: v23,
      a10: v22,
      a11: nullptr,
      a12: nullptr);
    (*(void (__fastcall **)(int *, _QWORD, _QWORD, __int64))(*(_QWORD *)a1 + 104LL))(a1, a2: v11, a3, a4: 100); /*0x1802f6b43*/
    (*(void (__fastcall **)(int *, _QWORD))(*(_QWORD *)a1 + 112LL))(a1, a2: (unsigned int)a1[52]); /*0x1802f6b52*/
    if ( v8 != nullptr ) /*0x1802f6b5c*/
      v24 = *v8; /*0x1802f6b5e*/
    else
      v24 = a1[13]; /*0x1802f6b6d*/
    *(_DWORD *)(*((_QWORD *)a1 + 1) + 84LL) = v24; /*0x1802f6b61*/
    *((_BYTE *)a1 + 108) = 1; /*0x1802f6b64*/
    goto LABEL_83; /*0x1802f6b68*/
  }
  if ( v8 != nullptr ) /*0x1802f699c*/
  {
    v20 = v8[1]; /*0x1802f6a1b*/
    if ( v20 != 1 ) /*0x1802f6a22*/
    {
      if ( v20 == 3 ) /*0x1802f6b7f*/
      {
        v25 = (unsigned int)*v8; /*0x1802f6b85*/
        if ( (int)v25 >= 0 ) /*0x1802f6b8a*/
        {
          v26 = 0; /*0x1802f6b90*/
          v27 = &xmmword_1813E4AD8; /*0x1802f6b92*/
          while ( *(_DWORD *)v27 != (_DWORD)v25 ) /*0x1802f6ba2*/
          {
            v26 = (unsigned int)(v26 + 1); /*0x1802f6ba4*/
            v27 = (__int128 *)((char *)v27 + 24); /*0x1802f6ba6*/
            if ( (__int64)v27 >= (__int64)&dword_1813E56D8 ) /*0x1802f6bad*/
              goto LABEL_86; /*0x1802f6bad*/
          }
          if ( (_DWORD)v26 != -1 ) /*0x1802f6bb7*/
          {
            v28 = 3LL * (int)v26; /*0x1802f6bc7*/
            v29 = *(_QWORD *)&dword_1813668B0[6 * (int)v26 + 129166]; /*0x1802f6bcb*/
            sub_180360060(a1: v26, a2: v25, a3: &dword_1813E56D8, a4); /*0x1802f6bd3*/
            Instance = (CLibXact *)CLibXact::getInstance(); /*0x1802f6bd8*/
            CLibXact::FadeOutSound(this: Instance, a2: v29, a3: false); /*0x1802f6be7*/
            dword_1813668B0[2 * v28 + 129162] = -1; /*0x1802f6bed*/
            LOWORD(dword_1813668B0[2 * v28 + 129163]) = 0; /*0x1802f6bf9*/
            BYTE2(dword_1813668B0[2 * v28 + 129163]) = 0; /*0x1802f6c02*/
            *(_QWORD *)&dword_1813668B0[2 * v28 + 129164] = -1; /*0x1802f6c0a*/
            *(_QWORD *)&dword_1813668B0[2 * v28 + 129166] = 0; /*0x1802f6c16*/
          }
        }
      }
      goto LABEL_86; /*0x1802f6c1e*/
    }
    *(_DWORD *)(*((_QWORD *)a1 + 1) + 80LL) = a1[8]; /*0x1802f6a2f*/
    if ( v6 <= 0 || dword_1813668F0 < v6 ) /*0x1802f6a3e*/
    {
      LOBYTE(a2) = 1; /*0x1802f6a70*/
      if ( dword_1813668F0 >= v6 && v6 >= 0 ) /*0x1802f6a78*/
      {
        v11 = 4; /*0x1802f6a7a*/
        goto LABEL_54; /*0x1802f6a80*/
      }
    }
    else
    {
      *((_BYTE *)a1 + 76) = 1; /*0x1802f6a44*/
      a1[20] = (v11 == 4) + 3; /*0x1802f6a50*/
    }
    if ( v11 != 4 ) /*0x1802f6a57*/
    {
      if ( dword_1813668E0 >= v6 && v6 > dword_1813668F8 ) /*0x1802f6a67*/
      {
        v10 = 1; /*0x1802f6a69*/
        goto LABEL_55; /*0x1802f6a6e*/
      }
      if ( dword_1813668FC > v6 && v6 >= dword_1813668E4 ) /*0x1802f6a90*/
      {
LABEL_55:
        a1[52] = v10; /*0x1802f6a94*/
        if ( (_BYTE)a2 == 0 ) /*0x1802f6a9c*/
          goto LABEL_86; /*0x1802f6a9c*/
        goto LABEL_56; /*0x1802f6a9c*/
      }
    }
LABEL_54:
    v10 = 0; /*0x1802f6a92*/
    goto LABEL_55; /*0x1802f6a92*/
  }
  v18 = *((_QWORD *)a1 + 12); /*0x1802f699e*/
  if ( v18 != 0 && (a1[26] <= 0 || v6 == v9) && *(_BYTE *)(v18 + 96) == 0 ) /*0x1802f69bc*/
  {
    v19 = *((_QWORD *)a1 + 1); /*0x1802f69c2*/
    if ( *(int *)(v18 + 80) <= 0 ) /*0x1802f69c9*/
      *(_BYTE *)(v19 + 96) = 1; /*0x1802f69cb*/
    *(_DWORD *)(v19 + 80) = a1[8]; /*0x1802f69d2*/
    if ( v6 <= 0 ) /*0x1802f69d7*/
    {
      if ( dword_1813668F0 >= v6 && v6 >= 0 ) /*0x1802f6a0a*/
        v11 = 4; /*0x1802f6a10*/
    }
    else if ( dword_1813668F0 >= v6 ) /*0x1802f69df*/
    {
      *((_BYTE *)a1 + 76) = 1; /*0x1802f69e9*/
      LOBYTE(v17) = v11 == 4; /*0x1802f69ed*/
      a1[20] = v17 + 3; /*0x1802f69f4*/
      goto LABEL_86; /*0x1802f69f7*/
    }
    goto LABEL_56; /*0x1802f69df*/
  }
LABEL_86:
  a1[30] = v11; /*0x1802f6d1f*/
  *((_BYTE *)a1 + 124) = a3; /*0x1802f6d34*/
  return v4; /*0x1802f6d30*/
}
