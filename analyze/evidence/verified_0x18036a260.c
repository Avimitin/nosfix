_QWORD *__fastcall sub_18036A260(__int64 a1)
{
  _QWORD *result; // rax
  char v2; // di
  __int64 v3; // rdx
  int i; // ebx
  __int64 v5; // r8
  __int64 v6; // rbx
  __int64 v7; // rdx
  __int64 v8; // rcx
  __int64 v9; // rdx
  __int64 v10; // rcx

  result = (_QWORD *)qword_1812D1E90; /*0x18036a274*/
  v2 = a1; /*0x18036a27b*/
  if ( qword_1812D1E90 != 0 ) /*0x18036a281*/
    *(_BYTE *)(qword_1812D1E90 + 44) = a1; /*0x18036a283*/
  v3 = qword_1812D1E70; /*0x18036a286*/
  if ( qword_1812D1E70 == 0 ) /*0x18036a299*/
  {
    result = operator new(Size: 0x40u); /*0x18036a29e*/
    v3 = (__int64)result; /*0x18036a2a3*/
    qword_1812D1E70 = (__int64)result; /*0x18036a2ab*/
    *result = &CDeviceDriver::`vftable'; /*0x18036a2b2*/
    result[1] = 0; /*0x18036a2b5*/
    result[2] = 0; /*0x18036a2b9*/
    result[3] = 0; /*0x18036a2bd*/
    result[4] = 0; /*0x18036a2c1*/
    result[5] = 0; /*0x18036a2c5*/
    result[6] = 0; /*0x18036a2c9*/
    *((_DWORD *)result + 14) = 0; /*0x18036a2cd*/
    *((_WORD *)result + 30) = 0; /*0x18036a2d0*/
  }
  *(_BYTE *)(v3 + 61) = v2; /*0x18036a2d4*/
  if ( v2 != 0 ) /*0x18036a2db*/
  {
    for ( i = 0; i < 3; ++i ) /*0x18036a2e1*/
    {
      if ( v3 == 0 ) /*0x18036a2e6*/
      {
        result = operator new(Size: 0x40u); /*0x18036a2eb*/
        v3 = (__int64)result; /*0x18036a2f0*/
        qword_1812D1E70 = (__int64)result; /*0x18036a2f8*/
        *result = &CDeviceDriver::`vftable'; /*0x18036a2ff*/
        result[1] = 0; /*0x18036a302*/
        result[2] = 0; /*0x18036a306*/
        result[3] = 0; /*0x18036a30a*/
        result[4] = 0; /*0x18036a30e*/
        result[5] = 0; /*0x18036a312*/
        result[6] = 0; /*0x18036a316*/
        *((_DWORD *)result + 14) = 0; /*0x18036a31a*/
        *((_WORD *)result + 30) = 0; /*0x18036a31d*/
      }
      if ( (unsigned int)i <= 2 ) /*0x18036a324*/
      {
        if ( i != 0 ) /*0x18036a330*/
        {
          if ( i == 1 ) /*0x18036a335*/
            v5 = 1; /*0x18036a342*/
          else
            v5 = 3; /*0x18036a33c*/
        }
        else
        {
          v5 = 2; /*0x18036a34a*/
        }
        a1 = qword_1812D1E68; /*0x18036a350*/
        *(_QWORD *)(qword_1812D1E68 + 16 * v5 + 2504) = 0; /*0x18036a35d*/
        result = (_QWORD *)(2 * (v5 + 157)); /*0x18036a36c*/
        *(_DWORD *)(a1 + 16 * (v5 + 157)) = 0; /*0x18036a36f*/
      }
    }
  }
  v6 = qword_1812D1EB0; /*0x18036a37d*/
  if ( qword_1812D1EB0 != 0 ) /*0x18036a387*/
  {
    result = (_QWORD *)sub_180311A00(a1); /*0x18036a389*/
    if ( *((_BYTE *)result + 37) != 0 ) /*0x18036a392*/
    {
      *(_BYTE *)(v6 + 357) = v2; /*0x18036a394*/
      *(_BYTE *)(sub_180353AF0(a1: v8, a2: v7) + 5898) = v2 ^ 1; /*0x18036a3a7*/
      result = (_QWORD *)sub_180353AF0(a1: v10, a2: v9); /*0x18036a3ad*/
      *((_BYTE *)result + 5899) = v2; /*0x18036a3b2*/
    }
  }
  byte_1812CF8D3 = v2; /*0x18036a3c8*/
  return result; /*0x18036a3b9*/
}
