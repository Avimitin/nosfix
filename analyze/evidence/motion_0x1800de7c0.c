__int64 __fastcall sub_1800DE7C0(int a1, __int64 a2, __int64 a3, unsigned int *a4)
{
  __int64 result; // rax
  int v8; // eax
  int v9; // eax
  __int64 v10; // rax
  __int64 v11; // rax
  int v12; // [rsp+30h] [rbp+8h] BYREF

  v12 = a1; /*0x1800de7d2*/
  result = sub_1800DD580(a1: &v12, a2: 1); /*0x1800de7e6*/
  if ( (_BYTE)result != 0 ) /*0x1800de7ed*/
  {
    v8 = sub_1800DD670(a1: &v12); /*0x1800de7f4*/
    if ( v8 != 0 ) /*0x1800de7fb*/
    {
      v9 = v8 - 1; /*0x1800de7fd*/
      if ( v9 != 0 ) /*0x1800de800*/
      {
        if ( v9 == 1 ) /*0x1800de805*/
        {
          v10 = sub_1800D6F40(); /*0x1800de807*/
          sub_1800D7ED0(a1: v10, a2, a3); /*0x1800de815*/
          sub_1800D2630(a1: a4); /*0x1800de81d*/
        }
      }
      else
      {
        sub_1800DE920(a1: &v12, a2, a3, a4); /*0x1800de832*/
      }
    }
    else
    {
      sub_1800DEA10(a1: &v12, a2, a3, a4); /*0x1800de847*/
    }
    v11 = sub_1800D6F40(); /*0x1800de84c*/
    return sub_1800D7E90(a1: v11); /*0x1800de854*/
  }
  return result; /*0x1800de859*/
}
