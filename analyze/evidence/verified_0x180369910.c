__int64 __fastcall sub_180369910(
        __int64 a1,
        unsigned int a2,
        unsigned __int8 a3,
        unsigned __int8 a4,
        unsigned __int8 a5)
{
  __int64 v6; // r10
  unsigned int v7; // edx
  __int64 v8; // r8
  __int64 v9; // rcx
  __int64 result; // rax

  if ( a2 <= 2 ) /*0x180369920*/
  {
    v6 = 4; /*0x180369922*/
    if ( a2 != 0 ) /*0x18036992a*/
    {
      v7 = a2 - 1; /*0x18036992c*/
      if ( v7 != 0 ) /*0x18036992f*/
      {
        if ( v7 == 1 ) /*0x180369934*/
          v6 = 3; /*0x180369936*/
      }
      else
      {
        v6 = 1; /*0x18036993c*/
      }
    }
    else
    {
      v6 = 2; /*0x180369944*/
    }
    v8 = qword_1812D1E68; /*0x18036994a*/
    v9 = 2 * v6; /*0x18036995a*/
    *(_DWORD *)(qword_1812D1E68 + 8 * v9 + 2504) = a3; /*0x180369964*/
    *(_DWORD *)(v8 + 8 * v9 + 2508) = a4; /*0x180369976*/
    *(_DWORD *)(v8 + 16 * (v6 + 157)) = a5; /*0x18036997e*/
    return 2 * (v6 + 157); /*0x180369973*/
  }
  return result; /*0x180369982*/
}
