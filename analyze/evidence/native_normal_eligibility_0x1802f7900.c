bool __fastcall sub_1802F7900(__int64 a1)
{
  int v1; // eax

  v1 = *(_DWORD *)(a1 + 24) - dword_18070A610 - HIDWORD(qword_181366914) - HIDWORD(qword_181366908); /*0x1802f790f*/
  if ( dword_1813668F8 >= v1 && v1 >= dword_1813668FC ) /*0x1802f7923*/
    return true; /*0x1802f7925*/
  if ( dword_1813668F0 >= v1 && v1 >= dword_1813668F4 ) /*0x1802f793f*/
    return true; /*0x1802f7941*/
  return dword_1813668E8 >= v1 && v1 >= dword_1813668EC; /*0x1802f795b*/
}
