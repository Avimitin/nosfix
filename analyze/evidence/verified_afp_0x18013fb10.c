__int64 __fastcall sub_18013FB10(__int64 a1)
{
  __int64 result; // rax
  int v3; // r9d
  _DWORD v4[6]; // [rsp+20h] [rbp-18h] BYREF

  if ( (*(_DWORD *)(a1 + 36) & 0x800000) == 0 ) /*0x18013fb21*/
    return 0; /*0x18013fb83*/
  v4[0] = 0; /*0x18013fb2b*/
  result = sub_180093070(a1, a2: 75, a3: v4); /*0x18013fb38*/
  if ( (_DWORD)result != 0 ) /*0x18013fb3f*/
  {
    *(_DWORD *)(a1 + 36) &= ~0x800000u; /*0x18013fb74*/
  }
  else
  {
    v3 = 0; /*0x18013fb4d*/
    if ( *(unsigned __int16 *)(a1 + 118) < v4[0] - 1 ) /*0x18013fb5b*/
      v3 = 0x20000; /*0x18013fb5b*/
    *(_DWORD *)(a1 + 32) = v3 | *(_DWORD *)(a1 + 32) & 0xFFFDFFFF; /*0x18013fb69*/
  }
  return result; /*0x18013fb6d*/
}
