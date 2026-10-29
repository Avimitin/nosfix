__int64 dll_entry_main()
{
  __int64 *v0; // rdi
  int v1; // r8d
  __int128 v2; // kr00_16
  __int64 *v3; // rbx
  char *v4; // rbx
  int v5; // r8d
  __int128 v6; // kr10_16
  HMODULE ModuleHandleA; // rax
  unsigned int v9; // ebx
  _BYTE v10[256]; // [rsp+40h] [rbp-118h] BYREF

  avs2_core_381(a1: "Main", a2: "dll_entry_main() started"); /*0x1801c7b7d*/
  if ( byte_1812D342D == 0 ) /*0x1801c7b92*/
  {
    sub_180102B80(a1: v10, a2: 0, a3: 256); /*0x1801c7ba5*/
    if ( (unsigned __int8)avs2_core_71(a1: v10, a2: "/dev/raw/") == 0 ) /*0x1801c7bbe*/
      avs2_core_258(a1: v10, a2: 256, a3: "./"); /*0x1801c7bd1*/
    sub_180363D90(a1: v10); /*0x1801c7bdc*/
    v0 = &qword_18070C108; /*0x1801c7bee*/
    sub_180077450(a1: &qword_18070C108, a2: ".", a3: 1); /*0x1801c7bf8*/
    v2 = xmmword_18070C118; /*0x1801c7c04*/
    if ( *((_QWORD *)&xmmword_18070C118 + 1) == (_QWORD)xmmword_18070C118 ) /*0x1801c7c15*/
    {
      sub_180077AE0(a1: (unsigned int)&qword_18070C108, a2: 1, a3: v1, a4: (unsigned int)";", a5: 1); /*0x1801c7c67*/
    }
    else
    {
      *(_QWORD *)&xmmword_18070C118 = xmmword_18070C118 + 1; /*0x1801c7c1b*/
      v3 = &qword_18070C108; /*0x1801c7c22*/
      if ( *((_QWORD *)&v2 + 1) >= 0x10u ) /*0x1801c7c29*/
        v3 = (__int64 *)qword_18070C108; /*0x1801c7c29*/
      v4 = (char *)v3 + v2; /*0x1801c7c31*/
      sub_180102EE0(a1: v4, a2: ";", a3: 1); /*0x1801c7c44*/
      v4[1] = 0; /*0x1801c7c49*/
    }
    v6 = xmmword_18070C118; /*0x1801c7c73*/
    if ( *((_QWORD *)&xmmword_18070C118 + 1) - (_QWORD)xmmword_18070C118 < 7u ) /*0x1801c7c84*/
    {
      sub_180077AE0(a1: (unsigned int)&qword_18070C108, a2: 7, a3: v5, a4: (unsigned int)"modules", a5: 7); /*0x1801c7cd4*/
    }
    else
    {
      *(_QWORD *)&xmmword_18070C118 = xmmword_18070C118 + 7; /*0x1801c7c8a*/
      if ( *((_QWORD *)&v6 + 1) >= 0x10u ) /*0x1801c7c95*/
        v0 = (__int64 *)qword_18070C108; /*0x1801c7c95*/
      sub_180102EE0(a1: (char *)v0 + v6, a2: "modules", a3: 7); /*0x1801c7cb1*/
      *((_BYTE *)v0 + v6 + 7) = 0; /*0x1801c7cb6*/
    }
    byte_1812D342D = 1; /*0x1801c7cd9*/
  }
  if ( (unsigned __int8)sub_18019AEB0() != 0 ) /*0x1801c7ce7*/
    return 0; /*0x1801c7ce9*/
  ModuleHandleA = GetModuleHandleA(lpModuleName: "nostalgia"); /*0x1801c7cf4*/
  v9 = sub_1801C7FB0(a1: ModuleHandleA); /*0x1801c7d02*/
  avs2_core_381(a1: "Main", a2: "dll_entry_main() finished"); /*0x1801c7d1c*/
  return v9; /*0x1801c7d24*/
}
