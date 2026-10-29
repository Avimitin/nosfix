__int64 *__fastcall sub_1802F7AA0(_QWORD *a1, int a2)
{
  __int64 v2; // r11
  int v4; // r10d
  int v5; // ebp
  __int64 (__fastcall *v6)(); // rdx
  char v7; // al
  bool v8; // bl
  __int64 v9; // rax
  int v10; // esi
  int v11; // r8d
  int v12; // r9d
  int v13; // ecx
  __int64 v14; // r10
  __int16 v15; // dx
  __int16 v16; // cx
  __int16 v17; // dx
  __int64 *result; // rax
  __int64 *v19; // rax
  int v20; // r10d
  int v21; // ebx
  int v22; // eax
  _QWORD *v23; // rax

  v2 = a1[1]; /*0x1802f7ab4*/
  v4 = dword_1813E4738 + dword_1813E476C * (*(_DWORD *)(v2 + 88) - 1); /*0x1802f7ad4*/
  v5 = v4 + (dword_1813E4738 + dword_1813E476C * *(_DWORD *)(v2 + 92) - v4) / 2; /*0x1802f7aee*/
  v8 = false; /*0x1802f7b1b*/
  if ( *(_DWORD *)(v2 + 48) != 2 )
  {
    v6 = *(__int64 (__fastcall **)())(*a1 + 192LL); /*0x1802f7af7*/
    v7 = v6 == sub_1800A2C30 ? *(_BYTE *)(v2 + 97) : v6();
    if ( v7 == 0 ) /*0x1802f7b15*/
      v8 = true; /*0x1802f7af2*/
  }
  v9 = sub_180304A70(); /*0x1802f7b1d*/
  v10 = 0; /*0x1802f7b22*/
  v11 = a2; /*0x1802f7b24*/
  v12 = 2; /*0x1802f7b29*/
  v13 = 2; /*0x1802f7b2f*/
  v14 = v9; /*0x1802f7b32*/
  if ( a2 < 0 ) /*0x1802f7b35*/
    v11 = 0; /*0x1802f7b35*/
  if ( v11 < 2 ) /*0x1802f7b3c*/
    v13 = v11; /*0x1802f7b3c*/
  v15 = *(_WORD *)(v9 + 2LL * v13 + 280); /*0x1802f7b43*/
  v16 = 9999; /*0x1802f7b4b*/
  v17 = v15 + 1; /*0x1802f7b50*/
  if ( v17 < 0 ) /*0x1802f7b56*/
    v17 = 0; /*0x1802f7b56*/
  if ( v17 < 9999 ) /*0x1802f7b5d*/
    v16 = v17; /*0x1802f7b5f*/
  if ( v11 < 2 ) /*0x1802f7b65*/
    v12 = v11; /*0x1802f7b65*/
  result = (__int64 *)v12; /*0x1802f7b69*/
  *(_WORD *)(v14 + 2LL * v12 + 280) = v16; /*0x1802f7b6c*/
  if ( v8 && a2 != 0 ) /*0x1802f7b7b*/
  {
    result = sub_18018B090(); /*0x1802f7b7d*/
    if ( *((_DWORD *)result + 28) == 0 ) /*0x1802f7b85*/
    {
      sub_18018B090(); /*0x1802f7b87*/
      v19 = sub_18018B090(); /*0x1802f7b90*/
      v21 = 595 - 102 * *((char *)v19 + 117) - v20; /*0x1802f7ba3*/
      LOBYTE(v10) = a2 != 1; /*0x1802f7ba9*/
      v22 = sub_180300D50(); /*0x1802f7bad*/
      v23 = (_QWORD *)sub_180300A50(a1: v22, a2: v10 + 23, a3: v5, a4: v21, a5: 0, a6: 4, a7: v5); /*0x1802f7bd3*/
      return (__int64 *)(*(__int64 (__fastcall **)(_QWORD, __int64))(*(_QWORD *)*v23 + 224LL))(a1: *v23, a2: 53); /*0x1802f7be3*/
    }
  }
  return result; /*0x1802f7be9*/
}
