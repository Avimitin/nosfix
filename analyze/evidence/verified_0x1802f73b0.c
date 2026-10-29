__int64 __fastcall sub_1802F73B0(__int64 a1, _DWORD *a2)
{
  int v3; // r8d
  int v5; // ecx
  int v6; // edi
  __int64 v7; // rdx
  int v8; // eax
  unsigned int *v9; // rdx
  int v10; // eax
  __int64 v11; // rcx
  int v12; // eax
  int v13; // r8d
  int v14; // ecx
  int v15; // r8d
  __int64 v16; // rdi
  char v17; // al
  __int64 v18; // rcx
  __int64 result; // rax
  int v20; // [rsp+90h] [rbp+8h] BYREF
  int v21; // [rsp+98h] [rbp+10h] BYREF

  *(_WORD *)(a1 + 16) = 1; /*0x1802f73c3*/
  *(_DWORD *)(a1 + 20) = a2[4]; /*0x1802f73d5*/
  v3 = a2[5]; /*0x1802f73db*/
  *(_QWORD *)(a1 + 8) = a2; /*0x1802f73e2*/
  *(_DWORD *)(a1 + 24) = v3; /*0x1802f73e6*/
  *(_BYTE *)(a1 + 68) = 0; /*0x1802f73ea*/
  *(_BYTE *)(a1 + 76) = 0; /*0x1802f73ee*/
  *(_QWORD *)(a1 + 80) = 0; /*0x1802f73f2*/
  *(_QWORD *)(a1 + 96) = 0; /*0x1802f73f6*/
  *(_DWORD *)(a1 + 104) = 0; /*0x1802f73fa*/
  *(_BYTE *)(a1 + 108) = 0; /*0x1802f73fd*/
  *(_DWORD *)(a1 + 112) = -1; /*0x1802f7401*/
  *(_BYTE *)(a1 + 116) = 0; /*0x1802f7408*/
  v5 = a2[22]; /*0x1802f740c*/
  *(_DWORD *)(a1 + 44) = v5; /*0x1802f740f*/
  v6 = a2[23]; /*0x1802f7412*/
  *(_DWORD *)(a1 + 48) = v6; /*0x1802f7415*/
  *(_DWORD *)(a1 + 88) = -1; /*0x1802f7418*/
  v7 = (unsigned int)((v6 + v5) >> 31); /*0x1802f742b*/
  v21 = 0; /*0x1802f7432*/
  *(_DWORD *)(a1 + 52) = (v6 + v5) / 2; /*0x1802f7438*/
  v20 = 0; /*0x1802f743e*/
  *(float *)(a1 + 56) = (float)((float)v5 + (float)v6) * 0.5; /*0x1802f7451*/
  if ( *(void ***)a1 == &NormalNotes::`vftable' /*0x1802f7634*/
    || (v6 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)a1 + 72LL))(a1), *(void ***)a1 == &NormalNotes::`vftable') )
  {
    v8 = *(_DWORD *)(a1 + 44); /*0x1802f7495*/
  }
  else
  {
    v8 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)a1 + 64LL))(a1); /*0x1802f761e*/
  }
  sub_180305430( /*0x1802f74dc*/
    a1: *(_DWORD *)(a1 + 20),
    a2: v7,
    a3: (int *)(a1 + 28),
    a4: (int *)(a1 + 32),
    a5: nullptr,
    a6: nullptr,
    a7: nullptr,
    a8: nullptr,
    a9: v8,
    a10: v6,
    a11: &v21,
    a12: &v20);
  v9 = *(unsigned int **)(a1 + 8); /*0x1802f74e1*/
  *(_DWORD *)(a1 + 60) = v20 - v21; /*0x1802f74f3*/
  *(_DWORD *)(a1 + 64) = v9[23] - v9[22]; /*0x1802f74fc*/
  v10 = a2[25]; /*0x1802f74ff*/
  *(_DWORD *)(a1 + 88) = v10; /*0x1802f7502*/
  if ( HIDWORD(qword_1813E4730) == 1 ) /*0x1802f750c*/
  {
    if ( v10 == 1 ) /*0x1802f7515*/
    {
      v11 = 6; /*0x1802f7517*/
    }
    else
    {
      v11 = 0; /*0x1802f751f*/
      if ( v10 == 3 ) /*0x1802f7527*/
        v11 = 7; /*0x1802f7527*/
    }
  }
  else
  {
    v11 = 0; /*0x1802f750e*/
  }
  v12 = sub_180301CB0(a1: v11, a2: v9[12]); /*0x1802f752e*/
  *(_DWORD *)(a1 + 168) = v12; /*0x1802f7533*/
  v13 = *(_DWORD *)(a1 + 64); /*0x1802f7540*/
  v14 = dword_184AEF6C0; /*0x1802f7553*/
  if ( v12 + 9 < v13 + v12 ) /*0x1802f7559*/
    v13 = 9; /*0x1802f7559*/
  v15 = v12 + v13; /*0x1802f755d*/
  if ( v15 > v12 ) /*0x1802f7563*/
    v12 = v15; /*0x1802f7563*/
  *(_DWORD *)(a1 + 168) = v12; /*0x1802f7567*/
  if ( (v14 & 1) == 0 ) /*0x1802f7570*/
  {
    dword_184AEF6C0 = v14 | 1; /*0x1802f7577*/
    sub_180102B80(a1: qword_184AEF6D0, a2: 0, a3: 1984); /*0x1802f7586*/
    atexit(a1: sub_1803A4DF0); /*0x1802f7592*/
    v12 = *(_DWORD *)(a1 + 168); /*0x1802f7597*/
  }
  (*(void (__fastcall **)(_QWORD, _QWORD, __int64))(*(_QWORD *)qword_184AEF6D0[v12] + 136LL))( /*0x1802f75af*/
    a1: qword_184AEF6D0[v12],
    a2: 0,
    a3: a1 + 172);
  v16 = a1 + 128; /*0x1802f75b5*/
  v17 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)(a1 + 128) + 152LL))(a1: a1 + 128); /*0x1802f75c2*/
  v18 = a1 + 128; /*0x1802f75c8*/
  if ( v17 != 0 ) /*0x1802f75cd*/
  {
    result = sub_1800D2A70(a1: v18); /*0x1802f75cf*/
  }
  else
  {
    (*(void (__fastcall **)(__int64, __int64, __int64))(*(_QWORD *)v16 + 8LL))(a1: v18, a2: 57, a3: 1); /*0x1802f75e2*/
    result = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)v16 + 144LL))(a1: a1 + 128, a2: 1); /*0x1802f75f0*/
  }
  *(_QWORD *)(a1 + 204) = 1; /*0x1802f7604*/
  return result; /*0x1802f75f6*/
}
