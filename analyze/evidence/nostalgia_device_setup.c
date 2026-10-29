char __fastcall sub_1801C6510(__int64 a1, __int64 a2, unsigned __int8 a3)
{
  IDirect3D9 *v6; // rax
  HWND v7; // rdx
  __int64 v8; // r8
  _QWORD *v9; // rax
  int v10; // ebx
  int v11; // ecx
  __int64 v12; // rdi
  __int64 v13; // rax
  int *v14; // r14
  char *v15; // rax
  int v16; // ebx
  __int64 v17; // rdi
  __int64 v18; // rsi
  bool v19; // cc
  int v20; // r15d
  unsigned int i; // edi
  int v22; // esi
  unsigned int j; // ebx
  int v24; // ebx
  void *v25; // r8
  int v26; // ebx
  __int64 v27; // rdi
  char v28; // di
  _QWORD *v29; // rbx
  int v30; // edx
  int v31; // ecx
  int v32; // r8d
  int v33; // r9d
  int v34; // edx
  int v35; // r8d
  int v36; // r9d
  int v37; // ebx
  _QWORD *v39; // [rsp+40h] [rbp-278h] BYREF
  int v40; // [rsp+48h] [rbp-270h]
  __int64 v41; // [rsp+50h] [rbp-268h] BYREF
  int v42; // [rsp+58h] [rbp-260h]
  int v43; // [rsp+5Ch] [rbp-25Ch]
  int v44; // [rsp+60h] [rbp-258h]
  int v45; // [rsp+64h] [rbp-254h]
  wchar_t Buffer[128]; // [rsp+70h] [rbp-248h] BYREF
  wchar_t v47[128]; // [rsp+170h] [rbp-148h] BYREF

  v6 = Direct3DCreate9(SDKVersion: 0x20u); /*0x1801c653e*/
  qword_1811CEE90 = (__int64)v6; /*0x1801c6544*/
  if ( v6 != nullptr )
  {
    ((void (__fastcall *)(IDirect3D9 *, _QWORD, __int64, void *))v6->lpVtbl->GetDeviceCaps)( /*0x1801c6583*/
      a1: v6,
      a2: 0,
      a3: 1,
      a4: &unk_1811CED60);
    if ( dword_1811CEAF4 != 0 ) /*0x1801c6590*/
    {
      dword_1811CEE98 = 2; /*0x1801c65bc*/
    }
    else if ( byte_1806FC344 != 0 ) /*0x1801c6599*/
    {
      dword_1811CEE98 = dword_1811CEE48; /*0x1801c65a1*/
      if ( dword_1811CEE48 <= 1 ) /*0x1801c65aa*/
      {
        byte_1806FC344 = 0; /*0x1801c65ac*/
        dword_1811CEE98 = 0; /*0x1801c65b3*/
      }
    }
    hwnd = (HWND)sub_1802F3DD0(a1: a1 + 16, a2, a3: "nostalgia"); /*0x1801c6602*/
    v7 = hwnd; /*0x1801c6609*/
    if ( hwnd == nullptr ) /*0x1801c660f*/
      goto LABEL_32; /*0x1801c660f*/
    v8 = -1; /*0x1801c661c*/
    if ( byte_1806FC344 != 0 ) /*0x1801c6623*/
    {
      v9 = operator new(Size: saturated_mul(dword_1811CEE98, 8u)); /*0x1801c663f*/
      v7 = hwnd; /*0x1801c6644*/
      v10 = 1; /*0x1801c664b*/
      v11 = dword_1811CEE98; /*0x1801c6650*/
      qword_1811CED10 = v9; /*0x1801c6656*/
      *v9 = hwnd; /*0x1801c665d*/
      if ( v11 > 1 ) /*0x1801c6662*/
      {
        v12 = 8; /*0x1801c6664*/
        while ( 1 ) /*0x1801c6684*/
        {
          swprintf(Buffer, BufferCount: 0x100u, Format: "nostalgia(sub%d)", v10); /*0x1801c6684*/
          v13 = sub_1802F3DD0(a1: a1 + 16, a2, a3: Buffer); /*0x1801c669f*/
          *(_QWORD *)((char *)qword_1811CED10 + v12) = v13; /*0x1801c66ab*/
          if ( v13 == 0 ) /*0x1801c66b2*/
            goto LABEL_32; /*0x1801c66b2*/
          v11 = dword_1811CEE98; /*0x1801c66b8*/
          ++v10; /*0x1801c66be*/
          v12 += 8; /*0x1801c66c0*/
          if ( v10 >= dword_1811CEE98 ) /*0x1801c66c6*/
          {
            v7 = hwnd; /*0x1801c66c8*/
            break; /*0x1801c66c8*/
          }
        }
      }
      v8 = -1; /*0x1801c66cf*/
    }
    else
    {
      v11 = dword_1811CEE98; /*0x1801c66d8*/
    }
    v14 = &dword_1811CED20; /*0x1801c66e2*/
    qword_1811CED30 = 0; /*0x1801c66ec*/
    qword_1811CED38 = 1; /*0x1801c66fa*/
    qword_1811CED58 = 0; /*0x1801c6705*/
    dword_1811CED48 = a3 ^ 1; /*0x1801c670c*/
    dword_1811CED2C = 2; /*0x1801c6712*/
    dword_1811CED20 = 1280; /*0x1801c671c*/
    dword_1811CED24 = 720; /*0x1801c6726*/
    dword_1811CED28 = 21; /*0x1801c6730*/
    dword_1811CED4C = 1; /*0x1801c673a*/
    qword_1811CED50 = 75; /*0x1801c6744*/
    qword_1811CED40 = (__int64)v7; /*0x1801c674f*/
    if ( byte_1806FC344 != 0 ) /*0x1801c6756*/
    {
      v15 = (char *)operator new(Size: saturated_mul(v11, 0x40u)); /*0x1801c676a*/
      v16 = 0; /*0x1801c6776*/
      qword_1811CED08 = v15; /*0x1801c6779*/
      if ( dword_1811CEE98 > 0 ) /*0x1801c6780*/
      {
        v17 = 0; /*0x1801c6782*/
        v18 = 0; /*0x1801c6785*/
        do /*0x1801c67d1*/
        {
          avs2_core_240(a1: &v15[64 * (__int64)v16], a2: &dword_1811CED20, a3: 64); /*0x1801c67a3*/
          v17 += 64; /*0x1801c67b0*/
          ++v16; /*0x1801c67b4*/
          v18 += 8; /*0x1801c67b6*/
          v19 = v16 < dword_1811CEE98; /*0x1801c67ba*/
          v15 = (char *)qword_1811CED08; /*0x1801c67c5*/
          *(_QWORD *)((char *)qword_1811CED08 + v17 - 32) = *(_QWORD *)((char *)qword_1811CED10 + v18 - 8); /*0x1801c67cc*/
        }
        while ( v19 ); /*0x1801c67d1*/
      }
    }
    HIDWORD(qword_1811CED58) = 1; /*0x1801c67d7*/
    LODWORD(qword_1811CED58) = a3 != 0 ? 0x3C : 0;
    v20 = (*(__int64 (__fastcall **)(__int64, HWND, __int64))(*(_QWORD *)qword_1811CEE90 + 32LL))( /*0x1801c67fb*/
            a1: qword_1811CEE90,
            a2: v7,
            a3: v8);
    for ( i = 0; (int)i < v20; ++i )
    {
      v22 = (*(__int64 (__fastcall **)(__int64, _QWORD, __int64))(*(_QWORD *)qword_1811CEE90 + 48LL))( /*0x1801c6841*/
              a1: qword_1811CEE90,
              a2: i,
              a3: 22);
      swprintf(Buffer: v47, BufferCount: 0x100u, Format: "D3DFMT_A8R8G8B8 mode cnt: %d : %d\n", i, v22);
      for ( j = 0; (int)j < v22; ++j )
      {
        (*(void (__fastcall **)(__int64, _QWORD, __int64, _QWORD, _QWORD **))(*(_QWORD *)qword_1811CEE90 + 56LL))( /*0x1801c686d*/
          a1: qword_1811CEE90,
          a2: 0,
          a3: 22,
          a4: j,
          a5: &v39);
        swprintf(Buffer: v47, BufferCount: 0x100u, Format: "%d : %d: %d*%d(%d)\n", i, j, (_DWORD)v39, HIDWORD(v39), v40);
      }
    }
    v24 = 132; /*0x1801c68c1*/
    if ( byte_1806FC344 != 0 && a3 != 0 ) /*0x1801c68cb*/
    {
      v14 = (int *)qword_1811CED08; /*0x1801c68cd*/
      v24 = 644; /*0x1801c68d4*/
    }
    if ( (*(int (__fastcall **)(__int64, _QWORD, __int64, HWND, int, int *, __int64 *))(*(_QWORD *)qword_1811CEE90 /*0x1801c6973*/
                                                                                      + 128LL))(
           a1: qword_1811CEE90,
           a2: 0,
           a3: 1,
           a4: hwnd,
           a5: v24,
           a6: v14,
           a7: &qword_1811CEAF8) < 0
      && (*(int (__fastcall **)(__int64, _QWORD, __int64, HWND, int, int *, __int64 *))(*(_QWORD *)qword_1811CEE90
                                                                                      + 128LL))(
           a1: qword_1811CEE90,
           a2: 0,
           a3: 1,
           a4: hwnd,
           a5: v24,
           a6: v14,
           a7: &qword_1811CEAF8) < 0
      && (*(int (__fastcall **)(__int64, _QWORD, __int64, HWND, int, int *, __int64 *))(*(_QWORD *)qword_1811CEE90
                                                                                      + 128LL))(
           a1: qword_1811CEE90,
           a2: 0,
           a3: 2,
           a4: hwnd,
           a5: v24,
           a6: v14,
           a7: &qword_1811CEAF8) < 0 )
    {
      *(_DWORD *)(a1 + 200) = 2; /*0x1801c6975*/
LABEL_32:
      LOBYTE(v6) = 0; /*0x1801c6980*/
      return (char)v6; /*0x1801c69c6*/
    }
    if ( byte_1806FC344 != 0 ) /*0x1801c69ce*/
    {
      qword_1811CED00 = operator new(Size: saturated_mul(dword_1811CEE98, 8u)); /*0x1801c69f6*/
      v25 = qword_1811CED00; /*0x1801c69fd*/
      if ( a3 != 0 ) /*0x1801c6a03*/
      {
        v26 = 0; /*0x1801c6a10*/
        if ( dword_1811CEE98 > 0 ) /*0x1801c6a13*/
        {
          v27 = 0; /*0x1801c6a15*/
          while ( 1 ) /*0x1801c6a2c*/
          {
            (*(void (__fastcall **)(__int64, _QWORD, __int64))(*(_QWORD *)qword_1811CEAF8 + 112LL))( /*0x1801c6a33*/
              a1: qword_1811CEAF8,
              a2: (unsigned int)v26,
              a3: (__int64)v25 + 8 * v26);
            v25 = qword_1811CED00; /*0x1801c6a37*/
            if ( *(_QWORD *)((char *)qword_1811CED00 + v27) == 0 ) /*0x1801c6a42*/
              break; /*0x1801c6a42*/
            ++v26; /*0x1801c6a48*/
            v27 += 8; /*0x1801c6a4a*/
            if ( v26 >= dword_1811CEE98 ) /*0x1801c6a54*/
              goto LABEL_39; /*0x1801c6a54*/
          }
LABEL_44:
          *(_DWORD *)(a1 + 200) = 3; /*0x1801c6d9a*/
          goto LABEL_32; /*0x1801c6da5*/
        }
      }
      else
      {
        (*(void (__fastcall **)(__int64, _QWORD, void *))(*(_QWORD *)qword_1811CEAF8 + 112LL))( /*0x1801c6d46*/
          a1: qword_1811CEAF8,
          a2: 0,
          a3: qword_1811CED00);
        v37 = 1; /*0x1801c6d49*/
        if ( dword_1811CEE98 > 1 ) /*0x1801c6d54*/
        {
          while ( (*(int (__fastcall **)(__int64, char *, char *))(*(_QWORD *)qword_1811CEAF8 + 104LL))( /*0x1801c6d89*/
                    a1: qword_1811CEAF8,
                    a2: (char *)qword_1811CED08 + 64 * (__int64)v37,
                    a3: (char *)qword_1811CED00 + 8 * v37) >= 0 )
          {
            if ( ++v37 >= dword_1811CEE98 ) /*0x1801c6d93*/
              goto LABEL_39; /*0x1801c6d93*/
          }
          goto LABEL_44; /*0x1801c6d89*/
        }
      }
    }
LABEL_39:
    (*(void (__fastcall **)(__int64, __int64, __int64))(*(_QWORD *)qword_1811CEAF8 + 456LL))( /*0x1801c6a56*/
      a1: qword_1811CEAF8,
      a2: 22,
      a3: 1);
    (*(void (__fastcall **)(__int64, __int64, _QWORD))(*(_QWORD *)qword_1811CEAF8 + 456LL))( /*0x1801c6a81*/
      a1: qword_1811CEAF8,
      a2: 137,
      a3: 0);
    (*(void (__fastcall **)(__int64, __int64, __int64))(*(_QWORD *)qword_1811CEAF8 + 456LL))( /*0x1801c6a9a*/
      a1: qword_1811CEAF8,
      a2: 27,
      a3: 1);
    (*(void (__fastcall **)(__int64, __int64, __int64))(*(_QWORD *)qword_1811CEAF8 + 456LL))( /*0x1801c6ab3*/
      a1: qword_1811CEAF8,
      a2: 19,
      a3: 5);
    (*(void (__fastcall **)(__int64, __int64, __int64))(*(_QWORD *)qword_1811CEAF8 + 456LL))( /*0x1801c6acc*/
      a1: qword_1811CEAF8,
      a2: 20,
      a3: 6);
    (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)qword_1811CEAF8 + 456LL))(a1: qword_1811CEAF8, a2: 7); /*0x1801c6ae3*/
    (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)qword_1811CEAF8 + 456LL))(a1: qword_1811CEAF8, a2: 14); /*0x1801c6afa*/
    (*(void (__fastcall **)(__int64, __int64, __int64))(*(_QWORD *)qword_1811CEAF8 + 456LL))( /*0x1801c6b13*/
      a1: qword_1811CEAF8,
      a2: 52,
      a3: 1);
    (*(void (__fastcall **)(__int64, _QWORD, _QWORD, __int64, int, int, _DWORD))(*(_QWORD *)qword_1811CEAF8 + 344LL))( /*0x1801c6b49*/
      a1: qword_1811CEAF8,
      a2: 0,
      a3: 0,
      a4: 3,
      a5: -16777216,
      a6: 1065353216,
      a7: 0);
    (*(void (__fastcall **)(__int64))(*(_QWORD *)qword_1811CEAF8 + 328LL))(a1: qword_1811CEAF8); /*0x1801c6b59*/
    (*(void (__fastcall **)(__int64))(*(_QWORD *)qword_1811CEAF8 + 336LL))(a1: qword_1811CEAF8); /*0x1801c6b69*/
    (*(void (__fastcall **)(__int64, _QWORD, _QWORD, _QWORD, _QWORD))(*(_QWORD *)qword_1811CEAF8 + 136LL))( /*0x1801c6b86*/
      a1: qword_1811CEAF8,
      a2: 0,
      a3: 0,
      a4: 0,
      a5: 0);
    v41 = 0; /*0x1801c6b9b*/
    v44 = 0; /*0x1801c6ba0*/
    v42 = 1280; /*0x1801c6ba6*/
    v43 = 720; /*0x1801c6bae*/
    v45 = 1065353216; /*0x1801c6bb6*/
    (*(void (__fastcall **)(__int64, __int64 *))(*(_QWORD *)qword_1811CEAF8 + 376LL))(a1: qword_1811CEAF8, a2: &v41); /*0x1801c6bc1*/
    v28 = dword_1812D1F44 == 2; /*0x1801c6bd3*/
    v29 = operator new(Size: 0x1B0u); /*0x1801c6bdc*/
    v39 = v29; /*0x1801c6bdf*/
    *v29 = &CRenderingTexture::`vftable'; /*0x1801c6beb*/
    v29[3] = 0; /*0x1801c6bee*/
    v29[5] = 0; /*0x1801c6bf9*/
    v29[6] = 0; /*0x1801c6bff*/
    v29[4] = 0; /*0x1801c6c09*/
    v29[1] = 0; /*0x1801c6c0d*/
    v29[2] = 0; /*0x1801c6c11*/
    *(_QWORD *)((char *)v29 + 146) = 0; /*0x1801c6c15*/
    sub_180102B80(a1: v29 + 24, a2: 0, a3: 192); /*0x1801c6c1c*/
    *((_DWORD *)v29 + 14) = 1106247680; /*0x1801c6c21*/
    *((_DWORD *)v29 + 15) = 1106247680; /*0x1801c6c28*/
    *((_DWORD *)v29 + 16) = 1115684864; /*0x1801c6c2f*/
    *((_DWORD *)v29 + 17) = 1115684864; /*0x1801c6c36*/
    *((_DWORD *)v29 + 18) = 1065353216; /*0x1801c6c3d*/
    *(_QWORD *)((char *)v29 + 76) = 1065353216; /*0x1801c6c44*/
    *((_DWORD *)v29 + 21) = 1112014848; /*0x1801c6c4c*/
    v29[11] = 1112014848; /*0x1801c6c53*/
    *((_DWORD *)v29 + 24) = 0; /*0x1801c6c5b*/
    *((_DWORD *)v29 + 25) = -1; /*0x1801c6c5f*/
    v29[13] = 1128267776; /*0x1801c6c66*/
    *((_DWORD *)v29 + 28) = -1; /*0x1801c6c6e*/
    *((_DWORD *)v29 + 29) = 0; /*0x1801c6c75*/
    *((_DWORD *)v29 + 30) = 1125122048; /*0x1801c6c79*/
    *((_DWORD *)v29 + 31) = -1; /*0x1801c6c80*/
    *((_DWORD *)v29 + 32) = 1128267776; /*0x1801c6c87*/
    *((_DWORD *)v29 + 33) = 1125122048; /*0x1801c6c91*/
    *((_DWORD *)v29 + 34) = -1; /*0x1801c6c9b*/
    *((_DWORD *)v29 + 35) = 0; /*0x1801c6ca5*/
    *((_BYTE *)v29 + 144) = 0; /*0x1801c6cac*/
    v29[20] = 0; /*0x1801c6cb3*/
    v29[21] = 0; /*0x1801c6cba*/
    v29[22] = 1; /*0x1801c6cc1*/
    *((_DWORD *)v29 + 46) = 0; /*0x1801c6ccc*/
    *((_DWORD *)v29 + 47) = 1; /*0x1801c6cd3*/
    v29[48] = 0; /*0x1801c6cdd*/
    v29[49] = 0; /*0x1801c6ce4*/
    v29[50] = 0; /*0x1801c6ceb*/
    v29[51] = 0; /*0x1801c6cf2*/
    v29[52] = 0; /*0x1801c6cf9*/
    *((_DWORD *)v29 + 106) = -16777216; /*0x1801c6d00*/
    *((_BYTE *)v29 + 428) = 0; /*0x1801c6d0a*/
    qword_1811CED18 = (__int64)v29; /*0x1801c6d11*/
    sub_180336FD0(a1: v31, a2: v30, a3: v32, a4: v33, a5: v28); /*0x1801c6d1d*/
    sub_1803357A0(a1: (unsigned int)&off_18070B2C0, a2: v34, a3: v35, a4: v36, a5: v28); /*0x1801c6d2e*/
    LOBYTE(v6) = 1; /*0x1801c6d33*/
    return (char)v6; /*0x1801c6d35*/
  }
  *(_DWORD *)(a1 + 200) = 1; /*0x1801c6553*/
  return (char)v6; /*0x1801c69aa*/
}
