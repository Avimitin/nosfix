__int64 __fastcall sub_180197770(__int64 a1)
{
  __int64 result; // rax
  __int64 v3; // rbx
  int v4; // eax
  __int64 v5; // rax
  char *v6; // rbx
  char *v7; // rsi
  char *v8; // rax
  __int64 v9; // rax
  void *v10; // rbx
  void (__fastcall ***v11)(_QWORD, __int64); // rcx
  void *v12; // rcx
  void (__fastcall ***v13)(_QWORD, __int64); // rcx
  void *v14; // rcx
  unsigned int v15; // esi
  __int64 *v16; // rbx
  __int64 v17; // rax
  unsigned int v18; // esi
  _QWORD *v19; // rbx
  unsigned int v20; // ebx
  void *Block; // [rsp+20h] [rbp-88h] BYREF
  char *v22; // [rsp+28h] [rbp-80h]
  __int64 v23; // [rsp+30h] [rbp-78h]
  __int64 v24; // [rsp+38h] [rbp-70h]
  _BYTE v25[32]; // [rsp+40h] [rbp-68h] BYREF
  char v26[32]; // [rsp+60h] [rbp-48h] BYREF

  v24 = -2; /*0x18019777e*/
  sub_180360060(); /*0x1801977a3*/
  off_18070B838(); /*0x1801977a8*/
  sub_1802BC5D0(a1: &xmmword_1813E4AD8); /*0x1801977b5*/
  sub_180360060(); /*0x1801977ba*/
  result = sub_180359940(); /*0x1801977bf*/
  if ( *(_BYTE *)(a1 + 1617) != 0 ) /*0x1801977cb*/
  {
    v3 = sub_180360060(); /*0x1801977d6*/
    if ( *(_DWORD *)(v3 + 940) != 0 ) /*0x1801977e1*/
    {
      off_18070B840(); /*0x1801977ea*/
      *(_DWORD *)(v3 + 940) = 0; /*0x1801977f0*/
    }
    v4 = dword_181B7D57C; /*0x1801977f6*/
    if ( (dword_181B7D57C & 1) == 0 ) /*0x1801977fe*/
    {
      __wind /*0xf1c000000000009c*/
      {
        dword_181B7D57C |= 1u; /*0x180197803*/
        sub_1801A3930(); /*0x180197809*/
        atexit(a1: sub_1803928D0); /*0x180197815*/
        v4 = dword_181B7D57C; /*0x18019781b*/
      }
      __unwind
      {
        dword_181B7D57C &= ~1u; /*0x180381582*/
      }
    }
    if ( (_BYTE)word_184ACC188 != 0 ) /*0x180197828*/
    {
      if ( (v4 & 1) == 0 ) /*0x180197830*/
      {
        __wind /*0xf1c0000000000084*/
        {
          dword_181B7D57C = v4 | 1; /*0x180197835*/
          sub_1801A3930(); /*0x18019783b*/
          atexit(a1: sub_1803928D0); /*0x180197847*/
        }
        __unwind
        {
          dword_181B7D57C &= ~1u; /*0x180381598*/
        }
      }
      sub_1801A3E80(a1: word_181B7D580, a2: &Block); /*0x180197859*/
      __wind /*0xf1c000000000006c*/
      {
        v5 = sub_180360060(); /*0x18019785f*/
        sub_18035AFB0(a1: v5 + 520, a2: &Block); /*0x180197870*/
      }
      __unwind
      {
        sub_18009DA80(a1: &Block); /*0x1803815a7*/
      }
      v6 = (char *)Block; /*0x180197876*/
      if ( Block != nullptr ) /*0x18019787e*/
      {
        v7 = v22; /*0x180197880*/
        if ( Block != v22 ) /*0x180197888*/
        {
          do /*0x18019789f*/
          {
            sub_1800773D0(a1: v6); /*0x180197893*/
            v6 += 32; /*0x180197898*/
          }
          while ( v6 != v7 ); /*0x18019789f*/
          v6 = (char *)Block; /*0x1801978a1*/
        }
        v8 = v6; /*0x1801978b2*/
        if ( ((v23 - (_QWORD)v6) & 0xFFFFFFFFFFFFFFE0uLL) >= 0x1000 ) /*0x1801978bc*/
        {
          v6 = *((char **)v6 - 1); /*0x1801978c2*/
          if ( (unsigned __int64)(v8 - v6 - 8) > 0x1F ) /*0x1801978d1*/
            invalid_parameter_noinfo_noreturn(); /*0x180197c16*/
        }
        j_j_free(Block: v6); /*0x1801978da*/
      }
    }
    dword_18070A724 = dword_18070A720; /*0x1801978e7*/
    dword_18070A610 = -200; /*0x1801978ef*/
    qword_18070A718 = 0; /*0x1801978f9*/
    dword_18070A728 = 1065353216; /*0x180197900*/
    dword_18070A72C = 1065353216; /*0x18019790a*/
    byte_18070A730 = 0; /*0x180197914*/
    HIDWORD(qword_18070A740) = 0; /*0x18019791b*/
    qword_18070A734 = 0; /*0x180197921*/
    dword_18070A73C = 0; /*0x18019792c*/
    *(__int64 *)((char *)&qword_18070A748 + 4) = 0; /*0x180197936*/
    v9 = sub_180300D50(); /*0x18019793d*/
    sub_180300E00(a1: v9); /*0x180197945*/
    sub_1803090D0(a1: *(_QWORD *)(a1 + 1600)); /*0x180197951*/
    sub_1802FBB40(a1: *(_QWORD *)(a1 + 1592)); /*0x18019795d*/
    v10 = *(void **)(a1 + 1600); /*0x180197962*/
    if ( v10 != nullptr ) /*0x18019796c*/
    {
      __wind /*0xf1c0000000000054*/
      {
        sub_1803090D0(a1: *(_QWORD *)(a1 + 1600)); /*0x180197971*/
      }
      __unwind
      {
        terminate(); /*0x180101fdc*/
      }
      j_j_free(Block: v10); /*0x18019797f*/
    }
    *(_QWORD *)(a1 + 1600) = 0; /*0x180197984*/
    v11 = *(void (__fastcall ****)(_QWORD, __int64))(a1 + 1608); /*0x18019798b*/
    if ( v11 != nullptr ) /*0x180197995*/
      (**v11)(a1: v11, a2: 1); /*0x18019799f*/
    *(_QWORD *)(a1 + 1608) = 0; /*0x1801979a1*/
    v12 = *(void **)(a1 + 1592); /*0x1801979a8*/
    if ( v12 != nullptr ) /*0x1801979b2*/
      sub_18009D880(Block: v12); /*0x1801979b4*/
    *(_QWORD *)(a1 + 1592) = 0; /*0x1801979b9*/
    j_j_free(Block: *(void **)(a1 + 1784)); /*0x1801979cc*/
    *(_QWORD *)(a1 + 1784) = 0; /*0x1801979d1*/
    v13 = *(void (__fastcall ****)(_QWORD, __int64))(a1 + 1768); /*0x1801979d8*/
    if ( v13 != nullptr ) /*0x1801979e2*/
      (**v13)(a1: v13, a2: 1); /*0x1801979ec*/
    *(_QWORD *)(a1 + 1768) = 0; /*0x1801979ee*/
    v14 = *(void **)(a1 + 1776); /*0x1801979f5*/
    if ( v14 != nullptr ) /*0x1801979ff*/
      sub_18009D9A0(Block: v14); /*0x180197a01*/
    *(_QWORD *)(a1 + 1776) = 0; /*0x180197a06*/
    if ( (dword_184AEF6C0 & 1) == 0 ) /*0x180197a15*/
    {
      dword_184AEF6C0 |= 1u; /*0x180197a1a*/
      sub_180102B80(a1: &unk_184AEF6D0, a2: 0, a3: 1984); /*0x180197a2f*/
      atexit(a1: sub_1803A4DF0); /*0x180197a3b*/
    }
    v15 = 0; /*0x180197a40*/
    v16 = (__int64 *)&unk_184AEF6D0; /*0x180197a42*/
    do /*0x180197a72*/
    {
      if ( v15 <= 0xE3 && *v16 != 0 ) /*0x180197a5e*/
      {
        (*(void (__fastcall **)(__int64))(*(_QWORD *)*v16 + 16LL))(a1: *v16); /*0x180197a63*/
        *v16 = 0; /*0x180197a66*/
      }
      ++v15; /*0x180197a69*/
      ++v16; /*0x180197a6b*/
    }
    while ( (__int64)v16 < (__int64)&qword_184AEFDF0 ); /*0x180197a72*/
    v17 = sub_180195910(a1, a2: v25); /*0x180197a7c*/
    __wind /*0xf1c000000000003c*/
    {
      sub_1803595C0(a1: v17); /*0x180197a85*/
    }
    __unwind
    {
      sub_18009D860(a1: v25); /*0x1803815b3*/
    }
    sub_1800773D0(a1: v26); /*0x180197a90*/
    sub_1800773D0(a1: v25); /*0x180197a9a*/
    *(_BYTE *)(a1 + 1617) = 0; /*0x180197a9f*/
    v18 = 0; /*0x180197aa6*/
    v19 = &unk_184AEF6D0; /*0x180197aa8*/
    do /*0x180197b08*/
    {
      if ( (dword_184AEF6C0 & 1) == 0 ) /*0x180197abd*/
      {
        dword_184AEF6C0 |= 1u; /*0x180197abf*/
        sub_180102B80(a1: &unk_184AEF6D0, a2: 0, a3: 1984); /*0x180197ad5*/
        atexit(a1: sub_1803A4DF0); /*0x180197ae1*/
      }
      if ( v18 <= 0xE3 && *v19 != 0 ) /*0x180197af4*/
      {
        (*(void (__fastcall **)(_QWORD))(*(_QWORD *)*v19 + 16LL))(a1: *v19); /*0x180197af9*/
        *v19 = 0; /*0x180197afc*/
      }
      ++v18; /*0x180197aff*/
      ++v19; /*0x180197b01*/
    }
    while ( (__int64)v19 <= (__int64)qword_184AEFD48 ); /*0x180197b08*/
    sub_1803369D0(a1: a1 + 1752); /*0x180197b11*/
    sub_1802B90D0(a1: 9); /*0x180197b1b*/
    sub_180308630(a1: &unk_1813E5750); /*0x180197b27*/
    if ( (dword_184ACE9C0 & 1) == 0 ) /*0x180197b34*/
    {
      dword_184ACE9C0 |= 1u; /*0x180197b39*/
      avs2_core_244(a1: &qword_184ACEA50, a2: 0, a3: 2048); /*0x180197b4e*/
      avs2_core_244(a1: &unk_184ACE9D8, a2: 0, a3: 120); /*0x180197b61*/
      dword_184ACE9D0 = 0; /*0x180197b67*/
      atexit(a1: sub_1803A0F00); /*0x180197b74*/
    }
    v20 = dword_18058B6C8[dword_1813E57D8]; /*0x180197b87*/
    sub_1802B90D0(a1: 239); /*0x180197b91*/
    sub_1802B90D0(a1: 240); /*0x180197b9d*/
    sub_1802B90D0(a1: v20); /*0x180197ba6*/
    sub_1802B90D0(a1: 242); /*0x180197bb2*/
    sub_1802B90D0(a1: 243); /*0x180197bbe*/
    sub_1802B90D0(a1: 244); /*0x180197bca*/
    sub_1802B90D0(a1: 259); /*0x180197bd6*/
    result = sub_1802B90D0(a1: 260); /*0x180197be2*/
    *(_BYTE *)(a1 + 1616) = 0; /*0x180197be7*/
  }
  return result; /*0x180197bee*/
}
