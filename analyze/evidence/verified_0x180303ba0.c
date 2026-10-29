bool __fastcall sub_180303BA0(int a1, int a2)
{
  __int64 v3; // rdx
  int v4; // r9d
  int v5; // ecx
  bool result; // al

  result = true; /*0x180303bfd*/
  if ( dword_1813E4968 >= 0 )
  {
    v3 = 2LL * a2; /*0x180303bb9*/
    v4 = dword_1813668B0[v3 + 129074]; /*0x180303bc1*/
    v5 = v4 <= 0 ? 0 : dword_1813668B0[v3 + 129075];
    if ( a1 > dword_1813E4968 + dword_1813E496C + v5 && v4 > 0 && dword_1813668B0[v3 + 129075] > 0 ) /*0x180303bf8*/
      return false; /*0x180303bad*/
  }
  return result; /*0x180303bfc*/
}
