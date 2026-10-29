__int64 __fastcall sub_1802F79E0(__int64 a1, int a2, __int64 a3, int a4)
{
  __int64 v6; // rcx
  __int64 (__fastcall *v8)(); // r8
  char v9; // al
  char v10; // di
  __int64 (__fastcall *v11)(); // rdx
  int v12; // eax

  v6 = *(_QWORD *)(a1 + 8); /*0x1802f79fa*/
  v10 = false; /*0x1802f7a29*/
  if ( *(_DWORD *)(v6 + 48) != 2 )
  {
    v8 = *(__int64 (__fastcall **)())(*(_QWORD *)a1 + 192LL); /*0x1802f7a09*/
    v9 = v8 == sub_1800A2C30 ? *(_BYTE *)(v6 + 97) : ((__int64 (__fastcall *)(__int64))v8)(a1);
    if ( v9 == 0 ) /*0x1802f7a22*/
      v10 = true; /*0x1802f7a04*/
  }
  v11 = *(__int64 (__fastcall **)())(*(_QWORD *)a1 + 240LL); /*0x1802f7a2e*/
  if ( v11 == sub_1800A2C90 ) /*0x1802f7a3f*/
    v12 = *(_DWORD *)(a1 + 84); /*0x1802f7a41*/
  else
    v12 = ((__int64 (__fastcall *)(__int64))v11)(a1); /*0x1802f7a8e*/
  return sub_180303E80( /*0x1802f7a6e*/
           a1: v12,
           a2: (_DWORD)v11,
           a3: a2,
           a4,
           a5: *(_DWORD *)(a1 + 32),
           a6: *(_DWORD *)(a1 + 64),
           a7: *(_QWORD *)(a1 + 8),
           a8: a4,
           a9: v10);
}
