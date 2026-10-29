void __fastcall sub_18036BF30(__int64 a1)
{
  _QWORD *v1; // rdx
  int v2; // ebx
  __int64 v4; // r8
  __int64 v5; // rcx

  v1 = (_QWORD *)qword_1812D1E70; /*0x18036bf44*/
  *(_BYTE *)(a1 + 24) = 0; /*0x18036bf54*/
  *(_DWORD *)(a1 + 20) = 0; /*0x18036bf58*/
  v2 = 0; /*0x18036bf5b*/
  *(_WORD *)(a1 + 26) = 0; /*0x18036bf60*/
  *(_BYTE *)(a1 + 28) = 0; /*0x18036bf66*/
  do /*0x18036c004*/
  {
    if ( v1 == nullptr ) /*0x18036bf73*/
    {
      v1 = operator new(Size: 0x40u); /*0x18036bf7d*/
      qword_1812D1E70 = (__int64)v1; /*0x18036bf85*/
      *v1 = &CDeviceDriver::`vftable'; /*0x18036bf8c*/
      v1[1] = 0; /*0x18036bf8f*/
      v1[2] = 0; /*0x18036bf93*/
      v1[3] = 0; /*0x18036bf97*/
      v1[4] = 0; /*0x18036bf9b*/
      v1[5] = 0; /*0x18036bf9f*/
      v1[6] = 0; /*0x18036bfa3*/
      *((_DWORD *)v1 + 14) = 0; /*0x18036bfa7*/
      *((_WORD *)v1 + 30) = 0; /*0x18036bfaa*/
    }
    if ( (unsigned int)v2 <= 2 ) /*0x18036bfb1*/
    {
      if ( v2 != 0 ) /*0x18036bfbd*/
      {
        if ( v2 == 1 ) /*0x18036bfc2*/
          v4 = 1; /*0x18036bfcf*/
        else
          v4 = 3; /*0x18036bfc9*/
      }
      else
      {
        v4 = 2; /*0x18036bfd7*/
      }
      v5 = qword_1812D1E68; /*0x18036bfdd*/
      *(_QWORD *)(qword_1812D1E68 + 16 * v4 + 2504) = 0; /*0x18036bfea*/
      *(_DWORD *)(v5 + 16 * (v4 + 157)) = 0; /*0x18036bffc*/
    }
    ++v2; /*0x18036bfff*/
  }
  while ( v2 < 3 ); /*0x18036c004*/
  *(_BYTE *)(a1 + 36) = 0; /*0x18036c014*/
}
