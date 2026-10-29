__int64 __usercall sub_180001260@<xmm0>(__int64 a1@<rcx>)
{
  __int64 v1; // rcx
  __int64 result; // xmm0_8
  _BYTE v3[12]; // [rsp+20h] [rbp-28h] BYREF
  __int64 v4; // [rsp+2Ch] [rbp-1Ch]

  v1 = *(_QWORD *)(a1 + 88368); /*0x180001273*/
  if ( v1 == 0 ) /*0x18000127d*/
    return 0; /*0x1800012c9*/
  (*(void (__fastcall **)(__int64, _BYTE *))(*(_QWORD *)v1 + 200LL))(a1: v1, a2: v3); /*0x180001287*/
  HIDWORD(result) = 0; /*0x180001292*/
  *(float *)&result = (float)(int)v4; /*0x180001295*/
  if ( v4 < 0 ) /*0x18000129d*/
    *(float *)&result = *(float *)&result + 1.8446744e19; /*0x18000129f*/
  *(float *)&result = (float)(*(float *)&result * 1000.0) / 44100.0; /*0x1800012af*/
  return result; /*0x1800012b7*/
}
