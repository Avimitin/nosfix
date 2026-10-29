char sub_18036A0B0()
{
  _WORD *v0; // rax
  _WORD *v1; // rax
  _WORD *v2; // rax
  _WORD *v3; // rax
  _WORD *v4; // rbx
  void *v5; // rax
  __int64 v6; // rax
  __int64 *v7; // rbx
  __int64 i; // rdi
  void *v10; // [rsp+40h] [rbp+8h]
  void *v11; // [rsp+40h] [rbp+8h]

  v0 = operator new(Size: 0x38u); /*0x18036a0c9*/
  v0[4] = 0; /*0x18036a0d3*/
  *(_QWORD *)v0 = &CCoinDevice::`vftable'; /*0x18036a0e0*/
  *((_DWORD *)v0 + 4) = 0; /*0x18036a0e5*/
  *((_BYTE *)v0 + 20) = 1; /*0x18036a0e8*/
  *((_QWORD *)v0 + 3) = 0; /*0x18036a0ec*/
  *((_QWORD *)v0 + 4) = 0; /*0x18036a0f0*/
  *((_QWORD *)v0 + 5) = 0; /*0x18036a0f4*/
  *((_DWORD *)v0 + 12) = 0; /*0x18036a0f8*/
  qword_1812D1E80 = (__int64)v0; /*0x18036a0fb*/
  v1 = operator new(Size: 0x20u); /*0x18036a105*/
  v1[4] = 0; /*0x18036a10f*/
  *(_QWORD *)v1 = &CInputSwitchDevice::`vftable'; /*0x18036a11a*/
  *((_QWORD *)v1 + 2) = 0; /*0x18036a11d*/
  *((_DWORD *)v1 + 6) = 0; /*0x18036a121*/
  qword_1812D1E88 = (__int64)v1; /*0x18036a124*/
  v2 = operator new(Size: 0x30u); /*0x18036a12e*/
  v2[4] = 0; /*0x18036a138*/
  *(_QWORD *)v2 = &CTowerLEDDevice::`vftable'; /*0x18036a143*/
  *((_QWORD *)v2 + 2) = 0; /*0x18036a146*/
  *((_DWORD *)v2 + 6) = 0; /*0x18036a14a*/
  v2[14] = 0; /*0x18036a14d*/
  *((_DWORD *)v2 + 8) = 0; /*0x18036a151*/
  *((_BYTE *)v2 + 36) = 0; /*0x18036a154*/
  *((_DWORD *)v2 + 10) = 0; /*0x18036a158*/
  *((_BYTE *)v2 + 44) = 0; /*0x18036a15b*/
  qword_1812D1E90 = (__int64)v2; /*0x18036a15f*/
  v3 = operator new(Size: 0x10u); /*0x18036a169*/
  v3[4] = 0; /*0x18036a173*/
  *(_QWORD *)v3 = &CSecurityKeyDevice::`vftable'; /*0x18036a17e*/
  qword_1812D1E98 = (__int64)v3; /*0x18036a181*/
  v4 = operator new(Size: 0x48u); /*0x18036a190*/
  v4[4] = 0; /*0x18036a198*/
  *(_QWORD *)v4 = &CCardUnitDevice::`vftable'; /*0x18036a1a3*/
  *((_QWORD *)v4 + 2) = 4; /*0x18036a1a6*/
  *((_DWORD *)v4 + 15) = 0; /*0x18036a1ae*/
  *((_DWORD *)v4 + 16) = 1; /*0x18036a1b1*/
  avs2_core_244(a1: v4 + 12, a2: 0, a3: 17); /*0x18036a1c2*/
  avs2_core_244(a1: (char *)v4 + 41, a2: 0, a3: 17); /*0x18036a1d2*/
  qword_1812D1EA0 = (__int64)v4; /*0x18036a1d8*/
  v5 = operator new(Size: 0x4100u); /*0x18036a1e4*/
  v10 = v5; /*0x18036a1e9*/
  __wind /*0xf1c000000000001c*/
  {
    v6 = sub_18036B3F0(a1: v5); /*0x18036a1f1*/
  }
  __unwind
  {
    j_j_free(Block: v10); /*0x18038df62*/
  }
  qword_1812D1EA8 = v6; /*0x18036a1f7*/
  v11 = operator new(Size: 0x178u); /*0x18036a208*/
  qword_1812D1EB0 = sub_1803526C0(a1: v11); /*0x18036a215*/
  v7 = &qword_1812D1E80; /*0x18036a21c*/
  for ( i = 7; i != 0; --i ) /*0x18036a223*/
  {
    (*(void (__fastcall **)(__int64))(*(_QWORD *)*v7 + 8LL))(a1: *v7); /*0x18036a236*/
    ++v7; /*0x18036a239*/
  }
  byte_1812CF8F9 = 1; /*0x18036a243*/
  return 1; /*0x18036a24c*/
}
