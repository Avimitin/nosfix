bool __fastcall sub_1802F4E20(__int64 a1)
{
  bool result; // al
  int v2; // eax

  result = *(_DWORD *)(a1 + 300) == 1 /*0x1802f4e29*/
        || (v2 = *(_DWORD *)(a1 + 24) - dword_18070A610 - HIDWORD(qword_181366908), dword_1813668F8 >= v2)
        && v2 >= dword_1813668FC
        || dword_1813668F0 >= v2 && v2 >= dword_1813668F4
        || dword_1813668E8 >= v2 && v2 >= dword_1813668EC;
  return result; /*0x1802f4e2b*/
}
