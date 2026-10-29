void sub_18036A640()
{
  _QWORD *v0; // rbx
  int v1; // edi
  int v2; // r8d
  int v3; // ecx
  int v4; // r9d
  int v5; // ecx
  __int64 *v6; // rbx
  __int64 i; // rdi

  v0 = (_QWORD *)qword_1812D1E70; /*0x18036a64a*/
  v1 = 0; /*0x18036a651*/
  if ( qword_1812D1E70 != 0 ) /*0x18036a656*/
    goto LABEL_21; /*0x18036a656*/
  v0 = operator new(Size: 0x40u); /*0x18036a660*/
  qword_1812D1E70 = (__int64)v0; /*0x18036a668*/
  *v0 = &CDeviceDriver::`vftable'; /*0x18036a676*/
  v0[1] = 0; /*0x18036a679*/
  v0[2] = 0; /*0x18036a67d*/
  v0[3] = 0; /*0x18036a681*/
  v0[4] = 0; /*0x18036a685*/
  v0[5] = 0; /*0x18036a689*/
  v0[6] = 0; /*0x18036a68d*/
  *((_DWORD *)v0 + 14) = 0; /*0x18036a691*/
  *((_WORD *)v0 + 30) = 0; /*0x18036a694*/
  if ( v0 != nullptr ) /*0x18036a69b*/
  {
LABEL_21:
    if ( qword_1812D1E68 != 0 ) /*0x18036a6a8*/
    {
      sub_180365DA0(); /*0x18036a6ae*/
      if ( *(_BYTE *)(qword_1812D1E68 + 45) != 0 ) /*0x18036a6be*/
      {
        v2 = (*(_BYTE *)(qword_1812D1E68 + 50) != 0) | 2; /*0x18036a6d0*/
        if ( *(_BYTE *)(qword_1812D1E68 + 51) == 0 ) /*0x18036a6d8*/
          v2 = *(_BYTE *)(qword_1812D1E68 + 50) != 0; /*0x18036a6d8*/
        v3 = (*(_BYTE *)(qword_1812D1E68 + 46) != 0) | 2; /*0x18036a6e7*/
        if ( *(_BYTE *)(qword_1812D1E68 + 47) == 0 ) /*0x18036a6ee*/
          v3 = *(_BYTE *)(qword_1812D1E68 + 46) != 0; /*0x18036a6ee*/
        v4 = v3 | 4; /*0x18036a6f4*/
        if ( *(_BYTE *)(qword_1812D1E68 + 48) == 0 ) /*0x18036a6fc*/
          v4 = v3; /*0x18036a6fc*/
        v5 = *((_DWORD *)v0 + 10); /*0x18036a700*/
        if ( v5 == v2 ) /*0x18036a70f*/
        {
          if ( (int)--*((_DWORD *)v0 + 14) <= 0 ) /*0x18036a723*/
          {
            v1 = v2; /*0x18036a725*/
            *((_DWORD *)v0 + 14) = 12; /*0x18036a728*/
          }
        }
        else
        {
          v1 = v4; /*0x18036a711*/
          *((_DWORD *)v0 + 14) = 18; /*0x18036a714*/
        }
        *((_DWORD *)v0 + 10) = v2; /*0x18036a732*/
        *((_DWORD *)v0 + 11) = v4; /*0x18036a736*/
        *((_DWORD *)v0 + 12) = v5 - (v2 & v5); /*0x18036a73a*/
        *((_DWORD *)v0 + 13) = v1; /*0x18036a73d*/
        sub_180368F10(a1: v0); /*0x18036a740*/
      }
    }
  }
  if ( byte_1812CF8F9 != 0 ) /*0x18036a74c*/
  {
    v6 = &qword_1812D1E80; /*0x18036a74e*/
    for ( i = 7; i != 0; --i ) /*0x18036a755*/
    {
      (*(void (__fastcall **)(__int64))(*(_QWORD *)*v6 + 40LL))(a1: *v6); /*0x18036a766*/
      ++v6; /*0x18036a769*/
    }
  }
}
