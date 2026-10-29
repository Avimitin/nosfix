__int64 __fastcall sub_1802A83F0(__int64 a1)
{
  int v1; // eax
  int v2; // esi
  __int64 v4; // rdx
  int v5; // ebx
  int v6; // edi
  bool v7; // zf
  _DWORD *v8; // r9
  int v9; // r8d
  int v10; // edx
  __int64 v11; // rdx
  __int64 v12; // rbx
  int v13; // ecx
  int v14; // edi
  __int64 v15; // r14
  __int64 v16; // rcx
  __int64 v17; // rax
  __int64 result; // rax
  __int64 v19; // rbx
  int v20; // [rsp+20h] [rbp-28h] BYREF
  __int64 v21; // [rsp+24h] [rbp-24h]
  __int16 v22; // [rsp+2Ch] [rbp-1Ch]
  char v23; // [rsp+2Eh] [rbp-1Ah]
  char v24; // [rsp+2Fh] [rbp-19h]
  _QWORD *v25; // [rsp+50h] [rbp+8h]
  _QWORD *v26; // [rsp+50h] [rbp+8h]
  _QWORD *v27; // [rsp+50h] [rbp+8h]

  v1 = *(_DWORD *)(a1 + 28); /*0x1802a83fc*/
  v2 = 0; /*0x1802a83ff*/
  if ( v1 != 0 ) /*0x1802a840b*/
  {
    if ( v1 == 1 ) /*0x1802a8419*/
    {
      sub_180102B80(a1: a1 + 32, a2: 127, a3: 93); /*0x1802a8426*/
      v1 = *(_DWORD *)(a1 + 28); /*0x1802a842b*/
      *(_DWORD *)(a1 + 24) = 0; /*0x1802a842e*/
    }
    if ( v1 == 2 ) /*0x1802a843b*/
    {
      v4 = (unsigned int)(++*(_DWORD *)(a1 + 24) / 30); /*0x1802a8457*/
      if ( *(_DWORD *)(a1 + 24) / 30 != 0 ) /*0x1802a8459*/
      {
        if ( (int)v4 > 93 ) /*0x1802a84b6*/
        {
          *(_DWORD *)(a1 + 24) = 0; /*0x1802a84f2*/
        }
        else
        {
          v5 = ((int)v4 - 1) / 3; /*0x1802a84cb*/
          v6 = ((int)v4 - 1) % 3; /*0x1802a84d2*/
          sub_180102B80(a1: a1 + 32, a2: 0, a3: 93); /*0x1802a84d8*/
          *(_BYTE *)(v6 + v5 + a1 + 2LL * v5 + 32) = 127; /*0x1802a84eb*/
        }
      }
      else
      {
        if ( qword_1812D1E70 == 0 ) /*0x1802a8462*/
        {
          v25 = operator new(Size: (unsigned int)(v4 + 64)); /*0x1802a846c*/
          qword_1812D1E70 = (__int64)v25; /*0x1802a8471*/
          *v25 = &CDeviceDriver::`vftable'; /*0x1802a8478*/
          v25[1] = 0; /*0x1802a847b*/
          v25[2] = 0; /*0x1802a847f*/
          v25[3] = 0; /*0x1802a8483*/
          v25[4] = 0; /*0x1802a8487*/
          v25[5] = 0; /*0x1802a848b*/
          v25[6] = 0; /*0x1802a848f*/
          *((_DWORD *)v25 + 14) = 0; /*0x1802a8493*/
          *((_WORD *)v25 + 30) = 0; /*0x1802a8496*/
        }
        sub_180365780(a1, a2: v4); /*0x1802a849a*/
        sub_180102B80(a1: a1 + 32, a2: 127, a3: 93); /*0x1802a84ac*/
      }
    }
    if ( qword_1812D1E70 == 0 ) /*0x1802a84fc*/
    {
      v26 = operator new(Size: 0x40u); /*0x1802a8508*/
      qword_1812D1E70 = (__int64)v26; /*0x1802a850d*/
      *v26 = &CDeviceDriver::`vftable'; /*0x1802a8514*/
      v26[1] = 0; /*0x1802a8517*/
      v26[2] = 0; /*0x1802a851b*/
      v26[3] = 0; /*0x1802a851f*/
      v26[4] = 0; /*0x1802a8523*/
      v26[5] = 0; /*0x1802a8527*/
      v26[6] = 0; /*0x1802a852b*/
      *((_DWORD *)v26 + 14) = 0; /*0x1802a852f*/
      *((_WORD *)v26 + 30) = 0; /*0x1802a8532*/
    }
    v7 = qword_1812D1E70 == 0; /*0x1802a8536*/
    v8 = (_DWORD *)qword_1812D1E68; /*0x1802a853d*/
    v9 = *(unsigned __int8 *)(a1 + 34); /*0x1802a8544*/
    v10 = *(unsigned __int8 *)(a1 + 33); /*0x1802a8549*/
    *(_DWORD *)(qword_1812D1E68 + 2536) = *(unsigned __int8 *)(a1 + 32); /*0x1802a8551*/
    v8[635] = v10; /*0x1802a8558*/
    v8[636] = v9; /*0x1802a855f*/
    if ( v7 ) /*0x1802a8566*/
    {
      v27 = operator new(Size: 0x40u); /*0x1802a8572*/
      v8 = (_DWORD *)qword_1812D1E68; /*0x1802a8577*/
      qword_1812D1E70 = (__int64)v27; /*0x1802a857e*/
      *v27 = &CDeviceDriver::`vftable'; /*0x1802a8585*/
      v27[1] = 0; /*0x1802a8588*/
      v27[2] = 0; /*0x1802a858c*/
      v27[3] = 0; /*0x1802a8590*/
      v27[4] = 0; /*0x1802a8594*/
      v27[5] = 0; /*0x1802a8598*/
      v27[6] = 0; /*0x1802a859c*/
      *((_DWORD *)v27 + 14) = 0; /*0x1802a85a0*/
      *((_WORD *)v27 + 30) = 0; /*0x1802a85a3*/
    }
    v11 = *(unsigned __int8 *)(a1 + 37); /*0x1802a85a7*/
    v12 = a1 + 42; /*0x1802a85ab*/
    v13 = *(unsigned __int8 *)(a1 + 36); /*0x1802a85af*/
    v14 = 0; /*0x1802a85b3*/
    v15 = 28; /*0x1802a85b9*/
    v8[638] = *(unsigned __int8 *)(a1 + 35); /*0x1802a85c4*/
    v8[639] = v13; /*0x1802a85cb*/
    v8[640] = v11; /*0x1802a85d2*/
    v24 = 0; /*0x1802a85d9*/
    do /*0x1802a862a*/
    {
      v16 = *(unsigned __int8 *)(v12 + 1); /*0x1802a85e4*/
      v22 = *(_WORD *)(v12 - 1); /*0x1802a85f8*/
      v21 = 0; /*0x1802a85fd*/
      v20 = v14; /*0x1802a8602*/
      v23 = v16; /*0x1802a8606*/
      v17 = sub_180353AF0(a1: v16, a2: v11); /*0x1802a860a*/
      result = sub_180351450(a1: v17 + 5536, a2: &v20); /*0x1802a861b*/
      ++v14; /*0x1802a8620*/
      v12 += 3; /*0x1802a8622*/
      --v15; /*0x1802a8626*/
    }
    while ( v15 != 0 ); /*0x1802a862a*/
  }
  else
  {
    result = sub_180102B80(a1: a1 + 32, a2: 0, a3: 93); /*0x1802a8638*/
  }
  v19 = *(_QWORD *)(a1 + 128); /*0x1802a864e*/
  if ( byte_1806FC618 != 0 ) /*0x1802a865a*/
  {
    if ( v19 == 0 ) /*0x1802a865f*/
      return result; /*0x1802a865f*/
    if ( *(void ***)v19 == &CUIQCButton::`vftable' ) /*0x1802a8667*/
      result = *(unsigned __int8 *)(v19 + 114); /*0x1802a866d*/
    else
      result = (*(__int64 (__fastcall **)(_QWORD))(*(_QWORD *)v19 + 120LL))(a1: *(_QWORD *)(a1 + 128)); /*0x1802a8700*/
    if ( (_BYTE)result != 0 ) /*0x1802a8673*/
      result = off_1806FC620(); /*0x1802a8678*/
  }
  if ( v19 != 0 ) /*0x1802a8681*/
  {
    if ( *(void ***)v19 == &CUIQCButton::`vftable' ) /*0x1802a868d*/
    {
      *(_BYTE *)(v19 + 113) = *(_BYTE *)(v19 + 112); /*0x1802a8693*/
      *(_DWORD *)(v19 + 44) = 7; /*0x1802a8696*/
      *(_BYTE *)(v19 + 40) = 0; /*0x1802a869d*/
      *(_DWORD *)(v19 + 114) = 0; /*0x1802a86a1*/
      if ( *(_BYTE *)(v19 + 118) == 0 ) /*0x1802a86a8*/
      {
        *(_BYTE *)(v19 + 112) = 0; /*0x1802a86aa*/
        *(_QWORD *)(v19 + 88) = 0; /*0x1802a86ae*/
        *(_QWORD *)(v19 + 96) = 0; /*0x1802a86b2*/
        *(_QWORD *)(v19 + 104) = 0; /*0x1802a86b6*/
      }
      result = (unsigned int)(*(_DWORD *)(v19 + 128) - 1); /*0x1802a86c0*/
      *(_BYTE *)(v19 + 118) = 0; /*0x1802a86c2*/
      if ( (int)result > 0 ) /*0x1802a86c8*/
        v2 = result; /*0x1802a86c8*/
      *(_DWORD *)(v19 + 128) = v2; /*0x1802a86cb*/
    }
    else
    {
      return (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)v19 + 24LL))(a1: v19); /*0x1802a86ee*/
    }
  }
  return result; /*0x1802a86d1*/
}
