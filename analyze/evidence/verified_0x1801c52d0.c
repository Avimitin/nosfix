BOOL __fastcall sub_1801C52D0(__int64 a1, __int64 a2, __int64 a3, __int64 a4)
{
  __int64 v5; // rcx
  __int64 v6; // rax
  __int64 v7; // rcx
  CLibXact *Instance; // rax
  __int64 v9; // rdx
  __int64 v10; // rcx
  __int64 v11; // r8
  __int64 v12; // r9
  _QWORD *v13; // rbx
  __int64 v14; // rcx
  void (__fastcall ***v15)(_QWORD, __int64); // rcx
  __int64 v16; // rsi
  __int64 v17; // rcx
  __int64 v18; // rbx
  __int64 v19; // rdi
  __int64 v20; // rbx
  unsigned int v21; // r15d
  __int64 v22; // rbx
  char v23; // bp
  int v24; // r14d
  __int64 v25; // r8
  __int64 v26; // rax
  void (__fastcall ***v27)(_QWORD, __int64); // rcx
  __int64 v28; // rcx
  void **v29; // rdi
  _QWORD *v30; // rcx
  _QWORD *v31; // rbx
  void *v32; // rdi
  int i; // ebx
  __int64 v34; // rax
  __int64 v35; // rcx
  int *v36; // rcx
  int j; // ebx
  __int64 v38; // r8
  __int64 v39; // rdx
  _QWORD *k; // rbx
  _QWORD *v41; // rbx
  __int64 v42; // rcx
  __int64 v43; // rdi
  int v44; // eax
  int v45; // eax
  __int64 v46; // rcx
  __int64 v47; // rax
  __int64 v48; // rcx
  __int64 v49; // rax
  __int64 v50; // rcx
  __int64 v51; // rbx
  __int64 v52; // rcx
  int v53; // edi
  __int64 v54; // rbx
  __int64 v55; // rcx
  __int128 v57; // [rsp+28h] [rbp-70h]
  int *v58; // [rsp+A0h] [rbp+8h] BYREF

  v5 = qword_1811CEEB8; /*0x1801c52f7*/
  if ( qword_1811CEEB8 != 0 ) /*0x1801c5301*/
    (**(void (__fastcall ***)(__int64, __int64))qword_1811CEEB8)(a1: qword_1811CEEB8, a2: 1); /*0x1801c530b*/
  qword_1811CEEB8 = 0; /*0x1801c5310*/
  v6 = sub_180360060(a1: v5, a2, a3, a4); /*0x1801c5317*/
  sub_18035FE90(a1: v6); /*0x1801c531f*/
  Instance = (CLibXact *)CLibXact::getInstance(a1: v7); /*0x1801c5324*/
  CLibXact::ReleaseSound(this: Instance); /*0x1801c532d*/
  if ( *(_BYTE *)(a1 + 205) != 0 ) /*0x1801c533a*/
  {
    v13 = (_QWORD *)sub_1802C5260(a1: v10, a2: v9, a3: v11, a4: v12); /*0x1801c5345*/
    v13[3] = v13[2]; /*0x1801c534c*/
    v13[6] = v13[5]; /*0x1801c5354*/
    v14 = v13[10]; /*0x1801c5358*/
    if ( v14 != 0 ) /*0x1801c535f*/
    {
      (*(void (__fastcall **)(__int64))(*(_QWORD *)v14 + 16LL))(a1: v14); /*0x1801c5364*/
      v15 = (void (__fastcall ***)(_QWORD, __int64))v13[10]; /*0x1801c5367*/
      if ( v15 != nullptr ) /*0x1801c536e*/
        (**v15)(a1: v15, a2: 1); /*0x1801c5378*/
      v13[10] = 0; /*0x1801c537a*/
    }
    v16 = *(_QWORD *)(a1 + 176); /*0x1801c537e*/
    if ( v16 != 0 ) /*0x1801c5388*/
    {
      while ( 1 ) /*0x1801c5390*/
      {
        v17 = *(_QWORD *)(v16 + 16); /*0x1801c5390*/
        *(_BYTE *)(v17 + 84) = 0; /*0x1801c5394*/
        *(_DWORD *)(v17 + 80) = 0; /*0x1801c5398*/
        if ( sub_180355CD0(a1: v17) == 0 ) /*0x1801c53a3*/
          break; /*0x1801c53a3*/
        v18 = *(_QWORD *)(v16 + 16); /*0x1801c53a9*/
        v19 = 0; /*0x1801c53ad*/
        if ( sub_180355CD0(a1: v18) != 0 ) /*0x1801c53ba*/
          v19 = *(_QWORD *)(**(_QWORD **)(v18 + 8LL * *(unsigned __int8 *)(v18 + 84) + 8) /*0x1801c53db*/
                          + 24LL * *(unsigned int *)(v18 + 80)
                          + 16);
        (*(void (__fastcall **)(__int64))(*(_QWORD *)v19 + 8LL))(a1: v19); /*0x1801c53e6*/
        v20 = *(_QWORD *)(v16 + 16); /*0x1801c53e9*/
        v21 = 0x7FFFFFFF; /*0x1801c53ed*/
        if ( sub_180355CD0(a1: v20) != 0 ) /*0x1801c53fd*/
          v21 = *(_OWORD *)(**(_QWORD **)(v20 + 8LL * *(unsigned __int8 *)(v20 + 84) + 8) /*0x1801c541b*/
                          + 24LL * *(unsigned int *)(v20 + 80));
        v22 = *(_QWORD *)(v16 + 16); /*0x1801c5420*/
        v23 = *(_BYTE *)(v22 + 84); /*0x1801c5424*/
        v24 = *(_DWORD *)(v22 + 80); /*0x1801c5428*/
        if ( (unsigned __int8)sub_180356220(a1: v22, a2: v21) != 0 ) /*0x1801c5439*/
        {
          BYTE8(v57) = 0; /*0x1801c543b*/
          *(_QWORD *)&v57 = 0x7FFFFFFF; /*0x1801c5440*/
          if ( sub_180355CD0(a1: v22) != 0 ) /*0x1801c5458*/
          {
            v25 = 3LL * *(unsigned int *)(v22 + 80); /*0x1801c5466*/
            v26 = **(_QWORD **)(v22 + 8LL * *(unsigned __int8 *)(v22 + 84) + 8); /*0x1801c546a*/
            *(_OWORD *)(v26 + 8 * v25) = v57; /*0x1801c5472*/
            *(_QWORD *)(v26 + 8 * v25 + 16) = 0; /*0x1801c547d*/
          }
        }
        *(_DWORD *)(v22 + 80) = v24; /*0x1801c5484*/
        *(_BYTE *)(v22 + 84) = v23; /*0x1801c5488*/
        (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)v19 + 56LL))(a1: v19, a2: 1); /*0x1801c5497*/
      }
      v27 = *(void (__fastcall ****)(_QWORD, __int64))(a1 + 176); /*0x1801c549f*/
      if ( v27 != nullptr ) /*0x1801c54a9*/
        (**v27)(a1: v27, a2: 1); /*0x1801c54b3*/
      *(_QWORD *)(a1 + 176) = 0; /*0x1801c54b5*/
    }
    if ( *((_QWORD *)&xmmword_181365578 + 1) != 0 ) /*0x1801c54c6*/
      (***((void (__fastcall ****)(_QWORD, __int64))&xmmword_181365578 + 1))( /*0x1801c54d0*/
        a1: *((_QWORD *)&xmmword_181365578 + 1),
        a2: 1);
    *((_QWORD *)&xmmword_181365578 + 1) = 0; /*0x1801c54d2*/
    v28 = xmmword_181365578; /*0x1801c54d9*/
    if ( (_QWORD)xmmword_181365578 != 0 ) /*0x1801c54e3*/
      (**(void (__fastcall ***)(_QWORD, __int64))xmmword_181365578)(a1: xmmword_181365578, a2: 1); /*0x1801c54ed*/
    *(_QWORD *)&xmmword_181365578 = 0; /*0x1801c54ef*/
    byte_181365588 = 0; /*0x1801c54f6*/
    v29 = (void **)qword_1812D1EB8; /*0x1801c54fd*/
    if ( qword_1812D1EB8 != nullptr ) /*0x1801c5507*/
    {
      v30 = **(_QWORD ***)qword_1812D1EB8; /*0x1801c550c*/
      **(_QWORD **)qword_1812D1EB8 = *(_QWORD *)qword_1812D1EB8; /*0x1801c550f*/
      *((_QWORD *)*v29 + 1) = *v29; /*0x1801c5515*/
      v29[1] = nullptr; /*0x1801c5519*/
      if ( v30 != *v29 ) /*0x1801c5520*/
      {
        do /*0x1801c5535*/
        {
          v31 = (_QWORD *)*v30; /*0x1801c5522*/
          j_j_free(Block: v30); /*0x1801c552a*/
          v30 = v31; /*0x1801c552f*/
        }
        while ( v31 != *v29 ); /*0x1801c5535*/
      }
      j_j_free(Block: *v29); /*0x1801c553f*/
      j_j_free(Block: v29); /*0x1801c554c*/
    }
    qword_1812D1EB8 = nullptr; /*0x1801c5551*/
    v32 = qword_1812CF918; /*0x1801c5558*/
    if ( qword_1812CF918 != nullptr ) /*0x1801c5562*/
    {
      __eh34_enter_wind_state(-1, 0); /*0xf1c0000000000058*/
      for ( i = 0; i < 13; ++i ) /*0x1801c5564*/
      {
        v34 = sub_1800DA910(a1: v28); /*0x1801c5567*/
        sub_1800DBDC0(a1: v34, a2: (unsigned int)i); /*0x1801c5571*/
      }
      if ( __eh34_unwind(0) ) /*0xf1c000000000004c*/
unwind_state_0:
        terminate(); /*0xf1c000000000001c*/
      __eh34_exit_wind_state(0, -1); /*0xf1c0000000000050*/
      j_j_free(Block: v32); /*0x1801c5585*/
    }
    qword_1812CF918 = nullptr; /*0x1801c558a*/
    sub_18036A5E0(a1: v28); /*0x1801c5591*/
    sub_180311A00(a1: v35); /*0x1801c5596*/
    v36 = (int *)qword_1812D1E70; /*0x1801c559b*/
    if ( qword_1812D1E70 != 0 ) /*0x1801c55a5*/
    {
      for ( j = 0; j < 3; ++j ) /*0x1801c55ab*/
      {
        if ( v36 == nullptr ) /*0x1801c55b8*/
        {
          v36 = (int *)operator new(Size: 0x40u); /*0x1801c55c4*/
          v58 = v36; /*0x1801c55c7*/
          *(_QWORD *)v36 = &CDeviceDriver::`vftable'; /*0x1801c55cf*/
          *((_QWORD *)v36 + 1) = 0; /*0x1801c55d2*/
          *((_QWORD *)v36 + 2) = 0; /*0x1801c55d6*/
          *((_QWORD *)v36 + 3) = 0; /*0x1801c55da*/
          *((_QWORD *)v36 + 4) = 0; /*0x1801c55de*/
          *((_QWORD *)v36 + 5) = 0; /*0x1801c55e2*/
          *((_QWORD *)v36 + 6) = 0; /*0x1801c55e6*/
          v36[14] = 0; /*0x1801c55ea*/
          *((_WORD *)v36 + 30) = 0; /*0x1801c55ee*/
          qword_1812D1E70 = (__int64)v36; /*0x1801c55f4*/
        }
        if ( (unsigned int)j <= 2 ) /*0x1801c55fe*/
        {
          if ( j != 0 ) /*0x1801c560a*/
          {
            if ( j == 1 ) /*0x1801c560f*/
              v38 = 1; /*0x1801c561c*/
            else
              v38 = 3; /*0x1801c5616*/
          }
          else
          {
            v38 = 2; /*0x1801c5624*/
          }
          v39 = qword_1812D1E68; /*0x1801c562a*/
          *(_QWORD *)(qword_1812D1E68 + 16 * v38 + 2504) = 0; /*0x1801c5637*/
          *(_DWORD *)(v39 + 16 * (v38 + 157)) = 0; /*0x1801c5649*/
        }
      }
      if ( v36 != nullptr ) /*0x1801c565b*/
        (**(void (__fastcall ***)(void *, __int64))v36)(a1: v36, a2: 1); /*0x1801c5665*/
    }
    qword_1812D1E70 = 0; /*0x1801c5667*/
    if ( dword_1812D04D8 > 0 && (unsigned int)avs2_core_12(a1: (unsigned int)dword_1812D04D8, a2: 0) == 0 ) /*0x1801c5682*/
    {
      avs2_core_7(a1: (unsigned int)dword_1812D04D8); /*0x1801c568a*/
      dword_1812D04D8 = 0; /*0x1801c5690*/
    }
    v58 = &dword_1812D3F9C; /*0x1801c569e*/
    avs2_core_16(a1: (unsigned int)dword_1812D3F9C); /*0x1801c56ac*/
    __wind /*0xf1c0000000000040*/
    {
      for ( k = (_QWORD *)xmmword_181366898; k != *((_QWORD **)&xmmword_181366898 + 1); ++k ) /*0x1801c56c1*/
        (*(void (__fastcall **)(_QWORD))(*(_QWORD *)*k + 16LL))(a1: *k); /*0x1801c56c9*/
    }
    __unwind
    {
      sub_1800931C0(a1: &v58); /*0x180382f6d*/
    }
    avs2_core_17(a1: (unsigned int)dword_1812D3F9C); /*0x1801c56df*/
    avs2_core_16(a1: (unsigned int)dword_1812D3F9C); /*0x1801c56eb*/
    v41 = (_QWORD *)xmmword_181366898; /*0x1801c56f1*/
    if ( (_QWORD)xmmword_181366898 != *((_QWORD *)&xmmword_181366898 + 1) ) /*0x1801c56ff*/
    {
      do /*0x1801c5721*/
      {
        if ( *v41 != 0 ) /*0x1801c5707*/
          (**(void (__fastcall ***)(_QWORD, __int64))*v41)(a1: *v41, a2: 1); /*0x1801c5711*/
        *v41++ = 0; /*0x1801c5713*/
      }
      while ( v41 != *((_QWORD **)&xmmword_181366898 + 1) ); /*0x1801c5721*/
      v41 = (_QWORD *)xmmword_181366898; /*0x1801c5723*/
    }
    *((_QWORD *)&xmmword_181366898 + 1) = v41; /*0x1801c572a*/
    avs2_core_17(a1: (unsigned int)dword_1812D3F9C); /*0x1801c5737*/
    sub_18030EEE0(a1: &qword_1813E5A20); /*0x1801c5744*/
    if ( (dword_1813EE5F4 & 1) == 0 ) /*0x1801c5751*/
    {
      dword_1813EE5F4 |= 1u; /*0x1801c5756*/
      xmmword_1813EE5F8 = 0; /*0x1801c575f*/
      qword_1813EE608 = 0; /*0x1801c5767*/
      atexit(a1: sub_18038E7E0); /*0x1801c5775*/
    }
    v43 = sub_180078A80(a1: v42); /*0x1801c577f*/
    v44 = Mtx_lock(a1: (_Mtx_t)(v43 + 8)); /*0x1801c5786*/
    if ( v44 != 0 ) /*0x1801c578d*/
      std::_Throw_C_error(a1: v44); /*0x1801c5791*/
    sub_1800775A0(a1: *(_QWORD *)(v43 + 88), a2: *(_QWORD *)(v43 + 96)); /*0x1801c579e*/
    *(_QWORD *)(v43 + 96) = *(_QWORD *)(v43 + 88); /*0x1801c57a7*/
    __eh34_enter_wind_state(-1, 0); /*0xf1c000000000002c*/
    v45 = Mtx_unlock(a1: (_Mtx_t)(v43 + 8)); /*0x1801c57af*/
    if ( v45 != 0 ) /*0x1801c57b6*/
      std::_Throw_C_error(a1: v45); /*0x1801c57ba*/
    if ( __eh34_unwind(0) ) /*0xf1c0000000000018*/
      goto unwind_state_0; /*0xf1c0000000000018*/
    __eh34_exit_wind_state(0, -1); /*0xf1c0000000000024*/
    sub_1802B90D0(a1: 35); /*0x1801c57c5*/
    v47 = sub_1802BA000(a1: v46); /*0x1801c57ca*/
    sub_1802B9DF0(a1: v47); /*0x1801c57d2*/
    v49 = sub_1803383C0(a1: v48); /*0x1801c57d7*/
    sub_180338130(a1: v49); /*0x1801c57df*/
    sub_1800D6300(a1: v50); /*0x1801c57e4*/
    sub_1800D61E0(); /*0x1801c57e9*/
    v51 = qword_1813E9610; /*0x1801c57ee*/
    if ( qword_1813E9610 != 0 ) /*0x1801c57f8*/
    {
      *(_BYTE *)(qword_1813E9610 + 48) = 1; /*0x1801c57fa*/
      avs2_core_10(a1: *(unsigned int *)(v51 + 40)); /*0x1801c5801*/
      if ( *(_QWORD *)(v51 + 32) == 0 ) /*0x1801c580c*/
      {
        v52 = *(unsigned int *)(v51 + 40); /*0x1801c580e*/
        if ( (_DWORD)v52 != 0 ) /*0x1801c5813*/
        {
          v58 = nullptr; /*0x1801c5815*/
          if ( (unsigned int)avs2_core_12(a1: v52, a2: &v58) == 0 ) /*0x1801c582d*/
          {
            avs2_core_7(a1: *(unsigned int *)(v51 + 40)); /*0x1801c5832*/
            *(_DWORD *)(v51 + 40) = 0; /*0x1801c5838*/
          }
          avs2_core_15(a1: *(unsigned int *)(v51 + 44)); /*0x1801c583f*/
        }
      }
    }
  }
  if ( byte_1806FC344 != 0 ) /*0x1801c584c*/
  {
    v53 = 0; /*0x1801c5852*/
    if ( dword_1811CEE98 > 0 ) /*0x1801c585c*/
    {
      v54 = 0; /*0x1801c585e*/
      do /*0x1801c58aa*/
      {
        if ( v54 != 0 ) /*0x1801c5864*/
          RemovePropA(hWnd: *((HWND *)qword_1811CED10 + v54), lpString: "PropClassWindowProc"); /*0x1801c5878*/
        v55 = *((_QWORD *)qword_1811CED00 + v54); /*0x1801c5885*/
        if ( v55 != 0 ) /*0x1801c588c*/
        {
          (*(void (__fastcall **)(__int64))(*(_QWORD *)v55 + 16LL))(a1: v55); /*0x1801c5891*/
          *((_QWORD *)qword_1811CED00 + v54) = 0; /*0x1801c589b*/
        }
        ++v53; /*0x1801c589f*/
        ++v54; /*0x1801c58a1*/
      }
      while ( v53 < dword_1811CEE98 ); /*0x1801c58aa*/
    }
    j_j_free(Block: qword_1811CED08); /*0x1801c58b3*/
    qword_1811CED08 = nullptr; /*0x1801c58b8*/
    j_j_free(Block: qword_1811CED10); /*0x1801c58c6*/
    qword_1811CED10 = nullptr; /*0x1801c58cb*/
    j_j_free(Block: qword_1811CED00); /*0x1801c58d9*/
    qword_1811CED00 = nullptr; /*0x1801c58de*/
  }
  if ( qword_1811CED18 != 0 ) /*0x1801c58ef*/
    (**(void (__fastcall ***)(__int64, __int64))qword_1811CED18)(a1: qword_1811CED18, a2: 1); /*0x1801c58f9*/
  qword_1811CED18 = 0; /*0x1801c58fb*/
  if ( qword_1811CEAF8 != 0 ) /*0x1801c590c*/
  {
    (*(void (__fastcall **)(__int64))(*(_QWORD *)qword_1811CEAF8 + 16LL))(a1: qword_1811CEAF8); /*0x1801c5911*/
    qword_1811CEAF8 = 0; /*0x1801c5914*/
  }
  if ( qword_1811CEE90 != 0 ) /*0x1801c5925*/
  {
    (*(void (__fastcall **)(__int64))(*(_QWORD *)qword_1811CEE90 + 16LL))(a1: qword_1811CEE90); /*0x1801c592a*/
    qword_1811CEE90 = 0; /*0x1801c592d*/
  }
  RemovePropA(hWnd: hwnd, lpString: "PropClassWindowProc"); /*0x1801c5942*/
  return UnregisterClassA(lpClassName: (LPCSTR)(a1 + 104), hInstance: *(HINSTANCE *)(a1 + 48)); /*0x1801c5950*/
}
