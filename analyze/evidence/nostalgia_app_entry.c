__int64 __fastcall sub_1801C7FB0(__int64 a1)
{
  HANDLE CurrentProcess; // rax
  __int64 v3; // rax
  __int64 v4; // r8
  __int64 result; // rax
  HANDLE TokenHandle[2]; // [rsp+38h] [rbp-D0h] BYREF
  struct _TOKEN_PRIVILEGES v7[5]; // [rsp+48h] [rbp-C0h] BYREF
  struct _TOKEN_PRIVILEGES NewState; // [rsp+98h] [rbp-70h] BYREF
  void **hData; // [rsp+A8h] [rbp-60h] BYREF
  char v10; // [rsp+B0h] [rbp-58h]
  void **v11; // [rsp+B8h] [rbp-50h]
  _QWORD v12[10]; // [rsp+C0h] [rbp-48h]
  __int64 v13; // [rsp+110h] [rbp+8h]
  __int64 v14; // [rsp+118h] [rbp+10h]
  __int64 v15; // [rsp+120h] [rbp+18h]
  __int64 v16; // [rsp+128h] [rbp+20h]
  __int64 v17; // [rsp+130h] [rbp+28h]
  __int64 v18; // [rsp+138h] [rbp+30h]
  __int64 v19; // [rsp+140h] [rbp+38h]
  __int64 v20; // [rsp+148h] [rbp+40h]
  __int64 v21; // [rsp+158h] [rbp+50h]
  _QWORD v22[2]; // [rsp+160h] [rbp+58h] BYREF
  int v23; // [rsp+170h] [rbp+68h]
  __int16 v24; // [rsp+174h] [rbp+6Ch]
  int v25; // [rsp+178h] [rbp+70h]

  TokenHandle[1] = (HANDLE)-2LL; /*0x1801c7fc2*/
  timeBeginPeriod(uPeriod: 1u); /*0x1801c7fec*/
  hData = &CApp::`vftable'; /*0x1801c7ff9*/
  v10 = 0; /*0x1801c7ffd*/
  v11 = &CWindow::`vftable'; /*0x1801c8008*/
  sub_180102B80(a1: v7, a2: 0, a3: 80); /*0x1801c8017*/
  qmemcpy(v12, v7, sizeof(v12)); /*0x1801c8021*/
  v13 = 0; /*0x1801c804a*/
  v14 = 0; /*0x1801c804e*/
  v15 = 0; /*0x1801c8052*/
  v16 = 0; /*0x1801c8056*/
  v17 = 0; /*0x1801c805a*/
  v18 = 0; /*0x1801c805e*/
  v19 = 0; /*0x1801c8062*/
  v20 = 0; /*0x1801c8066*/
  sub_180102B80(a1: &unk_1812CF390, a2: 0, a3: 1024); /*0x1801c8079*/
  v21 = 0; /*0x1801c8080*/
  v22[0] = &CDefMemory::`vftable'; /*0x1801c808b*/
  qword_1812D1CF8 = (__int64)v22; /*0x1801c8093*/
  v22[1] = 0; /*0x1801c809a*/
  v23 = 0; /*0x1801c809e*/
  v24 = 0; /*0x1801c80a1*/
  v25 = 0; /*0x1801c80a5*/
  if ( FindWindowA(lpClassName: ClassName, lpWindowName: "Microsoft Windows") != nullptr ) /*0x1801c80bf*/
  {
    CurrentProcess = GetCurrentProcess(); /*0x1801c80c1*/
    OpenProcessToken(ProcessHandle: CurrentProcess, DesiredAccess: 0x28u, TokenHandle); /*0x1801c80d2*/
    LookupPrivilegeValueA(lpSystemName: nullptr, lpName: "SeShutdownPrivilege", lpLuid: &NewState.Privileges[0].Luid); /*0x1801c80e5*/
    NewState.PrivilegeCount = 1; /*0x1801c80eb*/
    NewState.Privileges[0].Attributes = 2; /*0x1801c80f2*/
    AdjustTokenPrivileges( /*0x1801c8111*/
      TokenHandle: TokenHandle[0],
      DisableAllPrivileges: false,
      &NewState,
      BufferLength: 0,
      PreviousState: nullptr,
      ReturnLength: nullptr);
    ExitWindowsEx(uFlags: 1u, dwReason: 0); /*0x1801c811c*/
  }
  sub_18031C350(); /*0x1801c8122*/
  *(_QWORD *)(sub_18031C3C0() + 40) = sub_180334550; /*0x1801c8133*/
  v3 = sub_18031C3C0(); /*0x1801c8137*/
  *(_QWORD *)(v3 + 56) = sub_180334540; /*0x1801c8143*/
  __wind /*0xf1c000000000001c*/
  {
    sub_180374E10(); /*0x1801c8147*/
    dword_1811CEAF4 = *(_BYTE *)(sub_180311A00() + 39) == 0; /*0x1801c815a*/
    byte_180701AA4 = *(_BYTE *)(sub_180311A00() + 48); /*0x1801c8169*/
    LOBYTE(v4) = dword_1811CEAF4 == 0; /*0x1801c8176*/
    if ( (unsigned __int8)sub_1801C6510(a1: &hData, a2: a1, a3: v4) != 0 ) /*0x1801c818f*/
    {
      SetPropA(hWnd: hwnd, lpString: "PropClassWindowProc", &hData); /*0x1801c81ae*/
      sub_180360310(); /*0x1801c81b4*/
      HIBYTE(v24) = sub_1801C5A90(a1: &hData); /*0x1801c81c2*/
      if ( HIBYTE(v24) != 0 ) /*0x1801c81c7*/
      {
        sub_180360060(); /*0x1801c81c9*/
        sub_1801CFBC0(); /*0x1801c81ce*/
        sub_1801C3FE0(a1: &hData); /*0x1801c81d7*/
        sub_1801C52D0(a1: &hData); /*0x1801c81e0*/
      }
    }
    else
    {
      PostMessageA(hWnd: hwnd, Msg: 0x10u, wParam: 0, lParam: 0); /*0x1801c819b*/
    }
    timeEndPeriod(uPeriod: 1u); /*0x1801c81ea*/
    result = (unsigned int)(v23 + 1100); /*0x1801c81f3*/
  }
  __unwind
  {
    sub_1801C7080(a1: &hData); /*0x1803830cd*/
  }
  qword_1812D1CF8 = 0; /*0x1801c81f8*/
  return result; /*0x1801c81ff*/
}
