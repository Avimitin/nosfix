__int64 __fastcall sub_180352560(__int64 a1)
{
  __int64 result; // rax
  __int64 v3; // rax
  __int64 v4; // rcx
  __int64 v5; // rdx
  __int64 v6; // rcx
  __int64 v7; // rax

  result = sub_180311A00(a1); /*0x180352569*/
  if ( *(_BYTE *)(result + 37) != 0 && *(_BYTE *)(a1 + 356) != 0 ) /*0x18035257b*/
  {
    v3 = sub_180369760(); /*0x18035257d*/
    sub_1803520D0(a1: v4, a2: a1 + 16, a3: v3 + 2224); /*0x18035258d*/
    v7 = sub_180353AF0(a1: v6, a2: v5); /*0x180352592*/
    return sub_180353600(a1: v7, a2: a1 + 16); /*0x1803525a3*/
  }
  return result; /*0x18035259e*/
}
