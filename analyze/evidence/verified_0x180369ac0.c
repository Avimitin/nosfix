__int64 (__fastcall ***__fastcall sub_180369AC0(__int64 a1))(_QWORD, __int64)
{
  __int64 (__fastcall ***result)(_QWORD, __int64); // rax
  __int64 (__fastcall ***v2)(_QWORD, __int64); // rcx
  int i; // edi
  __int64 v4; // r9
  __int64 v5; // r8

  result = (__int64 (__fastcall ***)(_QWORD, __int64))sub_180311A00(a1); /*0x180369ac6*/
  v2 = (__int64 (__fastcall ***)(_QWORD, __int64))qword_1812D1E70; /*0x180369acb*/
  if ( qword_1812D1E70 != 0 ) /*0x180369ad7*/
  {
    for ( i = 0; i < 3; ++i ) /*0x180369aee*/
    {
      if ( v2 == nullptr ) /*0x180369af3*/
      {
        result = (__int64 (__fastcall ***)(_QWORD, __int64))operator new(Size: 0x40u); /*0x180369afa*/
        v2 = result; /*0x180369aff*/
        qword_1812D1E70 = (__int64)result; /*0x180369b07*/
        *result = (__int64 (__fastcall **)(_QWORD, __int64))&CDeviceDriver::`vftable'; /*0x180369b0e*/
        result[1] = nullptr; /*0x180369b11*/
        result[2] = nullptr; /*0x180369b15*/
        result[3] = nullptr; /*0x180369b19*/
        result[4] = nullptr; /*0x180369b1d*/
        result[5] = nullptr; /*0x180369b21*/
        result[6] = nullptr; /*0x180369b25*/
        *((_DWORD *)result + 14) = 0; /*0x180369b29*/
        *((_WORD *)result + 30) = 0; /*0x180369b2c*/
      }
      if ( (unsigned int)i <= 2 ) /*0x180369b33*/
      {
        if ( i != 0 ) /*0x180369b3f*/
        {
          if ( i == 1 ) /*0x180369b44*/
            v4 = 1; /*0x180369b51*/
          else
            v4 = 3; /*0x180369b4b*/
        }
        else
        {
          v4 = 2; /*0x180369b59*/
        }
        v5 = qword_1812D1E68; /*0x180369b5f*/
        *(_QWORD *)(qword_1812D1E68 + 16 * v4 + 2504) = 0; /*0x180369b6c*/
        result = (__int64 (__fastcall ***)(_QWORD, __int64))(2 * (v4 + 157)); /*0x180369b7b*/
        *(_DWORD *)(v5 + 16 * (v4 + 157)) = 0; /*0x180369b7e*/
      }
    }
    if ( v2 != nullptr ) /*0x180369b9a*/
      result = (__int64 (__fastcall ***)(_QWORD, __int64))(**v2)(a1: v2, a2: 1); /*0x180369ba4*/
  }
  qword_1812D1E70 = 0; /*0x180369ba6*/
  return result; /*0x180369bad*/
}
