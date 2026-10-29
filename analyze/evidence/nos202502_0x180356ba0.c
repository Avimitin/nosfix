__int64 __fastcall sub_180356BA0(__int64 a1)
{
  __int64 v2; // rcx
  __int64 result; // rax
  __int64 v4; // rsi
  __int64 (__fastcall ***v5)(_QWORD); // rbx
  struct __crt_locale_pointers *(__fastcall *v6)(_LocaleUpdate *__hidden); // rdx
  __int64 (__fastcall ***v7)(_QWORD); // rax
  __int64 v8; // rbx
  __int64 v9; // rsi
  __int64 v10; // r14
  unsigned int v11; // r15d
  unsigned __int8 v12; // r12
  __int64 v13; // rsi
  _LocaleUpdate *v14; // rbx
  struct __crt_locale_pointers *(__fastcall *v15)(_LocaleUpdate *__hidden); // rdx
  _QWORD *v16; // rax
  _DWORD *v17; // rbx
  __int64 v18; // r14
  unsigned int v19; // esi
  __int64 v20; // rbx
  __int64 v21; // rbx
  __int64 (__fastcall ***v22)(_QWORD); // rsi
  __int64 v23; // rbx
  char v24; // si
  __int64 v25; // rsi
  __int64 v26; // rbx
  __int64 v27; // rbx
  __int128 v28; // [rsp+20h] [rbp-E0h]
  __int128 v29; // [rsp+38h] [rbp-C8h]

  v2 = *(_QWORD *)(a1 + 16); /*0x180356bb7*/
  *(_BYTE *)(v2 + 84) = 0; /*0x180356bbb*/
  *(_DWORD *)(v2 + 80) = 0; /*0x180356bbf*/
  result = sub_180355CD0(a1: v2); /*0x180356bc3*/
  if ( (_BYTE)result != 0 ) /*0x180356bca*/
  {
    while ( 1 ) /*0x180356c10*/
    {
      v4 = *(_QWORD *)(a1 + 16); /*0x180356c10*/
      v5 = nullptr; /*0x180356c14*/
      if ( (unsigned __int8)sub_180355CD0(a1: v4) != 0 ) /*0x180356c21*/
        v5 = *(__int64 (__fastcall ****)(_QWORD))(**(_QWORD **)(v4 + 8LL * *(unsigned __int8 *)(v4 + 84) + 8) /*0x180356c42*/
                                                + 24LL * *(unsigned int *)(v4 + 80)
                                                + 16);
      v6 = (struct __crt_locale_pointers *(__fastcall *)(_LocaleUpdate *__hidden))(*v5)[8]; /*0x180356c4a*/
      if ( v6 == _LocaleUpdate::GetLocaleT ) /*0x180356c51*/
        v7 = v5 + 1; /*0x180356c57*/
      else
        v7 = (__int64 (__fastcall ***)(_QWORD))v6(this: (_LocaleUpdate *)v5); /*0x180356f28*/
      if ( *v7 == (__int64 (__fastcall **)(_QWORD))&CProcess::CResources::`vftable' ) /*0x180356c61*/
        v8 = (__int64)(v7 + 1); /*0x180356c67*/
      else
        v8 = (**v7)(a1: v7); /*0x180356f34*/
      v9 = *(_QWORD *)(a1 + 16); /*0x180356c6b*/
      DWORD1(v28) = 0; /*0x180356c72*/
      if ( (unsigned __int8)sub_180355CD0(a1: v9) != 0 ) /*0x180356c7e*/
        v28 = *(_OWORD *)(**(_QWORD **)(v9 + 8LL * *(unsigned __int8 *)(v9 + 84) + 8) + 24LL /*0x180356c97*/
                                                                                      * *(unsigned int *)(v9 + 80));
      v10 = *(_QWORD *)(a1 + 16); /*0x180356c9c*/
      v11 = 0x7FFFFFFF; /*0x180356ca0*/
      v12 = *(_BYTE *)(v10 + 84); /*0x180356cb2*/
      if ( (unsigned __int8)sub_180355CD0(a1: v10) != 0 ) /*0x180356cbe*/
        v11 = *(_OWORD *)(**(_QWORD **)(v10 + 8LL * *(unsigned __int8 *)(v10 + 84) + 8) /*0x180356cde*/
                        + 24LL * *(unsigned int *)(v10 + 80));
      if ( *(void ***)v8 == &CProcessInfo::`vftable' ) /*0x180356cf0*/
      {
        *(_DWORD *)(v8 + 8) = v11; /*0x180356cf6*/
        *(_BYTE *)(v8 + 12) = v12; /*0x180356cfa*/
        *(_DWORD *)(v8 + 16) = DWORD1(v28); /*0x180356cfe*/
      }
      else
      {
        (*(void (__fastcall **)(__int64, _QWORD, _QWORD, _QWORD))(*(_QWORD *)v8 + 48LL))( /*0x180356f49*/
          a1: v8,
          a2: v11,
          a3: v12,
          a4: DWORD1(v28));
      }
      v13 = *(_QWORD *)(a1 + 16); /*0x180356d01*/
      v14 = nullptr; /*0x180356d05*/
      if ( (unsigned __int8)sub_180355CD0(a1: v13) != 0 ) /*0x180356d12*/
        v14 = *(_LocaleUpdate **)(**(_QWORD **)(v13 + 8LL * *(unsigned __int8 *)(v13 + 84) + 8) /*0x180356d32*/
                                + 24LL * *(unsigned int *)(v13 + 80)
                                + 16);
      v15 = *(struct __crt_locale_pointers *(__fastcall **)(_LocaleUpdate *__hidden))(*(_QWORD *)v14 + 64LL); /*0x180356d40*/
      if ( v15 == _LocaleUpdate::GetLocaleT ) /*0x180356d47*/
        v16 = (_QWORD *)((char *)v14 + 8); /*0x180356d4d*/
      else
        v16 = (_QWORD *)v15(this: v14); /*0x180356f54*/
      if ( (void **)*v16 == &CProcess::CResources::`vftable' ) /*0x180356d5e*/
        v17 = (_DWORD *)v16[6]; /*0x180356d64*/
      else
        v17 = (_DWORD *)(*(__int64 (__fastcall **)(_QWORD *))(*v16 + 16LL))(a1: v16); /*0x180356f61*/
      v18 = *(_QWORD *)(a1 + 16); /*0x180356d68*/
      v19 = 0x7FFFFFFF; /*0x180356d6c*/
      if ( (unsigned __int8)sub_180355CD0(a1: v18) != 0 ) /*0x180356d7b*/
        v19 = *(_OWORD *)(**(_QWORD **)(v18 + 8LL * *(unsigned __int8 *)(v18 + 84) + 8) /*0x180356d9a*/
                        + 24LL * *(unsigned int *)(v18 + 80));
      if ( *(void ***)v17 == &CProcessMail::`vftable' ) /*0x180356daa*/
        v17[6] = v19; /*0x180356db0*/
      else
        (*(void (__fastcall **)(_DWORD *, _QWORD))(*(_QWORD *)v17 + 32LL))(a1: v17, a2: v19); /*0x180356f1d*/
      v20 = *(_QWORD *)(a1 + 16); /*0x180356db3*/
      DWORD1(v29) = 0; /*0x180356dba*/
      if ( (unsigned __int8)sub_180355CD0(a1: v20) != 0 ) /*0x180356dc6*/
        v29 = *(_OWORD *)(**(_QWORD **)(v20 + 8LL * *(unsigned __int8 *)(v20 + 84) + 8) /*0x180356ddf*/
                        + 24LL * *(unsigned int *)(v20 + 80));
      if ( DWORD1(v29) == 1 ) /*0x180356df0*/
        break; /*0x180356df0*/
      if ( (unsigned int)(DWORD1(v29) - 2) <= 2 ) /*0x180356df8*/
        goto LABEL_32; /*0x180356df8*/
LABEL_38:
      v27 = *(_QWORD *)(a1 + 16); /*0x180356eba*/
      result = sub_180355CD0(a1: v27); /*0x180356ec1*/
      if ( (_BYTE)result != 0 ) /*0x180356ec8*/
      {
        ++*(_DWORD *)(v27 + 80); /*0x180356eca*/
        result = sub_180355CD0(a1: v27); /*0x180356ed0*/
        if ( (_BYTE)result != 0 ) /*0x180356ede*/
          continue; /*0x180356ede*/
      }
      return result; /*0x180356ede*/
    }
    v21 = *(_QWORD *)(a1 + 16); /*0x180356dff*/
    v22 = nullptr; /*0x180356e03*/
    if ( (unsigned __int8)sub_180355CD0(a1: v21) != 0 ) /*0x180356e10*/
      v22 = *(__int64 (__fastcall ****)(_QWORD))(**(_QWORD **)(v21 + 8LL * *(unsigned __int8 *)(v21 + 84) + 8) /*0x180356e30*/
                                               + 24LL * *(unsigned int *)(v21 + 80)
                                               + 16);
    result = (**v22)(a1: v22); /*0x180356e3a*/
    if ( (_BYTE)result == 0 ) /*0x180356e3e*/
      return result; /*0x180356e3e*/
LABEL_32:
    v23 = *(_QWORD *)(a1 + 16); /*0x180356e44*/
    v24 = 0; /*0x180356e48*/
    if ( (unsigned __int8)sub_180355CD0(a1: v23) != 0 ) /*0x180356e55*/
      v24 = *(_QWORD *)(**(_QWORD **)(v23 + 8LL * *(unsigned __int8 *)(v23 + 84) + 8) /*0x180356e72*/
                      + 24LL * *(unsigned int *)(v23 + 80)
                      + 8);
    if ( (v24 & 1) != 0 ) /*0x180356e7a*/
    {
      v25 = *(_QWORD *)(a1 + 16); /*0x180356e7c*/
      v26 = 0; /*0x180356e80*/
      if ( (unsigned __int8)sub_180355CD0(a1: v25) != 0 ) /*0x180356e8d*/
        v26 = *(_QWORD *)(**(_QWORD **)(v25 + 8LL * *(unsigned __int8 *)(v25 + 84) + 8) /*0x180356ead*/
                        + 24LL * *(unsigned int *)(v25 + 80)
                        + 16);
      (*(void (__fastcall **)(__int64))(*(_QWORD *)v26 + 16LL))(a1: v26); /*0x180356eb7*/
    }
    goto LABEL_38; /*0x180356eb7*/
  }
  return result; /*0x180356f0c*/
}
